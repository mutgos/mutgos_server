/*
 * socket_RawSocketConnection.cpp
 */

#include <memory>
#include <string.h>

#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>

#include "comminterface/comm_ClientConnection.h"

#include "logging/log_Logger.h"

#include "socket_RawSocketConnection.h"
#include "socket_SocketDriver.h"
#include "socket_SocketClientConnection.h"

#define MAX_INCOMING_BUFFER_SIZE 8192

namespace mutgos
{
namespace socket
{
    // Static initializers
    //
    const size_t RawSocketConnection::incoming_buffer_size =
        MAX_INCOMING_BUFFER_SIZE;


    // ----------------------------------------------------------------------
    RawSocketConnection::RawSocketConnection(
        mutgos::socket::SocketDriver *driver,
        boost::asio::io_context &io_context)
        : socket_accepted(false),
          socket_blocked(true),
          socket_connected(false),
          driver_ptr(driver),
          client_ptr(0),
          strand_executor(io_context.get_executor()),
          timer(
              io_context,
              (std::chrono::steady_clock::time_point::max()))
    {
        if (not driver_ptr)
        {
            LOG(fatal, "socket", "RawSocketConnection",
                "driver_ptr pointer is null!  Crash will follow...");
        }

        incoming_buffer = new char[MAX_INCOMING_BUFFER_SIZE];
        memset(incoming_buffer, 0, MAX_INCOMING_BUFFER_SIZE);
    }

    // ----------------------------------------------------------------------
    RawSocketConnection::~RawSocketConnection()
    {
        delete[] incoming_buffer;
        incoming_buffer = 0;
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::set_client(SocketClientConnection *client)
    {
        const bool need_read = (not client_ptr) and socket_accepted;

        if (client_ptr)
        {
            client_released();
        }

        if (client)
        {
            client_ptr = client;
            driver_ptr->add_reference(client_ptr);

            if (socket_accepted)
            {
                client_ptr->raw_ready();
            }

            if (need_read)
            {
                // Socket was accepted before this method was called, so trigger
                // read.
                boost::asio::post(
                    strand_executor,
                    std::bind(&RawSocketConnection::do_read, shared_from_this()));
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::set_timer(const MG_UnsignedInt seconds)
    {
        if (socket_connected)
        {
            if (seconds)
            {
                timer.expires_after(std::chrono::seconds(seconds));
            }
            else
            {
                timer.expires_after(std::chrono::milliseconds(100));
            }

            timer.async_wait(
                boost::asio::bind_executor(
                    strand_executor,
                    std::bind(
                        &RawSocketConnection::on_timer,
                        shared_from_this(),
                        std::placeholders::_1)));
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::cancel_timer(void)
    {
        timer.cancel();
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::client_released(void)
    {
        if (client_ptr)
        {
            driver_ptr->release(client_ptr);
            client_ptr = 0;

            raw_disconnect();
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::on_read(
        boost::system::error_code error_code,
        std::size_t bytes_transferred)
    {
        if (error_code)
        {
            // Socket was closed.  Inform client if disconnection was not
            // requested.
            handle_disconnect();
        }
        else if (client_ptr)
        {
            // Pass read data to the client.
            //
            client_ptr->raw_data(incoming_buffer, bytes_transferred);
            do_read();
        }
        else
        {
            LOG(error, "socket", "on_read",
                "Got data from client with no registered SocketClientConnection!");

            // This should never happen, but it keeps the system moving.
            do_read();
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::on_write_complete(
        boost::system::error_code error_code,
        std::size_t bytes_transferred)
    {
        if (error_code)
        {
            // Socket was closed.  Inform client if disconnection was not
            // requested.
            handle_disconnect();
        }
        else
        {
            // Write completed successfully.
            socket_blocked = false;

            if (client_ptr)
            {
                client_ptr->raw_send_complete();
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::on_timer(boost::system::error_code error_code)
    {
        if (socket_connected and (not error_code) and
            (timer.expiry() <= std::chrono::steady_clock::now()))
        {
            if (socket_accepted)
            {
                // Normal timer expiration
                //
                if (client_ptr)
                {
                    client_ptr->raw_timer_expired();
                }
            }
            else
            {
                // Indicates failure to negotiate connection.
                // Disconnect since something is wrong.
                //
                LOG(warning, "socket", "on_timer", "Failed to properly "
                    "negotiate socket; closing.");
                raw_disconnect();
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawSocketConnection::handle_disconnect(void)
    {
        if (socket_connected)
        {
            socket_connected = false;
            socket_accepted = false;
            socket_blocked = true;

            cancel_timer();

            if (client_ptr)
            {
                client_ptr->raw_disconnected();
                client_released();
            }
        }
    }
}
}
