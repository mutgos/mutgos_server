/*
 * angelscript_AngelException.cpp
 */

#include <string>

#include "angelscript_AngelException.h"

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    AngelException::AngelException(void)
      : message("Unknown error.")
    {
    }

    // ----------------------------------------------------------------------
    AngelException::AngelException(const std::string &reason)
      : message(reason)
    {
    }

    // ----------------------------------------------------------------------
    AngelException::AngelException(
        const std::string &reason,
        const primitives::Result &result)
    {
        message = result.status_to_string() + ": " + reason;
    }

    // ----------------------------------------------------------------------
    AngelException::AngelException(
        const std::string &reason,
        const std::string &originating_class,
        const std::string &originating_method)
    {
        message = originating_class + "." + originating_method + ": " + reason;
    }

    // ----------------------------------------------------------------------
    AngelException::AngelException(
        const std::string &reason,
        const primitives::Result &result,
        const std::string &originating_class,
        const std::string &originating_method)
    {
        message = originating_class + "." + originating_method + ": " +
            result.status_to_string() + ": " + reason;
    }

    // ----------------------------------------------------------------------
    AngelException::~AngelException()
    {
    }
}
}