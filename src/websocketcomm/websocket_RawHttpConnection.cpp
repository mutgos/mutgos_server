/*
 * websocket_RawHttpConnection.cpp
 */

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>

#include "logging/log_Logger.h"

#include "websocket_WebsocketDriver.h"
#include "websocket_RawHttpConnection.h"
#include "websocket_RawWSConnection.h"
#include "websocket_WSClientConnection.h"


#define MAX_INCOMING_BUFFER_SIZE 16384
#define MAX_NEGOTIATION_TIME_SECS 30

namespace mutgos
{
namespace websocket
{
    // ----------------------------------------------------------------------
    RawHttpConnection::RawHttpConnection(
        WebsocketDriver *driver,
        boost::asio::ip::tcp::socket socket)
      : driver_ptr(driver),
        socket_connected(true),
        http_socket(std::move(socket)),
        strand_executor(http_socket.get_executor()),
        timer(
            http_socket.get_executor().context(),
            (std::chrono::steady_clock::time_point::max())),
        incoming_buffer(MAX_INCOMING_BUFFER_SIZE)
    {
        if (not driver_ptr)
        {
            LOG(fatal, "websocket", "RawHttpConnection",
                "driver_ptr pointer is null!  Crash will follow...");
        }
    }

    // ----------------------------------------------------------------------
    void RawHttpConnection::start(void)
    {
        // Start the negotiation timer to prevent connections hanging forever,
        // which could also be a DoS attack.
        //
        timer.expires_after(std::chrono::seconds(MAX_NEGOTIATION_TIME_SECS));

        timer.async_wait(
            boost::asio::bind_executor(
                strand_executor,
                std::bind(
                    &RawHttpConnection::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));

        // Start reading the request when it comes in.
        do_read();
    }

    // ----------------------------------------------------------------------
    void RawHttpConnection::do_read(void)
    {
        // Read the request
        boost::beast::http::async_read(http_socket, incoming_buffer, request_info,
            boost::asio::bind_executor(
                strand_executor,
                std::bind(
                    &RawHttpConnection::on_read,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    // ----------------------------------------------------------------------
    void RawHttpConnection::on_timer(boost::system::error_code error_code)
    {
        if ((not error_code) and
            (timer.expiry() <= std::chrono::steady_clock::now()))
        {
            // Timer expired before negotiation finished.  Disconnect.
            //
            LOG(warning, "websocket", "on_timer",
                "Closing HTTP socket that didn't make any requests.");

            do_close();
        }
    }

    // ----------------------------------------------------------------------
    void RawHttpConnection::on_read(boost::system::error_code error_code)
    {
        if (error_code)
        {
            // Socket already closed or some other error.  Cleanup on our end.
            do_close();
        }
        else
        {
            if (boost::beast::websocket::is_upgrade(request_info))
            {
                // Extract the hostname, create the websocket connection
                // classes, and transfer the socket to them.
                //
                timer.cancel();
                const std::string hostname = get_hostname_from_request();

                LOG(info, "websocket", "on_read",
                    "Connection established from " + hostname);

                std::shared_ptr<RawWSConnection> ws_raw =
                    std::make_shared<RawWSConnection>(
                        driver_ptr,
                        std::move(http_socket));

                ws_raw->start(std::move(request_info));
                // Assumption:  The ws_raw connection will add the reference of
                // WSClientConnection to the driver when it gets the
                // pointer, which it should always get.  This is because
                // WSClientConnection tells ws_raw its pointer during
                // construction.
                new WSClientConnection(driver_ptr, ws_raw, hostname);

                // Nothing in here is needed.
                //
                incoming_buffer.consume(incoming_buffer.size());
                incoming_buffer.shrink_to_fit();
            }
            else
            {
                // Tried to make normal HTTP request - not currently supported.
                LOG(warning, "websocket", "on_read",
                    "Client tried to make non-websocket HTTP request.  "
                    "Not supported.");

                do_close();
            }
        }
    }

    // ----------------------------------------------------------------------
    void RawHttpConnection::do_close(void)
    {
        if (socket_connected)
        {
            if (http_socket.is_open())
            {
                try
                {
                    timer.cancel();
                    http_socket.shutdown(
                        boost::asio::ip::tcp::socket::shutdown_both);
                    http_socket.close();
                }
                catch (...)
                {
                    LOG(error, "websocket", "do_close",
                        "Failed to properly close HTTP socket that "
                        "didn't make any (or invalid) requests!");
                }
            }

            socket_connected = false;
        }
    }

    // ----------------------------------------------------------------------
    // This is designed to be used with the special WS-proxy settings in
    // Apache, though other proxying servers should work too if they
    // follow the format:  Format is ws:/..../hostname/ip
    std::string RawHttpConnection::get_hostname_from_request(void)
    {
        std::string result;
        const std::string url = request_info.target().to_string();

        if (url.empty())
        {
            LOG(error, "websocket", "get_hostname_from_request",
                "Failed to get hostname from request!");
        }
        else
        {
            size_t end = url.size() - 1;

            if (url[end] == '/')
            {
                // Skip trailing /
                end--;
            }

            const size_t ip_pos = url.find_last_of('/', end);

            if (ip_pos != std::string::npos)
            {
                const std::string ip_addr =
                    url.substr(ip_pos + 1, end - ip_pos + 1);
                const size_t name_pos = url.find_last_of('/', ip_pos - 1);

                if (name_pos == std::string::npos)
                {
                    result = ip_addr;
                }
                else
                {
                    const std::string hostname_addr =
                        url.substr(name_pos + 1, ip_pos - name_pos - 1);

                    if (not hostname_addr.empty())
                    {
                        result = hostname_addr + "  (" + ip_addr + ")";
                    }
                    else
                    {
                        result = ip_addr;
                    }
                }
            }
        }

        if (result.empty())
        {
            result = "UNKNOWN";
        }

        return result;
    }
}
}