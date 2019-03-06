/*
 * socket_SecureRawSocketConnection.cpp
 */

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/system_error.hpp>

#include "logging/log_Logger.h"

#include "socket_RawSocketConnection.h"
#include "socket_PlainRawSocketConnection.h"
#include "socket_SecureRawSocketConnection.h"
#include "socket_SocketClientConnection.h"

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    SecureRawSocketConnection::SecureRawSocketConnection(
        mutgos::socket::SocketDriver *driver,
        boost::asio::io_context &io_context,
        boost::asio::ssl::context &ssl_context)
      : PlainRawSocketConnection(driver, io_context),
        ssl_socket(this->get_socket(), ssl_context)
    {
    }

    // ----------------------------------------------------------------------
    SecureRawSocketConnection::~SecureRawSocketConnection()
    {
    }

    // ----------------------------------------------------------------------
    void SecureRawSocketConnection::start(void)
    {
        // Perform the handshake then let our base class configure 
        // the settings. This means that socket options will only be
        // set after the handshake.
        ssl_socket.async_handshake(
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>::server,
            boost::bind(
                &SecureRawSocketConnection::on_handshake_complete,
                boost::static_pointer_cast<SecureRawSocketConnection>(shared_from_this()),
                boost::asio::placeholders::error));
    }

    // ----------------------------------------------------------------------
    void SecureRawSocketConnection::on_handshake_complete(boost::system::error_code error_code)
    {
        if (error_code)
        {
            // Handshake failed. Shut it down.
            LOG(error, "socket", "on_handshake_complete", "Bad handshake from " +
                get_socket().remote_endpoint().address().to_string() +
                ": " + error_code.message());
            raw_disconnect();
        } else {
            // Continue setting socket options etc.
            PlainRawSocketConnection::start();
        }
    }

    // ----------------------------------------------------------------------
    void SecureRawSocketConnection::raw_disconnect(void)
    {
        if (socket_connected)
        {
            if (ssl_socket.lowest_layer().is_open())
            {
                // The proper way to close a secure socket is shutting down the 
                // SSL layer and then the TCP/IP one.
                try
                {
                    ssl_socket.shutdown();
                    ssl_socket.lowest_layer().shutdown(
                        boost::asio::ip::tcp::socket::shutdown_both);
                    ssl_socket.lowest_layer().close();
                }
                catch (...)
                {
                    LOG(error, "socket", "raw_disconnect",
                        "Failed to properly close secure socket!");
                }
            }

            handle_disconnect();
        }
    }

    // ----------------------------------------------------------------------
    bool SecureRawSocketConnection::raw_is_encrypted(void) const
    {
        return true;
    }

    // ----------------------------------------------------------------------
    bool SecureRawSocketConnection::raw_send(
        const char *data_ptr,
        const size_t data_size)
    {
        bool success = false;

        if (socket_accepted and socket_connected and (not socket_blocked))
        {
            if (not data_size)
            {
                success = true;
            }
            else if (data_ptr)
            {
                boost::asio::async_write(
                    ssl_socket,
                    boost::asio::buffer(data_ptr, data_size),
                    boost::bind(
                        &SecureRawSocketConnection::on_write_complete,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

                success = true;
                socket_blocked = true;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void SecureRawSocketConnection::do_read(void)
    {
        ssl_socket.async_read_some(
            boost::asio::buffer(incoming_buffer, incoming_buffer_size),
            boost::bind(
                &SecureRawSocketConnection::on_read,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
}
}
