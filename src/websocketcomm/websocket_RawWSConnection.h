/*
 * websocket_RawWSConnection.h
 */

#ifndef MUTGOS_WEBSOCKET_RAWWSCONNECTION_H
#define MUTGOS_WEBSOCKET_RAWWSCONNECTION_H

#include <memory>
#include <stddef.h>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace websocket
{
    // Forward declarations
    //
    class WSClientConnection;
    class WebsocketDriver;

    // TODO Add ping()

    /**
     * Represents the actual websocket connection that directly interfaces with
     * the websocket library.
     *
     * This is plaintext only.  Encryption is done at the webserver proxy
     * layer, outside of MUTGOS.
     *
     * Most of the beast-specific code is heavily inspired from beast examples
     * as much as possible.
     *
     * This is not multi-thread safe.
     */
    class RawWSConnection :
        public std::enable_shared_from_this<RawWSConnection>
    {
    public:
        /**
         * Creates a RawWSConnection.
         * @param driver[in] Pointer to the websocket driver in use.
         * @param socket[in] The raw socket associated with the connection.
         */
        explicit RawWSConnection(
            WebsocketDriver *driver,
            boost::asio::ip::tcp::socket socket);

        /**
         * Initiates associating the socket to a websocket (via beast).
         * This must be called before calling any other method and can
         * only be called once.
         * @param request[in] The original upgrade request as read by the HTTP
         * server.
         */
        template<class Body, class Allocator>
        void start(
            boost::beast::http::request<
                Body,
                boost::beast::http::basic_fields<Allocator>> request)
        {
            // Accept the websocket
            web_socket.async_accept(
                request,
                boost::asio::bind_executor(
                    strand_executor,
                    std::bind(
                        &RawWSConnection::on_accept,
                        shared_from_this(),
                        std::placeholders::_1)));
        };

        /**
         * When known, sets the pointer to the client connection.
         * Pointer will not be owned by class, but do not delete the pointer
         * without first called client_released();
         * @param client[in] Pointer to the client connection.
         */
        void set_client(WSClientConnection *client);

        /**
         * Sets the timer to expire after the provided number of seconds.
         * Any existing timer will be cancelled.  Can only be used when
         * connected.
         * On the WSClientConnection, raw_timer_expired() will be called when
         * the specified time has passed.
         * @param seconds[in] The seconds to set the timer for.  If 0, it
         * will expire at some point in the near future.
         */
        void set_timer(const MG_UnsignedInt seconds);

        /**
         * Cancels the timer set with set_timer(), if it is still pending.
         */
        void cancel_timer(void);

        /**
         * Informs the class that the client connection pointer has been
         * deleted, so it will no longer be used.
         */
        void client_released(void);

        /**
         * Closes the websocket immediately, but cleanly.
         */
        void raw_disconnect(void);

        /**
         * @return True if socket is connected.
         */
        bool raw_is_connected(void) const
        { return socket_connected; }

        /**
         * @return True if socket is blocked on send.
         */
        bool raw_is_blocked(void) const
        { return socket_blocked; }

        /**
         * Sends the given data over the websocket.
         * @param data_ptr[in] The data to send.  The data itself will be
         * copied.
         * @param data_size[in] The size of the data.
         * @return True if send has started, false if send did NOT start
         * because it was blocked, not connected, or had some other error.
         */
        bool raw_send(const char *data_ptr, const size_t data_size);

    private:
        /**
         * Called after beast has finished initial processing of the socket,
         * which converts it to a websocket.
         * @param error_code[in] If set, indicates beast had an error
         * negotiating the socket into a websocket.
         */
        void on_accept(boost::system::error_code error_code);

        /**
         * Called to asynchronously request any waiting data from the
         * websocket.
         */
        void do_read(void);

        /**
         * Called when the buffer has data in it, from a call to do_read().
         * This will also send the resulting buffer to client_ptr.
         * @param error_code[in] If set, indicates beast had an error getting
         * data.
         * @param bytes_transferred[in] The number of bytes transferred from
         * the websocket.
         */
        void on_read(
            boost::system::error_code error_code,
            std::size_t bytes_transferred);

        /**
         * Called when the current write to the websocket has completed.
         * @param error_code[in] If set, indicates beast had an error sending
         * data.
         * @param bytes_transferred[in] The number of bytes transferred to
         * the websocket.
         */
        void on_write_complete(
            boost::system::error_code error_code,
            std::size_t bytes_transferred);

        /**
         * Called when a forced disconnect completes.
         * @param error_code[in] Result of disconnect.
         */
        void on_disconnect_complete(boost::system::error_code error_code);

        /**
         * Called when the timer has expired, cancelled, etc.
         * @param error_code[in] Indicates if the timer completed successfully.
         */
        void on_timer(boost::system::error_code error_code);

        /**
         * Used when beast indicates the websocket closed during an operation.
         */
        void handle_disconnect(void);


        bool socket_accepted; ///< True if accepted (valid websocket).
        bool socket_blocked; ///< True if blocked on send.
        bool socket_connected; ///< True if socket connected.

        WebsocketDriver * const driver_ptr; ///< Pointer to driver.

        WSClientConnection *client_ptr; ///< Pointer to client connection
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>
            web_socket; ///< The web socket
        boost::asio::strand<boost::asio::executor> strand_executor; ///< The executor
        boost::asio::steady_timer timer; ///< General timer.  Put here to keep all beast/asio stuff in one place.

        boost::beast::flat_buffer incoming_buffer; ///< Buffers incoming data
        boost::beast::flat_buffer outgoing_buffer; ///< Buffers outgoing data
    };
}
}

#endif //MUTGOS_WEBSOCKET_RAWWSCONNECTION_H
