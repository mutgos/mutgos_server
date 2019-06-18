/*
 * socket_SocketDriver.cpp
 */

#include <boost/shared_ptr.hpp>

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_StringConversion.h"

#include "comminterface/comm_RouterSessionManager.h"
#include "comminterface/comm_ClientConnection.h"

#include "socket_SecureRawSocketConnection.h"
#include "socket_SocketDriver.h"
#include "socket_SocketClientConnection.h"
#include "socket_ConnectionListener.h"

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    SocketDriver::SocketDriver(
        mutgos::comm::RouterSessionManager *router)
        : my_router_ptr(router),
          io_context{1},
          ssl_context(boost::asio::ssl::context::tlsv12_server),
          started(false),
          plain_started(false),
          ssl_started(false),
          inside_do_work(false)
    {
        // Set up the TLS context
        //
        if (config::comm::so_enable_ssl())
        {
            // This may be too stringent for some clients, in which case we
            // can relax it later.
            ssl_context.set_options(boost::asio::ssl::context::no_tlsv1);
            // TODO(hyena): Support password callbacks for the certificate?
            ssl_context.use_certificate_chain_file(
                config::comm::so_certificate());
            ssl_context.use_private_key_file(
                config::comm::so_certificate_private(),
                boost::asio::ssl::context::pem);
        }

        if (not my_router_ptr)
        {
            LOG(fatal, "socket", "SocketDriver", "router is null!");
        }
    }

    // ----------------------------------------------------------------------
    SocketDriver::~SocketDriver()
    {
        if (plain_started || ssl_started)
        {
            LOG(error, "socket", "~SocketDriver",
                "Destructed without calling stop()!");
        }

        if (not client_connections.empty())
        {
            LOG(error, "socket", "~SocketDriver",
                "Client connections still instantiated!");
        }
    }

    // ----------------------------------------------------------------------
    bool SocketDriver::start(void)
    {
        // Taken from 'advanced_server.cpp' in beast examples.
        // Makes the IO context, and creates and starts the connection
        // listener.
        //
        const boost::asio::ip::address address =
                boost::asio::ip::make_address("0.0.0.0");

        if ((not plain_started) and config::comm::so_enable_plain())
        {
            const unsigned short port = (unsigned short) config::comm::so_port();
            // Factory method to make PlainRawSocketConnections.
            const ConnectionListener::RawSocketFactory plain_factory
                = [](SocketDriver *driver, boost::asio::io_context &io_context) {
                    return new PlainRawSocketConnection(driver, io_context);
                };
            plain_started = boost::make_shared<ConnectionListener>(
                this,
                io_context,
                boost::asio::ip::tcp::endpoint(address, port),
                plain_factory)->start();

            LOG(info, "socket", "start",
                "Socket Driver started, listening on port "
                + text::to_string(port));
        }

        if ((not ssl_started) and config::comm::so_enable_ssl())
        {
            const unsigned short port = (unsigned short) config::comm::so_port_ssl();
            // Factory method to make SecureRawSocketConnections.
            // Captures our ssl_context by reference
            const ConnectionListener::RawSocketFactory secure_factory
                = [&](SocketDriver *driver, boost::asio::io_context &io_context) {
                    return new SecureRawSocketConnection(driver, io_context, ssl_context);
                };
            ssl_started = boost::make_shared<ConnectionListener>(
                this,
                io_context,
                boost::asio::ip::tcp::endpoint(address, port),
                secure_factory)->start();

            LOG(info, "socket", "start",
                "Socket Driver started, listening securely on port "
                + text::to_string(port));
        }


        started = (config::comm::so_enable_plain() ? plain_started : true) and
            (config::comm::so_enable_ssl() ? ssl_started : true);

        if (not started)
        {
            LOG(error, "socket", "start", "Socket Driver couldn't start listeners.");
        }

        return started;
    }

    // ----------------------------------------------------------------------
    void SocketDriver::stop(mutgos::comm::RouterSessionManager *router_ptr)
    {
        LOG(info, "socket", "stop", "Socket Driver stopping...");

        if (started)
        {
            // Stop all connections, call do_work() a few times to let them
            // send out the shutdown packet, then exit.
            //
            for (ClientConnections::iterator connection_iter =
                client_connections.begin();
                 connection_iter != client_connections.end();
                 ++connection_iter)
            {
                connection_iter->first->stop();
            }

            for (MG_UnsignedInt count = 0; count < 5; ++count)
            {
                if (not do_work(router_ptr))
                {
                    // Probably done, so exit early.
                    break;
                }
            }

            started = false;
            plain_started = false;
            ssl_started = false;
        }

        io_context.stop();
        LOG(info, "socket", "stop", "Socket Driver stopped");
    }

    // ----------------------------------------------------------------------
    bool SocketDriver::do_work(
        mutgos::comm::RouterSessionManager *router_ptr)
    {
        bool done = true;

        if (router_ptr)
        {
            inside_do_work = true;

            // First, run the IO Context to service anything.
            done = not io_context.poll();

            // Then, service the pending actions.
            for (PendingActions::iterator actions_iter = pending_actions.begin();
                 actions_iter != pending_actions.end();
                 ++actions_iter)
            {
                (*actions_iter)->do_work();
            }

            pending_actions.clear();

            // Finally, delete anything pending.
            for (PendingDeletes::iterator delete_iter = pending_deletes.begin();
                 delete_iter != pending_deletes.end();
                 ++delete_iter)
            {
                delete *delete_iter;
            }

            pending_deletes.clear();

            inside_do_work = false;

            // Add in any deferred requests for pending actions
            //
            if (! pending_actions_deferred.empty())
            {
                for (PendingActions::iterator actions_iter =
                        pending_actions_deferred.begin();
                     actions_iter != pending_actions_deferred.end();
                     ++actions_iter)
                {
                    connection_has_pending_actions(*actions_iter);
                }

                pending_actions_deferred.clear();
            }
        }

        return done;
    }

    // ----------------------------------------------------------------------
    void SocketDriver::release(
        mutgos::comm::ClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            // This is safe because we will only ever do anything with it
            // if found in the set.
            ClientConnections::iterator connection_iter =
                client_connections.find(
                    reinterpret_cast<SocketClientConnection *>(connection_ptr));

            if (connection_iter != client_connections.end())
            {
                --(connection_iter->second);

                if (connection_iter->second <= 0)
                {
                    // No one references it.  Put it in the list of stuff to
                    // delete later, to allow the stack to unwind fully.
                    //
                    pending_deletes.push_back(connection_iter->first);
                    client_connections.erase(connection_iter);
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void SocketDriver::add_reference(SocketClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            ++client_connections[connection_ptr];
        }
    }

    // ----------------------------------------------------------------------
    void SocketDriver::connection_has_pending_actions(
        SocketClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            if (inside_do_work)
            {
                // To avoid iterating through a vector while it is being
                // modified.
                pending_actions_deferred.push_back(connection_ptr);
            }
            else
            {
                pending_actions.push_back(connection_ptr);
            }
        }
    }
}
}
