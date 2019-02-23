/*
 * socket_SocketDriver.h
 */

#ifndef MUTGOS_SOCKET_SOCKETDRIVER_H
#define MUTGOS_SOCKET_SOCKETDRIVER_H

#include <map>
#include <vector>

#include <boost/asio/ip/tcp.hpp>

#include "osinterface/osinterface_OsTypes.h"

#include "comminterface/comm_ConnectionDriver.h"

namespace mutgos
{
namespace comm
{
    // Forward declarations
    //
    class RouterSessionManager;
    class ClientConnection;
}

namespace socket
{
    // Forward declarations
    //
    class SocketClientConnection;

    /**
     * Driver class for sockets.
     *
     * This is not thread safe.
     */
    class SocketDriver : public comm::ConnectionDriver
    {
    public:
        /**
         * Creates an instance of the driver.
         * @param router[in] Pointer to the router used primarily for
         * authentication, seeing who is connected, etc.  Cannot be null.
         */
        SocketDriver(comm::RouterSessionManager *router);

        /**
         * Required virtual destructor.
         */
        virtual ~SocketDriver();

        /**
         * Called when MUTGOS wants this driver to start (listen for and
         * process connections).
         * If this is called when already started, there is no effect (must
         * return true).
         * It is possible for a driver to be repeatedly stopped and started.
         * If this driver has configuration data it reads, it will re-read
         * it when start() is called.
         * This method will not return until completely started.
         * @return True if successfully started.
         */
        virtual bool start(void);

        /**
         * Called when MUTGOS wants this driver to stop (close all connections
         * and stop listening for new ones).  If there are connections open,
         * it will NOT delete the pointers associated with the ClientConnection,
         * however each connection will be marked as closed.
         * If this is called when already stopped, there is no effect.
         * This method will not return until completely stopped.
         * @param router_ptr[in] Pointer to the router.  Used when
         * communicating status, incoming data, etc.
         */
        virtual void stop(comm::RouterSessionManager *router_ptr);

        /**
         * Called by the router when the driver is to service new and existing
         * connections, transfer incoming pending data, etc.  When not in this
         * method, the driver should do nothing at all - this is to restrict
         * comms processing to a single thread and make multi-threaded issues
         * less of a concern.
         *
         * Note that drivers may elect to send data on demand when provided via
         * a ClientConnection, or they can wait to transfer it via this method.
         * It is safe to do so because drivers will never be in do_work() AND
         * have a ClientConnection method called at the same time.
         *
         * Drivers should do as much work as is immediately available to them,
         * and then return.  They may be given the opportunity to be called
         * back immediately if they think more work will be available shortly.
         * @param router_ptr[in] Pointer to the router.  Used when
         * communicating status, incoming data, etc.
         * @return True if all work done, false if driver knows it has more
         * work to do.
         */
        virtual bool do_work(comm::RouterSessionManager *router_ptr);

        /**
         * Called by the comm subsystem when it is completely done using a
         * ClientConnection.  This normally only happens when the connection
         * has been disconnected, the driver is being stopped, or MUTGOS is
         * shutting down.  If a connection is released before it has been
         * disconnected, this implies it will be disconnected while being
         * released.
         * Generally, calling this method will result in the associated
         * memory of the ClientConnection being freed.
         * If the pointer to the ClientConnection is not recognized, no
         * action shall occur.
         * This may be called when the driver is stopped.
         * @param connection_ptr[in] The ClientConnection to release.
         */
        virtual void release(comm::ClientConnection *connection_ptr);

        /**
         * Indicates the ClientConnection's pointer is now held by an
         * additional class (which could be this class).
         * @param connection_ptr[in] The ClientConnection's pointer that has
         * another class holding onto it.
         */
        void add_reference(SocketClientConnection *connection_ptr);

        /**
         * @return The router in use.
         */
        comm::RouterSessionManager *get_router(void)
        { return my_router_ptr; }

        /**
         * Called when a connection needs to be called back (do_work())after
         * unwinding its stack, at some unspecified time in the future.
         * This does not check to see if multiple calls for the same
         * connection are pending.
         * @param connection_ptr[in] The pointer of the connection to be
         * called back later.
         */
        void connection_has_pending_actions(
            SocketClientConnection *connection_ptr);

    private:
        typedef std::vector<SocketClientConnection *> PendingActions;
        typedef std::vector<SocketClientConnection *> PendingDeletes;
        /** First is the pointer to the connection, second is number of references */
        typedef std::map<SocketClientConnection *, MG_SignedInt> ClientConnections;

        comm::RouterSessionManager * const my_router_ptr; ///< Pointer to router.

        boost::asio::io_context io_context; ///< The IO Context for the sockets.

        bool started; ///< True if start() has been called successfully.
        PendingActions pending_actions; ///< connections with pending actions.
        PendingDeletes pending_deletes; ///< connections to be deleted (no more references to them).
        ClientConnections client_connections; ///< All the active client connections.
    };
}
}

#endif //MUTGOS_SOCKET_SOCKETDRIVER_H
