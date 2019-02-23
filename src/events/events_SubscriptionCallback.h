/*
 * events_SubscriptionCallback.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONCALLBACK_H
#define MUTGOS_EVENTS_SUBSCRIPTIONCALLBACK_H

#include "executor/executor_ProcessInfo.h"
#include "events/events_CommonTypes.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class EventListener;
    class Event;

    /**
     * Used by a class to indicate how it wants to be called back when
     * a certain subscription is satisfied.
     */
    class SubscriptionCallback
    {
    public:
        /**
         * Creates an invalid callback.
         */
        SubscriptionCallback(void)
            : subscription_id(0),
              pid_callback(0),
              listener_callback_ptr(0)
          { }

        /**
         * Creates a callback that uses a message to send the callback to
         * a Process.
         * @param pid[in] The PID of the process.
         */
        SubscriptionCallback(const executor::PID pid)
            : subscription_id(0),
              pid_callback(pid),
              listener_callback_ptr(0)
          { }

        /**
         * Creates a callback that uses a listener that gets directly called
         * back.
         * @param listener_ptr[in] The pointer to the listener.  This class
         * will NOT own the pointer and will never try and delete it.  However,
         * the listener cannot be deleted until after this class has been
         * destructed.
         */
        SubscriptionCallback(EventListener * const listener_ptr)
            : subscription_id(0),
              pid_callback(0),
              listener_callback_ptr(listener_ptr)
          { }

        /**
         * Destructor.
         */
        ~SubscriptionCallback()
          { }

        /**
         * @return True if this class instance is valid - that a listener
         * callback has been specified.
         */
        bool valid(void) const
          { return (pid_callback || listener_callback_ptr); }

        /**
         * Sets the subscription ID.  This must be done prior to calling
         * do_callback().  Users do not call this; the events infrastructure
         * will.
         * @param id[in] The subscription ID the callback is associated with.
         */
        void set_subscription_id(const SubscriptionId id)
          { subscription_id = id; }

        /**
         * @return The subscription ID associated with the callback.
         */
        const SubscriptionId get_subscription_id(void) const
          { return subscription_id; }

        /**
         * @return The PID to be called back, or 0 (invalid) if not using PID.
         */
        const executor::PID get_pid(void) const
          { return pid_callback; }

        /**
         * @return The pointer of the listener to call back, or null if not
         * using a listener.
         */
        EventListener *get_listener(void) const
          { return listener_callback_ptr; }

        /**
         * Determines the correct way to notify the subscriber that the
         * provided event has satisfied the subscription, and then does the
         * notification.
         * @param event_ptr[in] The event to provide to the subscriber.
         * Control of the pointer passes to this method; the pointer will
         * likely be deleted when the call returns.
         * @return True if successfully notified, false if not.
         */
        bool do_callback(Event * const event_ptr) const;

        /**
         * Determines the correct way to notify the subscriber that the
         * provided event has been deleted by the infrastructure, and then
         * does the notification.
         * @return True if successfully notified, false if not.
         */
        bool do_delete_callback(void) const;

    private:
        SubscriptionId subscription_id; ///< Subscription ID
        executor::PID pid_callback; ///< If using messaging, PID of Process to message.
        EventListener *listener_callback_ptr; ///< If using direct callback, the pointer to listener.
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONCALLBACK_H
