/*
 * socket_SecureRawSocketConnection.h
 */

#ifndef MUTGOS_SOCKET_SECURERAWSOCKETCONNECTION_H
#define MUTGOS_SOCKET_SECURERAWSOCKETCONNECTION_H

#include <boost/asio/ssl.hpp>

#include "socket_PlainRawSocketConnection.h"

namespace mutgos
{
namespace socket
{
    /**
     * Implementation of an encrypted raw socket connection.
     */
    class SecureRawSocketConnection : public PlainRawSocketConnection
    {
    public:
        /**
         * Creates a SecureRawSocketConnection concrete implementation class
         * instance.
         */
        SecureRawSocketConnection(
            SocketDriver *driver,
            boost::asio::io_context &io_context,
            boost::asio::ssl::context &ssl_context);

        /**
         * Required virtual destructor.
         */
        virtual ~SecureRawSocketConnection();

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
         * Called asynchronously after handshake is complete.
         * @param error_code[in] If set, indicates ASIO had an error
         * completing the handshake.
         */
        void on_handshake_complete(boost::system::error_code error_code);

        /**
         * Called to asynchronously request any waiting data from the socket.
         */
        virtual void do_read(void);

    private:
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket; ///< The connected socket.
    };
}
}
#endif