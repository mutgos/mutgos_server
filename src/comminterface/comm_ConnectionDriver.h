/*
 * comm_ConnectionDriver.h
 */

#ifndef MUTGOS_COMM_CONNECTIONDRIVER_H
#define MUTGOS_COMM_CONNECTIONDRIVER_H

namespace mutgos
{
namespace comm
{
    // Forward declarations
    //
    class RouterSessionManager;
    class ClientConnection;

    /**
     * An interface class for specific connection implementations (socket,
     * websocket, etc).  Basically, it's like a device driver but for
     * connection types.  It provides abstracted access to functions common
     * to all connection types.
     *
     * It is assumed these are NOT thread safe (only one thread will call them
     * at once).
     */
    class ConnectionDriver
    {
    public:
        /**
         * Required 'interface' virtual destructor.
         */
        virtual ~ConnectionDriver()
          { }

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
        virtual bool start(void) =0;

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
        virtual void stop(comm::RouterSessionManager *router_ptr) =0;

        /**
         * Called by the router when the driver is to service new and existing
         * connections, transfer incoming pending data, etc.  When not in this
         * method, the driver should do nothing at all - this is to restrict comms
         * processing to a single thread.
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
        virtual bool do_work(RouterSessionManager *router_ptr) =0;

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
        virtual void release(ClientConnection *connection_ptr) =0;

    protected:
        /**
         * Required 'interface' constructor.
         */
        ConnectionDriver(void)
          { }

    private:
        // No copying
        ConnectionDriver &operator=(const ConnectionDriver &rhs);
        ConnectionDriver(const ConnectionDriver &rhs);
    };
}
}

#endif //MUTGOS_COMM_CONNECTIONDRIVER_H
