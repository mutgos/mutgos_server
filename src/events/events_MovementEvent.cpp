/*
 * events_MovementEvent.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "dbtypes/dbtype_Id.h"

#include "events/events_MovementEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string MovementEvent::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "MovementEvent" << std::endl
                  << Event::to_string()
                  << "Who:     " << movement_who.to_string(true) << std::endl
                  << "From:    " << movement_from.to_string(true) << std::endl
                  << "To:      " << movement_to.to_string(true) << std::endl
                  << "Program: " << movement_via_program << std::endl
                  << "How:     " << movement_how.to_string(true) << std::endl;

        return strstream.str();
    }
}
}