/*
 * events_SiteEvent.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "text/text_StringConversion.h"

#include "events/events_Event.h"
#include "dbtypes/dbtype_Id.h"
#include "events/events_SiteEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string SiteEvent::to_string() const
    {
        std::ostringstream strstream;

        strstream << "SiteEvent" << std::endl
                  << Event::to_string()
                  << "Site Action: " << text::to_string(site_action)
                  << std::endl
                  << "Site ID:     " << text::to_string(site_id)
                  << std::endl
                  << "Site Name:   " << site_name << std::endl;

        return strstream.str();
    }
}
}