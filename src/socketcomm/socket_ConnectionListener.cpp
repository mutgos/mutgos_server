/*
 * socket_ConnectionListener.cpp
 */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include "logging/log_Logger.h"

#include "socket_SocketDriver.h"
#include "socket_PlainRawSocketConnection.h"
#include "socket_ConnectionListener.h"
#include "socket_SocketClientConnection.h"

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    ConnectionListener::ConnectionListener(
        mutgos::socket::SocketDriver *driver,
        boost::asio::io_context &context,
        boost::asio::ip::tcp::endpoint endpoint)
      : initialization_error(false),
        driver_ptr(driver),
        socket_acceptor(context),
        io_context(context),
        socket(context),
        ssl_context(boost::asio::ssl::context::tlsv12_server)
    {
        if (not driver_ptr)
        {
            LOG(fatal, "socket", "ConnectionListener",
                "driver_ptr pointer is null!  Crash will follow...");
        }

        boost::system::error_code error_code;

        // Set up the TLS context
        //
        // This may be too stringent for some clients, in which case we can relax it later.
        ssl_context.set_options(boost::asio::ssl::context::no_tlsv1);
        // TODO(hyena): Support password callbacks for the certificate?
        ssl_context.use_certificate_chain_file("server.pem");
        ssl_context.use_private_key_file("server.pem", boost::asio::ssl::context::pem);

        // Open the acceptor
        //
        socket_acceptor.open(endpoint.protocol(), error_code);

        if (error_code)
        {
            LOG(error, "socket", "ConnectionListener",
                "Failed to open acceptor due to error: "
                + error_code.message());
            initialization_error = true;
            return;
        }

        // Allow address reuse
        //
        try
        {
            socket_acceptor.set_option(
                boost::asio::socket_base::reuse_address(true));
        }
        catch (...)
        {
            LOG(error, "socket", "ConnectionListener",
                "Failed to set reuse_address option to true.");
            initialization_error = true;
            return;
        }

        // Bind to the server address
        //
        socket_acceptor.bind(endpoint, error_code);

        if(error_code)
        {
            LOG(error, "socket", "ConnectionListener",
                "Failed to bind acceptor due to error: "
                + error_code.message());
            initialization_error = true;
            return;
        }

        // Start listening for connections
        //
        socket_acceptor.listen(
            boost::asio::socket_base::max_listen_connections, error_code);

        if(error_code)
        {
            LOG(error, "socket", "ConnectionListener",
                "Failed to start acceptor listen due to error: "
                + error_code.message());
            initialization_error = true;
            return;
        }
    }

    // ----------------------------------------------------------------------
    bool ConnectionListener::start(void)
    {
        if (initialization_error)
        {
            return false;
        }
        else
        {
            if (not socket_acceptor.is_open())
            {
                LOG(error, "socket", "start",
                    "Acceptor is not open; unable to accept new connections.");
                initialization_error = true;
            }
            else
            {
                do_accept();
            }
        }

        return not initialization_error;
    }

    // ----------   ------------------------------------------------------------
    void ConnectionListener::do_accept(void)
    {
        SecureRawSocketPtr new_connection(new SecureRawSocketConnection(
                driver_ptr,
                io_context,
                ssl_context));

        socket_acceptor.async_accept(
            new_connection->get_socket(),
            boost::bind(
                &ConnectionListener::on_accept,
                shared_from_this(),
                new_connection,
                boost::asio::placeholders::error));
    }

    // ----------------------------------------------------------------------
    void ConnectionListener::on_accept(
        SecureRawSocketPtr connection,
        boost::system::error_code error_code)
    {
        if (error_code)
        {
            LOG(error, "socket", "on_accept",
                "Could not accept connection due to error: "
                + error_code.message());
        }
        else
        {
            std::string source = "UNKNOWN";

            try
            {
                source = connection->get_socket().remote_endpoint().
                    address().to_string();
            }
            catch (boost::system::system_error &ex)
            {
                LOG(error, "socket", "on_accept",
                    "Failed to get remote IP address due to exception: "
                    + std::string(ex.what()));
            }
            catch (...)
            {
                LOG(error, "socket", "on_accept",
                    "Failed to get remote IP address due to unknown exception");
            }

            connection->start();
            new SocketClientConnection(driver_ptr, connection, source);
        }

        // Accept the next connection request.
        do_accept();
    }
}
}