/*
 * socket_PlainRawSocketConnection.cpp
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
#include "socket_SocketClientConnection.h"

#define SOCKET_SEND_BUFFER_BYTES 8192
#define SOCKET_RECV_BUFFER_BYTES 8192

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    PlainRawSocketConnection::PlainRawSocketConnection(
        mutgos::socket::SocketDriver *driver,
        boost::asio::io_context &io_context)
      : RawSocketConnection(driver, io_context),
        socket(io_context)
    {
    }

    // ----------------------------------------------------------------------
    PlainRawSocketConnection::~PlainRawSocketConnection()
    {
    }

    // ----------------------------------------------------------------------
    void PlainRawSocketConnection::start(void)
    {
        try
        {
            boost::asio::socket_base::send_buffer_size send_buffer(
                SOCKET_SEND_BUFFER_BYTES);
            socket.set_option(send_buffer);
        }
        catch (boost::system::system_error ex)
        {
            LOG(error, "socket", "start",
                "Failed to set socket send buffer size: "
                + std::string(ex.what()));
        }
        catch (...)
        {
            LOG(error, "socket", "start",
                "Failed to set socket send buffer size.");
        }

        try
        {
            boost::asio::socket_base::receive_buffer_size recv_buffer(
                SOCKET_RECV_BUFFER_BYTES);
            socket.set_option(recv_buffer);
        }
        catch (boost::system::system_error ex)
        {
            LOG(error, "socket", "start",
                "Failed to set socket receive buffer size: "
                + std::string(ex.what()));
        }
        catch (...)
        {
            LOG(error, "socket", "start",
                "Failed to set socket receive buffer size.");
        }

        socket_accepted = true;
        socket_connected = true;
        socket_blocked = false;

        do_read();

        if (client_ptr)
        {
            client_ptr->raw_ready();
        }
    }

    // ----------------------------------------------------------------------
    void PlainRawSocketConnection::raw_disconnect(void)
    {
        if (socket_connected)
        {
            if (socket.is_open())
            {
                // Plain sockets can just be shut down the usual way.
                try
                {
                    socket.shutdown(
                        boost::asio::ip::tcp::socket::shutdown_both);
                    socket.close();
                }
                catch (...)
                {
                    LOG(error, "socket", "raw_disconnect",
                        "Failed to properly close socket!");
                }
            }

            handle_disconnect();
        }
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt PlainRawSocketConnection::get_socket_send_buffer_size(void) const
    {
        return SOCKET_SEND_BUFFER_BYTES;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt PlainRawSocketConnection::get_socket_recv_buffer_size(void) const
    {
        return SOCKET_RECV_BUFFER_BYTES;
    }

    // ----------------------------------------------------------------------
    bool PlainRawSocketConnection::raw_is_encrypted(void) const
    {
        // Plain sockets are never encrypted.
        return false;
    }

    // ----------------------------------------------------------------------
    bool PlainRawSocketConnection::raw_send(
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
                    socket,
                    boost::asio::buffer(data_ptr, data_size),
                    boost::bind(
                        &PlainRawSocketConnection::on_write_complete,
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
    void PlainRawSocketConnection::do_read(void)
    {
        socket.async_read_some(
            boost::asio::buffer(incoming_buffer, incoming_buffer_size),
            boost::bind(
                &PlainRawSocketConnection::on_read,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
}
}
