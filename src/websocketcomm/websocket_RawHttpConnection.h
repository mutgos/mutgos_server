/*
 * websocket_RawHttpConnection.h
 */

#ifndef MUTGOS_WEBSOCKET_RAWHTTPCONNECTION_H
#define MUTGOS_WEBSOCKET_RAWHTTPCONNECTION_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace mutgos
{
namespace websocket
{
    // Forward declarations.
    //
    class WebsocketDriver;

    /**
     * Represents the initial HTTP(s) connection that comes in and requests
     * to be upgraded to a websocket.  It has just enough functionality to
     * parse the URL and create a WSClientConnection and associated
     * RawWSConnection.
     *
     * Most of the beast-specific code is heavily inspired from beast examples
     * as much as possible.
     *
     * This is not multi-thread safe.
     */
    class RawHttpConnection :
        public std::enable_shared_from_this<RawHttpConnection>
    {
    public:
        /**
         * Creates a RawHttpConnection.
         * @param driver[in] Pointer to websocket driver.
         * @param socket[in] The raw socket associated with the connection.
         */
        explicit RawHttpConnection(
            WebsocketDriver *driver,
            boost::asio::ip::tcp::socket socket);

        /**
         * Initiates associating the socket to this RawHttpConnection
         * (via beast).
         * This must be called before calling any other method and can
         * only be called once.
         */
        void start(void);

    private:

        /**
         * Initiates the process of reading the HTTP request and starting
         * websocket negotiation.
         */
        void do_read(void);

        /**
         * Called when the timer expires, this will disconnect the socket
         * if the time has elapsed and it has not attempted to become a
         * websocket.
         * @param error_code[in] Indicates if the timer completed successfully.
         */
        void on_timer(boost::system::error_code error_code);

        /**
         * Called when a full HTTP request has come in, this will extract
         * the originating host and transfer the socket to a RawWSConnection
         * and WSClientConnection.
         * @param error_code[in] Indicates if the read completed successfully.
         */
        void on_read(boost::system::error_code error_code);

        /**
         * Forcibly closes the socket.
         */
        void do_close(void);

        /**
         * Given an active request (stored in request_info), extract the
         * hostname and return it.
         * @return The hostname which made the request.
         */
        std::string get_hostname_from_request(void);


        WebsocketDriver * const driver_ptr; ///< Pointer to the driver.

        bool socket_connected; ///< True if currently connected.

        boost::asio::ip::tcp::socket http_socket; ///< The HTTP socket
        boost::asio::strand<boost::asio::executor> strand_executor; ///< The executor in use
        boost::asio::steady_timer timer; ///< Timer used to detect lack of activity
        boost::beast::flat_buffer incoming_buffer; ///< Buffer to hold the incoming request
        boost::beast::http::request<boost::beast::http::string_body>
            request_info; ///< Parsed incoming request data
    };
}
}

#endif //MUTGOS_WEBSOCKET_RAWHTTPCONNECTION_H
