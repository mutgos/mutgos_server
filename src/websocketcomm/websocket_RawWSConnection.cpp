/*
 * websocket_RawConnection.cpp
 */

#include <memory>
#include <string.h>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>

#include "comminterface/comm_ClientConnection.h"

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"

#include "websocket_WSClientConnection.h"
#include "websocket_RawWSConnection.h"
#include "websocket_WebsocketDriver.h"

namespace mutgos
{
namespace websocket
{
    // ----------------------------------------------------------------------
    RawWSConnection::RawWSConnection(
        WebsocketDriver *driver,
        boost::asio::ip::tcp::socket socket)
      : socket_accepted(false),
        socket_blocked(false),
        socket_connected(true),
        driver_ptr(driver),
        client_ptr(0),
        web_socket(std::move(socket)),
        strand_executor(web_socket.get_executor()),
        timer(
            web_socket.get_executor().context(),
            (std::chrono::steady_clock::time_point::max())),
        incoming_buffer(config::comm::ws_max_incoming_message_size()),
        outgoing_buffer()
    {
        if (not driver_ptr)
        {
            LOG(fatal, "websocket", "RawWSConnection",
                "driver_ptr pointer is null!  Crash will follow...");
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::set_client(
        mutgos::websocket::WSClientConnection *client)
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

            if (need_read)
            {
                // Socket was accepted before this method was called, so trigger
                // read.
                boost::asio::post(
                    strand_executor,
                    std::bind(&RawWSConnection::do_read, shared_from_this()));
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::set_timer(const MG_UnsignedInt seconds)
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
                        &RawWSConnection::on_timer,
                        shared_from_this(),
                        std::placeholders::_1)));
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::cancel_timer(void)
    {
        timer.cancel();
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::client_released(void)
    {
        if (client_ptr)
        {
            driver_ptr->release(client_ptr);
            client_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::raw_disconnect(void)
    {
        if (socket_connected)
        {
            if (web_socket.next_layer().is_open())
            {
                if (socket_accepted)
                {
                    web_socket.async_close(
                        "raw_disconnect()",
                        boost::asio::bind_executor(
                            strand_executor,
                            std::bind(
                                &RawWSConnection::on_disconnect_complete,
                                shared_from_this(),
                                std::placeholders::_1)));
                }
                else
                {
                    try
                    {
                        web_socket.next_layer().shutdown(
                            boost::asio::ip::tcp::socket::shutdown_both);
                        web_socket.next_layer().close();
                    }
                    catch (...)
                    {
                        LOG(error, "websocket", "raw_disconnect",
                            "Failed to properly close websocket that "
                            "didn't negotiate!");
                    }
                }
            }

            handle_disconnect();
        }
    }

    // ----------------------------------------------------------------------
    bool RawWSConnection::raw_send(const char *data_ptr, const size_t data_size)
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
                try
                {
                    boost::asio::mutable_buffer buffer =
                        outgoing_buffer.prepare(data_size);

                    memcpy(buffer.data(), data_ptr, data_size);

                    web_socket.text(true);
                    web_socket.async_write(
                        buffer,
                        boost::asio::bind_executor(
                            strand_executor,
                            std::bind(
                                &RawWSConnection::on_write_complete,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2)));
                    success = true;
                    socket_blocked = true;
                }
                catch (...)
                {
                    LOG(error, "websocket", "raw_send",
                        "Unexpected exception thrown when prepping to send.");
                }
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::on_accept(boost::system::error_code error_code)
    {
        if (socket_connected)
        {
            if (not error_code)
            {
                socket_accepted = true;

                // We can only read something if we have a client, othewise
                // it'd have nowhere to go.
                //
                if (client_ptr)
                {
                    boost::asio::post(
                        strand_executor,
                        std::bind(&RawWSConnection::do_read, shared_from_this()));
                }
            }
            else
            {
                // Failed to accept/connect fully
                //
                LOG(error, "websocket", "on_accept",
                    "Failed to accept connection: " + error_code.message());

                raw_disconnect();
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::do_read(void)
    {
        // Read a message into our buffer
        web_socket.async_read(
            incoming_buffer,
            boost::asio::bind_executor(
                strand_executor,
                std::bind(
                    &RawWSConnection::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::on_read(
        boost::system::error_code error_code,
        std::size_t bytes_transferred)
    {
        if (error_code)
        {
            // Socket was closed.  Inform client if disconnection was not
            // requested.
            handle_disconnect();
        }
        else if (bytes_transferred ==
            config::comm::ws_max_incoming_message_size())
        {
            LOG(warning, "websocket", "on_read",
                "Client exceeded incoming buffer.");

            // We've probably overflowed.  Disconnect.
            raw_disconnect();
        }
        else if (client_ptr)
        {
            // Copy the data to a mutable char *, then pass it to the client.
            //
            char * const raw_data_ptr = new char[bytes_transferred + 1];
            memcpy(
                raw_data_ptr,
                incoming_buffer.data().data(),
                bytes_transferred);
            raw_data_ptr[bytes_transferred] = '\0';

            client_ptr->raw_data(raw_data_ptr, bytes_transferred);
            incoming_buffer.consume(bytes_transferred);
            do_read();
        }
        else
        {
            LOG(error, "websocket", "on_read",
                "Got data from client with no registered WSClientConnection!");

            // This should never happen, but it keeps the system moving.
            incoming_buffer.consume(bytes_transferred);
            do_read();
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::on_write_complete(
        boost::system::error_code error_code,
        std::size_t bytes_transferred)
    {
        outgoing_buffer.consume(outgoing_buffer.size());

        if (outgoing_buffer.capacity() >
            config::comm::ws_max_incoming_message_size())
        {
            // Don't want this to stay too big.
            outgoing_buffer.shrink_to_fit();
        }

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
    void RawWSConnection::on_disconnect_complete(
        boost::system::error_code error_code)
    {
        // Currently we don't care about the result.
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::on_timer(boost::system::error_code error_code)
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
                // Indicates failure to negotiate websocket connection.
                // Disconnect since something is wrong.
                //
                LOG(warning, "websocket", "on_timer", "Failed to properly "
                    "negotiate websocket; closing.");
                raw_disconnect();
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawWSConnection::handle_disconnect(void)
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
