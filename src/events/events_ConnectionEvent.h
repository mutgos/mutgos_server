/*
 * events_ConnectionEvent.h
 */

#ifndef MUTGOS_EVENTS_CONNECTIONEVENT_H
#define MUTGOS_EVENTS_CONNECTIONEVENT_H

#include "events/events_Event.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents when the state of an Entity's connection outside of MUTGOS
     * (via the connection manager) has changed.
     */
    class ConnectionEvent : public Event
    {
    public:
        /** The possible actions (states) that can occur for a connection */
        enum ConnectionAction
        {
            /** Entity has connected */
            ACTION_CONNECTED,
            /** Entity has disconnected */
            ACTION_DISCONNECTED
        };

        /**
         * Constructor to set attributes of event.
         * @param action[in] The connection action for the event.
         * @param entity_id[in] The ID associated with the connection.
         * @param source[in] The fully qualified hostname or other address
         * associated with the connection.
         */
        ConnectionEvent(
            const ConnectionAction action,
            const dbtype::Id &entity_id,
            const std::string &source)
            : Event(Event::EVENT_CONNECTION),
              connection_action(action),
              connection_entity_id(entity_id),
              connection_source(source),
              connection_timestamp(true)
          { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ConnectionEvent(const ConnectionEvent &rhs)
            : Event(rhs),
              connection_action(rhs.connection_action),
              connection_entity_id(rhs.connection_entity_id),
              connection_source(rhs.connection_source),
              connection_timestamp(rhs.connection_timestamp)
        { }

        /**
         * Required virtual destructor.
         */
        virtual ~ConnectionEvent()
          { }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return The connection action.
         */
        const ConnectionAction get_action(void) const
          { return connection_action; }

        /**
         * @return The entity ID associated with the action.
         */
        const dbtype::Id &get_entity_id(void) const
          { return connection_entity_id; }

        /**
         * @return The source of the connection associated with the action.
         */
        const std::string &get_source(void) const
          { return connection_source; }

        /**
         * @return When this event was created.
         */
        const dbtype::TimeStamp &get_timestamp(void) const
          { return connection_timestamp; }

    private:
        const ConnectionAction connection_action; ///< Action this event represents
        const dbtype::Id connection_entity_id; ///< Entity ID associated with connection
        const std::string connection_source; ///< Hostname associated with connection
        const dbtype::TimeStamp connection_timestamp; ///< When the event was created
    };
}
}


#endif //MUTGOS_EVENTS_CONNECTIONEVENT_H
