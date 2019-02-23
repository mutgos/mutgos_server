/*
 * angelscript_SystemOps.cpp
 */

#include <string>
#include <angelscript.h>

#include "add_on/scriptarray.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "security/security_Context.h"

#include "primitives/primitives_Result.h"
#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_SystemPrims.h"

#include "angelscript_AString.h"
#include "angelscript_AEntity.h"
#include "angelscript_OnlineStatEntry.h"
#include "angelscript_AngelException.h"
#include "angelscript_ScriptUtilities.h"

#include "angelscript_SystemOps.h"

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "SystemOps";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool SystemOps::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        engine.SetDefaultNamespace(AS_OBJECT_TYPE_NAME.c_str());

        // Register the functions
        //
        rc = engine.RegisterGlobalFunction(
            "string@ get_formatted_processes()",
            asFUNCTION(get_formatted_processes),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "array<OnlineStatEntry> @get_online_players()",
            asFUNCTION(get_online_players),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "Entity@ get_me()",
            asFUNCTION(get_me),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        engine.SetDefaultNamespace("");

        return result;
    }

    // ----------------------------------------------------------------------
    void SystemOps::get_me(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "get_me",
                "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // What will be our return value.
        //
        AEntity * const result_ptr = new AEntity(
            engine_ptr,
            ScriptUtilities::get_my_security_context(engine_ptr)->
                get_requester());

        // Return the result
        *(AEntity **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void SystemOps::get_formatted_processes(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "get_formatted_processes",
                "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // What will be our return value.
        //
        AString *result_ptr = 0;

        try
        {
            std::string raw_output;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    system_prims().get_formatted_processes(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        raw_output);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_formatted_processes()");
            }
            else
            {
                result_ptr = new AString(engine_ptr);
                result_ptr->import_from_string(raw_output);
            }
        }
        catch (std::exception &ex)
        {
            ScriptUtilities::set_exception_info(engine_ptr, ex);
            throw;
        }
        catch (...)
        {
            ScriptUtilities::set_exception_info(engine_ptr);
            throw;
        }

        // Return the result
        *(AString **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void SystemOps::get_online_players(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "get_online_players",
                "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // What will be our return value.
        //
        CScriptArray *result_ptr = 0;

        try
        {
            security::Context * const context_ptr =
                ScriptUtilities::get_my_security_context(engine_ptr);
            primitives::SystemPrims::SessionStatsVector raw_sessions;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    system_prims().get_online_players(
                        *context_ptr,
                        context_ptr->get_requester().get_site_id(),
                        raw_sessions);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_online_players()");
            }
            else
            {
                result_ptr = ScriptUtilities::create_array(
                    engine_ptr,
                    "OnlineStatEntry",
                    raw_sessions.size(),
                    false);

                if (result_ptr)
                {
                    size_t inserts = 0;

                    // Have the array, now populate it.
                    //
                    for (primitives::SystemPrims::SessionStatsVector::const_iterator
                            iter = raw_sessions.begin();
                         iter != raw_sessions.end();
                         ++iter)
                    {
                        result_ptr->InsertLast(
                            new OnlineStatEntry(engine_ptr, *iter));
                        ++inserts;

                        if (not (inserts % 20))
                        {
                            memory::ThreadVirtualHeapManager::
                                check_overallocation(true);
                        }
                    }
                }
            }
        }
        catch (std::exception &ex)
        {
            ScriptUtilities::set_exception_info(engine_ptr, ex);
            throw;
        }
        catch (...)
        {
            ScriptUtilities::set_exception_info(engine_ptr);
            throw;
        }

        // Return the result
        *(CScriptArray **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void SystemOps::check_register_rc(
        const int rc,
        const size_t line,
        bool &current_result)
    {
        if (rc < 0)
        {
            current_result = false;

            LOG(fatal, "angelscript", "check_register_rc",
                "Failed to register with AngelScript.  rc = "
                + text::to_string(rc)
                + ",  line = "
                + text::to_string(line));
        }
    }
}
}
