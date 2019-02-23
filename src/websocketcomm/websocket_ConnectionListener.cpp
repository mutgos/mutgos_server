/*
 * websocket_ConnectionListener.cpp
 */

#include <boost/asio/ip/tcp.hpp>

#include "logging/log_Logger.h"

#include "websocket_WebsocketDriver.h"
#include "websocket_RawHttpConnection.h"
#include "websocket_ConnectionListener.h"

namespace mutgos
{
namespace websocket
{
    // ----------------------------------------------------------------------
    ConnectionListener::ConnectionListener(
        mutgos::websocket::WebsocketDriver *driver,
        boost::asio::io_context &context,
        boost::asio::ip::tcp::endpoint endpoint)
      : initialization_error(false),
        driver_ptr(driver),
        socket_acceptor(context),
        socket(context)
    {
        if (not driver_ptr)
        {
            LOG(fatal, "websocket", "ConnectionListener",
                "driver_ptr pointer is null!  Crash will follow...");
        }

        boost::system::error_code error_code;

        // Open the acceptor
        //
        socket_acceptor.open(endpoint.protocol(), error_code);

        if (error_code)
        {
            LOG(error, "websocket", "ConnectionListener",
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
            LOG(error, "websocket", "ConnectionListener",
                "Failed to set reuse_address option to true.");
            initialization_error = true;
            return;
        }

        // Bind to the server address
        //
        socket_acceptor.bind(endpoint, error_code);

        if(error_code)
        {
            LOG(error, "websocket", "ConnectionListener",
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
            LOG(error, "websocket", "ConnectionListener",
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
                LOG(error, "websocket", "start",
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

    // ----------------------------------------------------------------------
    void ConnectionListener::do_accept(void)
    {
        socket_acceptor.async_accept(
            socket,
            std::bind(
                &ConnectionListener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }

    // ----------------------------------------------------------------------
    void ConnectionListener::on_accept(boost::system::error_code error_code)
    {
        if (error_code)
        {
            LOG(error, "websocket", "on_accept",
                "Could not accept connection due to error: "
                + error_code.message());
        }
        else
        {
            // Create the http session and pass socket over for processing.
            std::make_shared<RawHttpConnection>(
                driver_ptr,
                std::move(socket))->start();
        }

        // Accept the next connection request.
        do_accept();
    }
}
}