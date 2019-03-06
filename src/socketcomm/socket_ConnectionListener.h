/*
 * socket_ConnectionListener.h
 */

#ifndef MUTGOS_SOCKET_CONNECTIONLISTENER_H
#define MUTGOS_SOCKET_CONNECTIONLISTENER_H

#include <boost/shared_ptr.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

#include "socket_RawSocketConnection.h"

namespace mutgos
{
namespace socket
{
    // Forward declarations.
    //
    class SocketDriver;


    /**
     * Listens for and accepts sockets, passing newly accepted sockets on to
     * a RawSocketConnection.
     *
     * Most of the ASIO-specific code is heavily inspired from beast and ASIO
     * examples as much as possible.
     *
     * This is not multi-thread safe.
     */
    class ConnectionListener
        : public boost::enable_shared_from_this<ConnectionListener>
    {
    public:
        typedef std::function<
            RawSocketConnection*(
                mutgos::socket::SocketDriver*,
                boost::asio::io_context&
            )
        > RawSocketFactory;

        /**
         * Constructor.
         * @param driver[in] Pointer to the SocketDriver in use.
         * @param context[in] The IO Context.
         * @param endpoint[in] What we are listening on.
         * @param socket_factory[in] Method that will produce PlainRawSocketConnections.
         */
        ConnectionListener(
            SocketDriver *driver,
            boost::asio::io_context &context,
            boost::asio::ip::tcp::endpoint endpoint,
            RawSocketFactory socket_factory);

        /**
         * Starts listening for connection requests.
         * @return True if successfully started.
         */
        bool start(void);


    private:
        typedef boost::shared_ptr<RawSocketConnection> RawSocketPtr;

        /**
         * Listens for the next connection request.
         */
        void do_accept(void);

        /**
         * Called when a connection request has been made.
         * @param error_code[in] The error code associated with the request.
         */
        void on_accept(
            RawSocketPtr connection,
            boost::system::error_code error_code);


        bool initialization_error; ///< True if error initializing.

        SocketDriver * const driver_ptr; ///< Pointer to the driver.

        boost::asio::ip::tcp::acceptor socket_acceptor; ///< Listens/accepts socket connections
        boost::asio::io_context &io_context; ///< IO Context for everything
        RawSocketFactory socket_factory; ///< Used to produce new connection objects
    };
}
}

#endif //MUTGOS_SOCKET_CONNECTIONLISTENER_H
