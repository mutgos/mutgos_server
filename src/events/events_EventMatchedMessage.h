/*
 * events_EventMatchedMessage.h
 */

#ifndef MUTGOS_EVENTS_EVENTMATCHEDMESSAGE_H
#define MUTGOS_EVENTS_EVENTMATCHEDMESSAGE_H

#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ProcessInfo.h"

#include "events/events_CommonTypes.h"
#include "events/events_Event.h"

namespace mutgos
{
namespace events
{
    /**
     * A message that can be sent to a Process, indicating an event has
     * matched one of their subscriptions.
     */
    class EventMatchedMessage : public executor::ProcessMessage
    {
    public:
        /**
         * Constructor.  Creates the message.
         * @param id[in] The subscription ID that the event matches.
         * @param event[in] The event itself.  This class will take
         * ownership of the pointer!  This does not check for null!
         */
        EventMatchedMessage(
            const SubscriptionId id,
            Event * const event)
            : ProcessMessage(ProcessMessage::MESSAGE_EVENT),
              subscription_id(id),
              event_ptr(event)
        { }

        /**
         * Required virtual destructor.
         */
        virtual ~EventMatchedMessage()
          { delete event_ptr; }

        /**
         * @return The subscription ID that the event matched.
         */
        const SubscriptionId get_subscription_id(void) const
          { return subscription_id; }

        /**
         * This is just a convenience method.
         * @return The type of the event, useful for knowing what to cast it
         * into.
         */
        const Event::EventType get_event_type(void) const
          { return event_ptr->get_event_type(); }

        /**
         * @return The event itself.
         */
        Event &get_event(void) const
          { return *event_ptr; }

    private:
        // No copying
        //
        EventMatchedMessage(const EventMatchedMessage &rhs);
        EventMatchedMessage &operator=(const EventMatchedMessage &rhs);


        const SubscriptionId subscription_id; ///< The subscription ID that matched
        Event * const event_ptr; ///< The event which matched.
    };
}
}

#endif //MUTGOS_EVENTS_EVENTMATCHEDMESSAGE_H
