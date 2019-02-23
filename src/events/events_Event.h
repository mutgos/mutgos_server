/*
 * events_Event.h
 */

#ifndef MUTGOS_EVENTS_EVENT_H
#define MUTGOS_EVENTS_EVENT_H

#include <string>

namespace mutgos
{
namespace events
{
    /**
     * Base class for all events, which are things that happen in the game,
     * such as movement, talking, connecting/disconnecting, etc.
     *
     * Since the (sub)class's information is not modified after being
     * accepted by the events subsystem, thread safety is not needed at this
     * time.
     */
    class Event
    {
    public:
        /** Type of event subclass */
        enum EventType
        {
            /** MovementEvent class */
            EVENT_MOVEMENT,
            /** EmitEvent class */
            EVENT_EMIT,
            /** ConnectionEvent class */
            EVENT_CONNECTION,
            /** EntityChangedEvent class */
            EVENT_ENTITY_CHANGED,
            /** ProcessExecutionEvent class */
            EVENT_PROCESS_EXECUTION,
            /** SiteEvent class */
            EVENT_SITE,
            /** Do not use, used for allocating arrays */
            EVENT_END_INVALID
        };

        /**
         * Constructor.
         */
        Event(const EventType type)
            : event_type(type)
          { }

        /**
         * Required virtual destructor.
         */
        virtual ~Event()
          { }

        /**
         * @return The type of the event.
         */
        const EventType get_event_type(void) const
          { return event_type; }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const
          { return "\n"; }

    protected:
        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        Event(const Event &rhs)
            : event_type(rhs.event_type)
        { }

    private:
        // Only copy constructors allowed, since everything is const.
        Event &operator=(const Event &rhs) const;

        // Currently, no equality check needed.
        bool operator==(const Event &rhs) const;

        const EventType event_type; ///< Type of subclass
    };
}
}

#endif //MUTGOS_EVENTS_EVENT_H
