/*
 * angelscript_ScriptContext.cpp
 */

#include <exception>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Id.h"

#include "security/security_SecurityException.h"
#include "security/security_Context.h"

#include "angelscript_AngelException.h"
#include "angelscript_ScriptContext.h"

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    ScriptContext::ScriptContext(security::Context * const security_context)
        : security_context_ptr(security_context),
          exception_type(ScriptContext::EXCEPTION_NONE)
    {
        if (not security_context_ptr)
        {
            LOG(fatal, "angelscript", "ScriptContext",
                "security_context is null!  Crash will follow.");
        }
    }

    // ----------------------------------------------------------------------
    ScriptContext::~ScriptContext()
    {
        delete security_context_ptr;
    }

    // ----------------------------------------------------------------------
    void ScriptContext::set_exception_info(const std::exception &exception)
    {
        if (dynamic_cast<const std::bad_alloc *>(&exception))
        {
            exception_type = EXCEPTION_MEMORY;
        }
        else if (dynamic_cast<const security::SecurityException *>(&exception))
        {
            exception_type = EXCEPTION_SECURITY;
            exception_what = exception.what();
        }
        else if (dynamic_cast<const AngelException *>(&exception))
        {
            exception_type = EXCEPTION_ANGEL;
            exception_what = exception.what();
        }
        else if (dynamic_cast<const std::exception *>(&exception))
        {
            exception_type = EXCEPTION_STD;
        }
        else
        {
            exception_type = EXCEPTION_OTHER;
        }
    }

    // ----------------------------------------------------------------------
    void ScriptContext::clear_exception(void)
    {
        exception_type = EXCEPTION_NONE;
        exception_what.clear();
        exception_what.shrink_to_fit();
    }
}
}