/*
 * websocket_WebsocketDriver.cpp
 */

#include <boost/asio/ip/tcp.hpp>


#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_StringConversion.h"

#include "websocket_WebsocketDriver.h"
#include "websocket_WSClientConnection.h"
#include "websocket_ConnectionListener.h"

namespace mutgos
{
namespace websocket
{
    // ----------------------------------------------------------------------
    WebsocketDriver::WebsocketDriver(
        mutgos::comm::RouterSessionManager *router)
      : my_router_ptr(router),
        io_context{1},
        started(false)
    {
        if (not my_router_ptr)
        {
            LOG(fatal, "websocket", "WebsocketDriver", "router is null!");
        }
    }

    // ----------------------------------------------------------------------
    WebsocketDriver::~WebsocketDriver()
    {
        if (started)
        {
            LOG(error, "websocket", "~WebsocketDriver",
                "Destructed without calling stop()!");
        }

        if (not client_connections.empty())
        {
            LOG(error, "websocket", "~WebsocketDriver",
                "Client connections still instantiated!");
        }
    }

    // ----------------------------------------------------------------------
    // This assumes it's being run behind a websocket proxy.
    bool WebsocketDriver::start(void)
    {
        if (not started)
        {
            // Taken from 'advanced_server.cpp' in beast examples.
            // Makes the IO context, and creates and starts the connection
            // listener.
            //
            const boost::asio::ip::address address =
                boost::asio::ip::make_address("127.0.0.1");
            const unsigned short port =
                (unsigned short) config::comm::ws_port();


            started = std::make_shared<ConnectionListener>(
                this,
                io_context,
                boost::asio::ip::tcp::endpoint{address, port})->start();

            LOG(info, "websocket", "start",
                "Websocket Driver started, listening on port "
                + text::to_string(port));
        }

        return started;
    }

    // ----------------------------------------------------------------------
    void WebsocketDriver::stop(comm::RouterSessionManager *router_ptr)
    {
        if (started)
        {
            LOG(info, "websocket", "stop", "Websocket Driver stopping...");

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

            io_context.stop();

            started = false;
            LOG(info, "websocket", "stop", "Websocket Driver stopped");
        }
    }

    // ----------------------------------------------------------------------
    bool WebsocketDriver::do_work(
        mutgos::comm::RouterSessionManager *router_ptr)
    {
        bool done = true;

        if (router_ptr)
        {
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
        }

        return done;
    }

    // ----------------------------------------------------------------------
    void WebsocketDriver::release(
        mutgos::comm::ClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            // This is safe because we will only ever do anything with it
            // if found in the set.
            ClientConnections::iterator connection_iter =
                client_connections.find(
                    reinterpret_cast<WSClientConnection *>(connection_ptr));

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
    void WebsocketDriver::add_reference(WSClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            ++client_connections[connection_ptr];
        }
    }

    // ----------------------------------------------------------------------
    void WebsocketDriver::connection_has_pending_actions(
        mutgos::websocket::WSClientConnection *connection_ptr)
    {
        if (connection_ptr)
        {
            pending_actions.push_back(connection_ptr);
        }
    }
}
}
