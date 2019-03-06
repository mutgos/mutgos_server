/*
 * socket_PlainRawSocketConnection.h
 */

#ifndef MUTGOS_SOCKET_PLAINRAWSOCKETCONNECTION_H
#define MUTGOS_SOCKET_PLAINRAWSOCKETCONNECTION_H

#include <boost/asio/ip/tcp.hpp>

#include "socket_RawSocketConnection.h"

namespace mutgos
{
namespace socket
{
    /**
     * Concrete implementation of an unencrypted raw socket connection.
     */
    class PlainRawSocketConnection : public RawSocketConnection
    {
    public:
        /**
         * Creates a PlainRawSocketConnection concrete implementation class
         * instance.
         * @param driver[in] Pointer to the socket driver in use.
         * @param io_context[in] The IO Context.
         */
        PlainRawSocketConnection(
            SocketDriver *driver,
            boost::asio::io_context &io_context);

        /**
         * Required virtual destructor.
         */
        virtual ~PlainRawSocketConnection();

        /**
         * @return The socket managed by this instance.
         */
        virtual boost::asio::ip::tcp::socket &get_socket(void)
        { return socket; }

        /**
         * Called to accept the connection and do any negotiation (encryption,
         * etc).
         */
        virtual void start(void);

        /**
         * Closes the socket immediately, but cleanly.
         * The client is not notified of the disconnect, since they made the
         * call.
         */
        virtual void raw_disconnect(void);

        /**
         * @return The size of the underlying socket send buffer, in bytes.
         */
        virtual MG_UnsignedInt get_socket_send_buffer_size(void) const;

        /**
         * @return The size of the underlying socket receive buffer, in bytes.
         */
        virtual MG_UnsignedInt get_socket_recv_buffer_size(void) const;

        /**
         * @return True if socket is encrypted.
         */
        virtual bool raw_is_encrypted(void) const;

        /**
         * Sends the given data over the socket.
         * @param data_ptr[in] The data to send.  The data itself will not be
         * copied.  It must be kept intact until the send is complete.
         * @param data_size[in] The size of the data.
         * @return True if send has started, false if send did NOT start
         * because it was blocked, not connected, or had some other error.
         */
        virtual bool raw_send(const char *data_ptr, const size_t data_size);

    protected:

        /**
         * Called to asynchronously request any waiting data from the socket.
         */
        virtual void do_read(void);

    private:
        boost::asio::ip::tcp::socket socket; ///< The connected socket.
    };
}
}

#endif //MUTGOS_SOCKET_PLAINRAWSOCKETCONNECTION_H
