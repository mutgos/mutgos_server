/*
 * primitives_Result.cpp
 */

#include <string>

#include "primitives_Result.h"

namespace
{
    const static std::string STATUS_AS_STRING[] =
    {
        "OK",
        "SECURITY_VIOLATION",
        "BAD_ARGUMENTS",
        "BAD_ENTITY_TYPE",
        "IMPOSSIBLE"
    };
}

namespace mutgos
{
namespace primitives
{
    // ----------------------------------------------------------------------
    const std::string &Result::status_to_string(void) const
    {
        if ((status > STATUS_IMPOSSIBLE) or (status < STATUS_OK))
        {
            return STATUS_AS_STRING[STATUS_IMPOSSIBLE];
        }

        return STATUS_AS_STRING[status];
    }
}
}