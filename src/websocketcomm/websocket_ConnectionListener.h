/*
 * websocket_ConnectionListener.h
 */

#ifndef MUTGOS_WEBSOCKET_CONNECTIONLISTENER_H
#define MUTGOS_WEBSOCKET_CONNECTIONLISTENER_H

#include <boost/asio/ip/tcp.hpp>


namespace mutgos
{
namespace websocket
{
    // Forward declarations.
    //
    class WebsocketDriver;


    /**
     * Listens for and accepts sockets, passing newly accepted sockets on to
     * a RawHttpConnection.
     *
     * Most of the beast-specific code is heavily inspired from beast examples
     * as much as possible.
     *
     * This is not multi-thread safe.
     */
    class ConnectionListener
        : public std::enable_shared_from_this<ConnectionListener>
    {
    public:
        /**
         * Constructor.
         * @param driver[in] Pointer to the WebsocketDriver in use.
         * @param context[in] The IO Context.
         * @param endpoint[in] What we are listening on.
         */
        ConnectionListener(
            WebsocketDriver *driver,
            boost::asio::io_context &context,
            boost::asio::ip::tcp::endpoint endpoint);

        /**
         * Starts listening for connection requests.
         * @return True if successfully started.
         */
        bool start(void);


    private:
        /**
         * Listens for the next connection request.
         */
        void do_accept(void);

        /**
         * Called when a connection request has been made.
         * @param error_code[in] The error code associated with the request.
         */
        void on_accept(boost::system::error_code error_code);


        bool initialization_error; ///< True if error initializing.

        WebsocketDriver * const driver_ptr; ///< Pointer to the driver.

        boost::asio::ip::tcp::acceptor socket_acceptor; ///< Listens/accepts socket connections
        boost::asio::ip::tcp::socket socket; ///< The socket currently being accepted
    };
}
}

#endif //MUTGOS_WEBSOCKET_CONNECTIONLISTENER_H
