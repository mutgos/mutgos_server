/*
 * events_EmitEvent.cpp
 */

#include <string>
#include <ostream>

#include "events/events_EmitEvent.h"
#include "text/text_ExternalText.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string EmitEvent::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "EmitEvent" << std::endl
                  << Event::to_string()
                  << "Source:     " << emit_source.to_string(true) << std::endl
                  << "Target:     " << emit_target.to_string(true) << std::endl
                  << "Exclude:    " << emit_exclude.to_string(true) << std::endl
                  << "Text:       " << text::ExternalText::to_string(emit_text)
                  << std::endl
                  << "Program:    " << emit_program.to_string(true) << std::endl
                  << "PID:        " << emit_program_pid << std::endl
                  << "Timestamp:  " << emit_timestamp.to_string() << std::endl;

        return strstream.str();
    }
}
}