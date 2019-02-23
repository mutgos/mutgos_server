/*
 * angelscript_ScriptUtilities.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"

#include "angelscript_AngelException.h"
#include "angelscript_ScriptUtilities.h"

#include "security/security_Context.h"

#include "angelscript_ScriptContext.h"

namespace
{
    const asPWORD MUTGOS_USER_DATA_CONTEXT_TYPE = 1;
    const size_t AS_ARRAY_MAX_SIZE = 4096;
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    void ScriptUtilities::set_my_script_context(
        asIScriptEngine *const engine,
        mutgos::angelscript::ScriptContext *context)
    {
        if (not engine)
        {
            LOG(fatal, "angelscript", "set_my_script_context",
                "Provided engine is null!");
        }
        else if (not context)
        {
            LOG(fatal, "angelscript", "set_my_script_context",
                "Provided context is null!");
        }
        else
        {
            engine->SetUserData(context, MUTGOS_USER_DATA_CONTEXT_TYPE);
        }
    }

    // ----------------------------------------------------------------------
    void ScriptUtilities::cleanup_my_script_context(
        asIScriptEngine *const engine)
    {
        engine->SetUserData(0, MUTGOS_USER_DATA_CONTEXT_TYPE);
    }

    // ----------------------------------------------------------------------
    ScriptContext *ScriptUtilities::get_my_script_context(
        asIScriptEngine *const engine)
    {
        ScriptContext *context = 0;

        if (not engine)
        {
            LOG(fatal, "angelscript", "get_my_script_context",
                "Provided engine is null!");
        }
        else
        {
            void *context_raw_ptr =
                engine->GetUserData(MUTGOS_USER_DATA_CONTEXT_TYPE);

            if (context_raw_ptr)
            {
                context = reinterpret_cast<ScriptContext *>(context_raw_ptr);
            }
            else
            {
                LOG(fatal, "angelscript", "get_my_script_context",
                    "Context is null!");
            }
        }

        return context;
    }

    // ----------------------------------------------------------------------
    security::Context *ScriptUtilities::get_my_security_context(
        asIScriptEngine *const engine)
    {
        security::Context *security_context = 0;
        ScriptContext * const context = get_my_script_context(engine);

        if (context)
        {
            security_context = &context->get_security_context();
        }

        return security_context;
    }

    // ----------------------------------------------------------------------
    void ScriptUtilities::set_exception_info(
        asIScriptEngine *const engine,
        const std::exception &exception)
    {
        ScriptContext * const context = get_my_script_context(engine);

        if (context)
        {
            context->set_exception_info(exception);
        }
    }

    // ----------------------------------------------------------------------
    void ScriptUtilities::set_exception_info(
        asIScriptEngine *const engine,
        const ScriptContext::ExceptionType type,
        const std::string &reason)
    {
        ScriptContext * const context = get_my_script_context(engine);

        if (context)
        {
            context->set_exception_type(type);
            context->set_exception_reason(reason);
        }
    }

    // ----------------------------------------------------------------------
    void ScriptUtilities::set_exception_info(asIScriptEngine *const engine)
    {
        ScriptContext * const context = get_my_script_context(engine);

        if (context)
        {
            context->set_exception_type(ScriptContext::EXCEPTION_OTHER);
            context->set_exception_reason("Unknown exception thrown.");
        }
    }

    // ----------------------------------------------------------------------
    CScriptArray *ScriptUtilities::create_array(
        asIScriptEngine *const engine,
        const std::string &template_type,
        const size_t initial_size,
        const bool exception_on_too_big)
    {
        CScriptArray *result = 0;
        const std::string full_type = "array<" + template_type + ">";
        asITypeInfo * const type_ptr =
            engine->GetTypeInfoByDecl(full_type.c_str());

        if (not type_ptr)
        {
            throw AngelException(
                "create_array(): Could not get array type declaration of "
                + template_type);
        }
        else
        {
            asUINT size_uint = 0;

            if (initial_size > AS_ARRAY_MAX_SIZE)
            {
                if (exception_on_too_big)
                {
                    throw AngelException(
                        "create_array(): Exceeded max initial size for array "
                        "type of " + template_type);
                }

                size_uint = AS_ARRAY_MAX_SIZE;
            }
            else
            {
                size_uint = (asUINT) initial_size;
            }

            result = CScriptArray::Create(type_ptr);

            if (not result)
            {
                throw AngelException(
                    "create_array(): Out of memory while creating "
                    "array of type " + template_type);
            }
            else if (size_uint)
            {
                result->Reserve(size_uint);
            }
        }

        return result;
    }
}
}
