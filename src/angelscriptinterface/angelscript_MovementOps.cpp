/*
 * angelscript_MovementOps.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_MovementPrims.h"

#include "angelscript_AngelException.h"
#include "angelscript_AEntity.h"

#include "angelscript_ScriptContext.h"
#include "angelscript_ScriptUtilities.h"

#include "angelscript_MovementOps.h"


namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "MovementOps";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool MovementOps::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        engine.SetDefaultNamespace(AS_OBJECT_TYPE_NAME.c_str());

        rc = engine.RegisterGlobalFunction(
            "void move_entity(Entity &in entity_to_move, Entity &in entity_destination)",
            asFUNCTION(move_entity),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        engine.SetDefaultNamespace("");

        return result;
    }

    // ----------------------------------------------------------------------
    void MovementOps::move_entity(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "move_entity", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AEntity * const entity_to_move = reinterpret_cast<AEntity *>(
            gen_ptr->GetArgObject(0));
        AEntity * const entity_destination = reinterpret_cast<AEntity *>(
            gen_ptr->GetArgObject(1));

        try
        {
            if ((not entity_to_move) or (not entity_destination))
            {
                throw AngelException(
                    "AngelScript passed null pointers to us",
                    AS_OBJECT_TYPE_NAME,
                    "move_entity(Entity, Entity)");
            }

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    movement_prims().move_entity(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_to_move->get_id(),
                        entity_destination->get_id());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "move_entity(Entity, Entity)");
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
    }

    // ----------------------------------------------------------------------
    void MovementOps::check_register_rc(
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
