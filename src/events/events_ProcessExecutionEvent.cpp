/*
 * events_ProcessExecutionEvent.cpp
 */

#include <string>
#include <ostream>

#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "events/events_ProcessExecutionEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    std::string ProcessExecutionEvent::to_string() const
    {
        std::ostringstream strstream;

        strstream << "ProcessExecutionEvent" << std::endl
                  << Event::to_string()
                  << "PID:             "
                  << text::to_string(process_id) << std::endl
                  << "EXE ID:          " << executable_id.to_string(true)
                  << std::endl
                  << "Native:          " << native_executable
                  << std::endl
                  << "Owner ID:        " << owner_id.to_string(true)
                  << std::endl
                  << "Process name:    " << process_name
                  << std::endl
                  << "Process state:   "
                  << text::to_string(process_state)
                  << std::endl;

        return strstream.str();
    }
}
}