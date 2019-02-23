/*
 * socket_RawSocketConnection.h
 */

#ifndef MUTGOS_SOCKET_RAWSOCKETCONNECTION_H
#define MUTGOS_SOCKET_RAWSOCKETCONNECTION_H

#include <stddef.h>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace socket
{
    // Forward declarations
    //
    class SocketDriver;
    class SocketClientConnection;

    /**
     * Represents the actual socket connection that directly interfaces with
     * the socket library.  This supports SSL and non-SSL connections using
     * sub-classes.  Sub-classes are responsible for 'starting' the connection,
     * doing any negotiation, etc.
     * TODO: Enable accepting SSL at the socket acceptor.
     *
     * Most of the ASIO code is heavily inspired from beast and ASIO examples
     * as much as possible.
     *
     * This is not multi-thread safe.
     */
    class RawSocketConnection
        : public boost::enable_shared_from_this<RawSocketConnection>
    {
    public:
        /**
         * Creates a RawSocketConnection base class instance.
         * @param driver[in] Pointer to the socket driver in use.
         * @param io_context[in] The IO Context.
         */
        RawSocketConnection(
            SocketDriver *driver,
            boost::asio::io_context &io_context);

        /**
         * Required virtual destructor.
         */
        virtual ~RawSocketConnection();

        /**
         * Called to accept the connection and do any negotiation (encryption,
         * etc).
         */
        virtual void start(void) =0;

        /**
         * When known, sets the pointer to the client connection.
         * Pointer will not be owned by class, but do not delete the pointer
         * without first called client_released();
         * @param client[in] Pointer to the client connection.
         */
        void set_client(SocketClientConnection *client);

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
         * Closes the socket immediately, but cleanly.
         * The client is not notified of the disconnect, since they made the
         * call.
         */
        virtual void raw_disconnect(void) =0;

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
         * @return The size of the underlying socket send buffer, in bytes.
         */
        virtual MG_UnsignedInt get_socket_send_buffer_size(void) const =0;

        /**
         * @return The size of the underlying socket receive buffer, in bytes.
         */
        virtual MG_UnsignedInt get_socket_recv_buffer_size(void) const =0;

        /**
         * @return True if socket is encrypted.
         */
        virtual bool raw_is_encrypted(void) const =0;

        /**
         * Sends the given data over the socket.
         * @param data_ptr[in] The data to send.  The data itself will not be
         * copied.  It must be kept intact until the send is complete.
         * @param data_size[in] The size of the data.
         * @return True if send has started, false if send did NOT start
         * because it was blocked, not connected, or had some other error.
         */
        virtual bool raw_send(const char *data_ptr, const size_t data_size) =0;

    protected:
        /**
         * Called to asynchronously request any waiting data from the socket.
         */
        virtual void do_read(void) =0;

        /**
         * Called when the buffer has data in it, from a call to do_read().
         * This will also send the resulting buffer to client_ptr.
         * @param error_code[in] If set, indicates ASIO had an error getting
         * data.
         * @param bytes_transferred[in] The number of bytes transferred from
         * the socket.
         */
        void on_read(
            boost::system::error_code error_code,
            std::size_t bytes_transferred);

        /**
         * Called when the current write to the socket has completed.
         * @param error_code[in] If set, indicates ASIO had an error sending
         * data.
         * @param bytes_transferred[in] The number of bytes tranferred.
         */
        void on_write_complete(
            boost::system::error_code error_code,
            std::size_t bytes_transferred);

        /**
         * Called when the timer has expired, cancelled, etc.
         * @param error_code[in] Indicates if the timer completed successfully.
         */
        void on_timer(boost::system::error_code error_code);

        /**
         * Used when ASIO indicates the socket closed during an operation.
         */
        void handle_disconnect(void);


        bool socket_accepted; ///< True if accepted (valid socket).
        bool socket_blocked; ///< True if blocked on send.
        bool socket_connected; ///< True if socket connected.

        SocketDriver * const driver_ptr; ///< Pointer to driver.

        SocketClientConnection *client_ptr; ///< Pointer to client connection

        boost::asio::strand<boost::asio::executor> strand_executor; ///< The executor
        boost::asio::steady_timer timer; ///< General timer.  Put here to keep all asio stuff in one place.

        char *incoming_buffer; //< Array of char that functions as buffer for incoming data
        static const size_t incoming_buffer_size; ///< Size of incoming_buffer
    };
}
}

#endif //MUTGOS_SOCKET_RAWSOCKETCONNECTION_H

