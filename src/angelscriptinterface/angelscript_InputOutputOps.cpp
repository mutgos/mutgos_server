/*
 * angelscript_InputOutputOps.cpp
 */

#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"
#include "text/text_ExternalText.h"

#include "channels/events_TextChannel.h"

#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_EventPrims.h"
#include "primitives/primitives_SystemPrims.h"

#include "angelscript_AngelException.h"
#include "angelscript_AEntity.h"

#include "angelscript_ScriptContext.h"
#include "angelscript_ScriptUtilities.h"

#include "angelscript_InputOutputOps.h"

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "InputOutputOps";
    const std::string EMIT_TO_ROOM_METHOD_SIG =
        "emit_to_room(Entity, string, bool)";
    const std::string BROADCAST_TO_ROOM_METHOD_SIG =
        "broadcast_to_room(Entity, string, bool)";
    const std::string SEND_TO_ENTITY_METHOD_SIG =
        "send_to_entity(Entity, string, bool)";
    const std::string PRINTLN_METHOD_SIG =
        "println(string)";
    const std::string MPRINTLN_METHOD_SIG =
        "mprintln(string)";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool InputOutputOps::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        engine.SetDefaultNamespace(AS_OBJECT_TYPE_NAME.c_str());

        // Register the functions
        //
        rc = engine.RegisterGlobalFunction(
            "void emit_to_room(Entity &in room, const string &in text, const bool prepend_self)",
            asFUNCTION(emit_to_room),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "void broadcast_to_room(Entity &in room, const string &in text, const bool prepend_self)",
            asFUNCTION(broadcast_to_room),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "void send_to_entity(Entity &in target, const string &in text, const bool prepend_self)",
            asFUNCTION(send_to_entity),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        engine.SetDefaultNamespace("");

        rc = engine.RegisterGlobalFunction(
            "void println(const string &in text)",
            asFUNCTION(println),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterGlobalFunction(
            "void mprintln(const string &in text)",
            asFUNCTION(mprintln),
            asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        return result;
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::emit_to_room(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "emit_to_room", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AEntity * const room_entity = reinterpret_cast<AEntity *>(
            gen_ptr->GetArgObject(0));
        AString * const raw_text = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(1));
        const bool prepend_self = *(bool*)gen_ptr->GetAddressOfArg(2);

        if ((not room_entity) or (not raw_text))
        {
            throw AngelException(
                "AngelScript passed null pointers to us",
                AS_OBJECT_TYPE_NAME,
                EMIT_TO_ROOM_METHOD_SIG);
        }

        send_event(
            engine_ptr,
            EMIT_TO_ROOM_METHOD_SIG,
            *room_entity,
            true,
            *raw_text,
            prepend_self,
            true);
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::broadcast_to_room(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "broadcast_to_room", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AEntity * const room_entity = reinterpret_cast<AEntity *>(
            gen_ptr->GetArgObject(0));
        AString * const raw_text = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(1));
        const bool prepend_self = *(bool*)gen_ptr->GetAddressOfArg(2);

        if ((not room_entity) or (not raw_text))
        {
            throw AngelException(
                "AngelScript passed null pointers to us",
                AS_OBJECT_TYPE_NAME,
                BROADCAST_TO_ROOM_METHOD_SIG);
        }

        send_event(
            engine_ptr,
            BROADCAST_TO_ROOM_METHOD_SIG,
            *room_entity,
            true,
            *raw_text,
            prepend_self,
            false);
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::send_to_entity(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "send_to_entity", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AEntity * const entity = reinterpret_cast<AEntity *>(
            gen_ptr->GetArgObject(0));
        AString * const raw_text = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(1));
        const bool prepend_self = *(bool*)gen_ptr->GetAddressOfArg(2);

        if ((not entity) or (not raw_text))
        {
            throw AngelException(
                "AngelScript passed null pointers to us",
                AS_OBJECT_TYPE_NAME,
                SEND_TO_ENTITY_METHOD_SIG);
        }

        send_event(
            engine_ptr,
            BROADCAST_TO_ROOM_METHOD_SIG,
            *entity,
            false,
            *raw_text,
            prepend_self);
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::println(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "println", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AString * const raw_text = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(0));

        if (not raw_text)
        {
            throw AngelException(
                "AngelScript passed null pointers to us",
                AS_OBJECT_TYPE_NAME,
                PRINTLN_METHOD_SIG);
        }

        text::ExternalTextLine text_line;

        try
        {

            ScriptContext * const script_context_ptr =
                ScriptUtilities::get_my_script_context(engine_ptr);

            // Convert text to ExternalText
            //
            const primitives::Result convert_result =
                primitives::PrimitivesAccess::instance()->
                    system_prims().to_external_text(
                        script_context_ptr->get_security_context(),
                        raw_text->export_to_string(),
                        text_line);

            if (not convert_result.is_success())
            {
                throw AngelException(
                    "Failed to convert text to ExternalText",
                    convert_result,
                    AS_OBJECT_TYPE_NAME,
                    PRINTLN_METHOD_SIG);
            }
            else
            {
                const bool success_send =
                    script_context_ptr->get_output_channel() ?
                        script_context_ptr->get_output_channel()->send_item(
                            text_line) : false;

                if (not success_send)
                {
                    // Should never happen
                    throw AngelException(
                        "Output Channel is closed or blocked",
                        convert_result,
                        AS_OBJECT_TYPE_NAME,
                        PRINTLN_METHOD_SIG);
                }
            }
        }
        catch (std::exception &ex)
        {
            text::ExternalText::clear_text_line(text_line);

            ScriptUtilities::set_exception_info(engine_ptr, ex);
            throw;
        }
        catch (...)
        {
            text::ExternalText::clear_text_line(text_line);

            ScriptUtilities::set_exception_info(engine_ptr);
            throw;
        }
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::mprintln(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "mprintln", "gen_ptr is null");
            return;
        }

        asIScriptEngine * const engine_ptr = gen_ptr->GetEngine();

        AString * const raw_text = reinterpret_cast<AString *>(
            gen_ptr->GetArgObject(0));

        if (not raw_text)
        {
            throw AngelException(
                "AngelScript passed null pointers to us",
                AS_OBJECT_TYPE_NAME,
                MPRINTLN_METHOD_SIG);
        }

        text::ExternalTextMultiline text_lines;

        try
        {
            ScriptContext * const script_context_ptr =
                ScriptUtilities::get_my_script_context(engine_ptr);

            // Convert text to ExternalText
            //
            const primitives::Result convert_result =
                primitives::PrimitivesAccess::instance()->
                    system_prims().to_external_text_multiline_unformatted(
                        script_context_ptr->get_security_context(),
                        raw_text->export_to_string(),
                        text_lines);

            if (not convert_result.is_success())
            {
                throw AngelException(
                    "Failed to convert text to ExternalText",
                    convert_result,
                    AS_OBJECT_TYPE_NAME,
                    MPRINTLN_METHOD_SIG);
            }
            else
            {
                events::TextChannel * const output_channel =
                    script_context_ptr->get_output_channel();
                bool success_send = output_channel;

                if (success_send)
                {
                    // Send each line one at a time until we're done or errored
                    // out.
                    //
                    for (text::ExternalTextMultiline::iterator line_iter =
                            text_lines.begin();
                        line_iter != text_lines.end();
                        ++line_iter)
                    {
                        success_send = output_channel->send_item(*line_iter);

                        if (not success_send)
                        {
                            break;
                        }
                    }
                }

                if (not success_send)
                {
                    // Should never happen
                    throw AngelException(
                        "Output Channel is closed or blocked",
                        convert_result,
                        AS_OBJECT_TYPE_NAME,
                        PRINTLN_METHOD_SIG);
                }
            }
        }
        catch (std::exception &ex)
        {
            text::ExternalText::clear_text_lines(text_lines);

            ScriptUtilities::set_exception_info(engine_ptr, ex);
            throw;
        }
        catch (...)
        {
            text::ExternalText::clear_text_lines(text_lines);

            ScriptUtilities::set_exception_info(engine_ptr);
            throw;
        }
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::send_event(
        asIScriptEngine *const engine_ptr,
        const std::string &method,
        AEntity &entity,
        const bool entity_is_room,
        AString &raw_text,
        const bool prepend_self,
        const bool exclude_requester)
    {
        text::ExternalTextLine text_line;

        try
        {
            security::Context * const security_context_ptr =
                ScriptUtilities::get_my_security_context(engine_ptr);

            // Convert text to ExternalText
            //
            const primitives::Result convert_result =
                primitives::PrimitivesAccess::instance()->
                    system_prims().to_external_text(
                        *security_context_ptr,
                        raw_text.export_to_string(),
                        text_line);

            if (not convert_result.is_success())
            {
                throw AngelException(
                    "Failed to convert text to ExternalText",
                    convert_result,
                    AS_OBJECT_TYPE_NAME,
                    method);
            }
            else
            {
                // Insert name in the front if requested.
                //
                if (prepend_self)
                {
                    text::ExternalIdText *id_text_ptr = 0;

                    const primitives::Result id_make_result =
                        primitives::PrimitivesAccess::instance()->
                            system_prims().make_id_text(
                                *security_context_ptr,
                                security_context_ptr->get_requester(),
                                id_text_ptr);

                    if ((not id_text_ptr) or (not id_make_result.is_success()))
                    {
                        delete id_text_ptr;
                        id_text_ptr = 0;

                        throw AngelException(
                            "Failed to construct ID Text with requester",
                            id_make_result,
                            AS_OBJECT_TYPE_NAME,
                            method);
                    }
                    else
                    {
                        text_line.insert(text_line.begin(), id_text_ptr);
                        id_text_ptr = 0;
                    }
                }

                // Send off the text.
                //
                const primitives::Result prim_result = (entity_is_room ?
                    primitives::PrimitivesAccess::instance()->
                        event_prims().send_text_to_room(
                            *security_context_ptr,
                            entity.get_id(),
                            text_line,
                            exclude_requester) :
                    primitives::PrimitivesAccess::instance()->
                        event_prims().send_text_to_entity(
                            *security_context_ptr,
                            entity.get_id(),
                            text_line));

                if (not prim_result.is_success())
                {
                    throw AngelException(
                        "",
                        prim_result,
                        AS_OBJECT_TYPE_NAME,
                        method);
                }
                else
                {
                    text::ExternalText::clear_text_line(text_line);
                }
            }
        }
        catch (std::exception &ex)
        {
            text::ExternalText::clear_text_line(text_line);

            ScriptUtilities::set_exception_info(engine_ptr, ex);
            throw;
        }
        catch (...)
        {
            text::ExternalText::clear_text_line(text_line);

            ScriptUtilities::set_exception_info(engine_ptr);
            throw;
        }
    }

    // ----------------------------------------------------------------------
    void InputOutputOps::check_register_rc(
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
