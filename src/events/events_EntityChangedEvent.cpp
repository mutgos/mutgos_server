/*
 * events_EntityChangedEvent.cpp
 */

#include <string>
#include <ostream>

#include "events/events_EntityChangedEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string EntityChangedEvent::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "EntityChangedEvent" << std::endl
                  << Event::to_string()
                  << "Entity ID:   " << entity_id.to_string(true) << std::endl
                  << "Entity type: "
                  << dbtype::entity_type_to_string(entity_type) << std::endl
                  << "Entity action: " << entity_action
                  << "Fields changed size: " << entity_fields_changed.size()
                  << std::endl
                  << "Entity flags changed size: "
                  << entity_flags_changed.first.size()
                  << "  " << entity_flags_changed.second.size()
                  << std::endl
                  << "Entity ID fields changed size: "
                  << entity_id_fields_changed.size()
                  << std::endl;

        return strstream.str();
    }
}
}