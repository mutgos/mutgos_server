/*
 * events_EventListener.h
 */

#ifndef MUTGOS_EVENTS_EVENTLISTENER_H
#define MUTGOS_EVENTS_EVENTLISTENER_H

#include "events/events_CommonTypes.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class Event;

    /**
     * A pure abstract (interface) class that non-Process based classes can be
     * called back with when their event subscription(s) match an event.
     *
     * Most listeners are Process based and therefore should be using messages.
     */
    class EventListener
    {
    public:
        /**
         * Default constructor.
         */
        EventListener(void)
         { }

        /**
         * Required virtual destructor.
         */
        virtual ~EventListener()
          { }

        /**
         * Called when an event matches a listener's subscription.
         * Only one thread will call this at a time.
         * @param id[in] The subscription ID that matched.
         * @param event[in] The event that matched.
         */
        virtual void subscribed_event_matched(
            const SubscriptionId id,
            Event &event) =0;

        /**
         * Called when a subscription is deleted by the infrastructure.
         * Currently, this can only happen because an Entity ID in the
         * subscription has been deleted, or a site has been removed.
         * @param ids_deleted[in] The subscription IDs being deleted.
         */
        virtual void subscription_deleted(
            const SubscriptionIdList &ids_deleted) =0;
    };
}
}

#endif //MUTGOS_EVENTS_EVENTLISTENER_H
