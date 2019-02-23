/*
 * angelscript_DatabaseOps.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_DatabasePrims.h"

#include "angelscript_AngelException.h"
#include "angelscript_AString.h"
#include "angelscript_AEntity.h"
#include "angelscript_DatabaseOps.h"

#include "angelscript_ScriptContext.h"
#include "angelscript_ScriptUtilities.h"

#include "dbtypes/dbtype_EntityType.h"

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "DatabaseOps";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool DatabaseOps::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        engine.SetDefaultNamespace(AS_OBJECT_TYPE_NAME.c_str());

        // Register the functions
        //
        rc = engine.RegisterGlobalFunction(
            "Entity@ match_name_to_entity(const string &in search_string, const bool exact_match, const EntityType entity_type, bool &out ambiguous)",
            asFUNCTION(match_name_to_entity),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "Entity@ convert_id_to_entity(const string &in id_as_string)",
            asFUNCTION(convert_id_to_entity),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "Entity@ create_entity(const EntityType entity_type, const string &in name)",
            asFUNCTION(create_entity),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        engine.SetDefaultNamespace("");

        return result;
    }

    // ----------------------------------------------------------------------
    void DatabaseOps::match_name_to_entity(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "match_name_to_entity", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // Arguments passed in via generic interface
        //
        AString * const search_string = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(0));
        const bool exact_match = *(bool*)gen_ptr->GetAddressOfArg(1);
        const dbtype::EntityType entity_type =
            (dbtype::EntityType) gen_ptr->GetArgDWord(2);
        bool * const ambiguous = (bool*)gen_ptr->GetAddressOfArg(3);

        // What will be our return value.
        //
        AEntity *result_ptr = 0;

        try
        {
            if ((not search_string) or (not ambiguous))
            {
                throw AngelException(
                    "AngelScript passed null pointers to us",
                    AS_OBJECT_TYPE_NAME,
                    "match_name_to_entity(string, bool, EntityType, bool)");
            }

            dbtype::Id found_entity;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().match_name_to_id(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        search_string->export_to_string(),
                        exact_match,
                        entity_type,
                        found_entity,
                        *ambiguous);

            if (not prim_result.is_success())
            {
                /**  Assume whatever it is cannot be found.
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "match_name_to_entity(string, bool, EntityType, bool)");
                */

                result_ptr = new AEntity(engine_ptr);
            }
            else
            {
                result_ptr = new AEntity(engine_ptr, found_entity);
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
        *(AEntity **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void DatabaseOps::convert_id_to_entity(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "convert_id_to_entity",
                "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // Arguments passed in via generic interface
        //
        AString * const id_as_string = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(0));

        // What will be our return value.
        //
        AEntity *result_ptr = 0;

        try
        {
            if (not id_as_string)
            {
                throw AngelException(
                    "AngelScript passed null pointers to us",
                    AS_OBJECT_TYPE_NAME,
                    "convert_id_to_entity(string)");
            }

            const dbtype::Id converted_entity =
                primitives::PrimitivesAccess::instance()->
                    database_prims().convert_string_to_id(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        id_as_string->export_to_string());

            result_ptr = new AEntity(engine_ptr, converted_entity);
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
        *(AEntity **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void DatabaseOps::create_entity(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "create_entity", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        // Arguments passed in via generic interface
        //
        const dbtype::EntityType entity_type =
            (dbtype::EntityType) gen_ptr->GetArgDWord(0);
        AString * const name = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(1));

        // What will be our return value.
        //
        AEntity *result_ptr = 0;

        try
        {
            if (not name)
            {
                throw AngelException(
                    "AngelScript passed null pointers to us",
                    AS_OBJECT_TYPE_NAME,
                    "convert_string_to_entity(string)");
            }

            dbtype::Id created_entity_id;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().create_entity(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_type,
                        name->export_to_string(),
                        created_entity_id);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "create_entity(EntityType, string)");
            }
            else
            {
                result_ptr = new AEntity(engine_ptr, created_entity_id);
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
        *(AEntity **)gen_ptr->GetAddressOfReturnLocation() = result_ptr;
    }

    // ----------------------------------------------------------------------
    void DatabaseOps::check_register_rc(
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
