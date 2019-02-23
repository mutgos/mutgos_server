/*
 * events_ConnectionEvent.cpp
 */

#include <string>
#include <ostream>

#include "events/events_Event.h"
#include "events/events_ConnectionEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string ConnectionEvent::to_string() const
    {
        std::ostringstream strstream;

        strstream << "ConnectionEvent" << std::endl
                  << Event::to_string()
                  << "Action:      " << connection_action << std::endl
                  << "Entity ID:   " << connection_entity_id.to_string(true)
                  << std::endl
                  << "Source:      " << connection_source << std::endl
                  << "Timestamp:   " << connection_timestamp.to_string()
                  << std::endl;

        return strstream.str();
    }
}
}