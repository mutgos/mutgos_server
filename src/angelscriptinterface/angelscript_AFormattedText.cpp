/*
 * angelscript_AFormattedText.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"
#include "utilities/memory_ThreadVirtualHeapManager.h"
#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"

#include "angelscript_SimpleGCObject.h"
#include "angelscript_AString.h"
#include "angelscript_AEntity.h"
#include "angelscript_AngelException.h"
#include "angelscript_ScriptUtilities.h"
#include "primitives/primitives_SystemPrims.h"
#include "primitives/primitives_PrimitivesAccess.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalFormattedText.h"
#include "text/text_ExternalIdText.h"

#include "angelscript_AFormattedText.h"

#define MAX_TEXT_ELEMENTS 2048

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "FormattedText";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool AFormattedText::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        // Register object type itself.
        //
        rc = engine.RegisterObjectType(
            AS_OBJECT_TYPE_NAME.c_str(),
            sizeof(AFormattedText),
            asOBJ_REF | asOBJ_GC);
        check_register_rc(rc, __LINE__, result);

        // Register normal and copy constructor, generic so we can get the
        // engine.
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "FormattedText@ FormattedText()",
            asFUNCTION(formatted_text_factory), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "FormattedText@ FormattedText(const FormattedText &in)",
            asFUNCTION(formatted_text_factory_copy), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        // Since this class supports references, we need to register
        // the garbage collection stuff
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETREFCOUNT,
            "int f()",
            asMETHOD(AFormattedText, get_ref_count), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_SETGCFLAG,
            "void f()",
            asMETHOD(AFormattedText, set_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETGCFLAG,
            "bool f()",
            asMETHOD(AFormattedText, get_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            asMETHOD(AFormattedText, enum_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ADDREF,
            "void f()",
            asMETHOD(AFormattedText, add_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASE,
            "void f()",
            asMETHOD(AFormattedText, release_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            asMETHOD(AFormattedText, release_all_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        // Register object methods
        //
        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void assign(const FormattedText &in text)",
            asMETHODPR(AFormattedText, assign, (const AFormattedText &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "FormattedText & opAssign(const FormattedText &in text)",
            asMETHODPR(AFormattedText, operator=, (const AFormattedText &), AFormattedText &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "FormattedText & append_formatted(const string &in text)",
            asMETHODPR(AFormattedText, append_formatted, (const AString &), AFormattedText &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "FormattedText & append_plain(const string &in text)",
            asMETHODPR(AFormattedText, append_plain, (const AString &), AFormattedText &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "FormattedText & append_entity(const Entity &in entity)",
            asMETHODPR(AFormattedText, append_entity, (AEntity &), AFormattedText &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        return result;
    }

    // ----------------------------------------------------------------------
    void AFormattedText::formatted_text_factory(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "formatted_text_factory",
                "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();

        *(AFormattedText **)gen_ptr->GetAddressOfReturnLocation() =
            new AFormattedText(engine);
    }

    // ----------------------------------------------------------------------
    void AFormattedText::formatted_text_factory_copy(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "formatted_text_factory_copy",
                "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();
        void * const raw_arg_ptr = gen_ptr->GetArgObject(0);

        if (raw_arg_ptr)
        {
            AFormattedText * const text_ptr = new AFormattedText(engine);

            try
            {
                text_ptr->assign(*reinterpret_cast<AFormattedText *>(
                    raw_arg_ptr));
                *(AFormattedText **)gen_ptr->GetAddressOfReturnLocation() =
                    text_ptr;
            }
            catch (std::bad_alloc &ex)
            {
                text_ptr->release_ref();
                throw;
            }
        }
    }

    // ----------------------------------------------------------------------
    AFormattedText::AFormattedText(asIScriptEngine *engine)
      : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, true)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AFormattedText),
            true);
    }

    // ----------------------------------------------------------------------
    AFormattedText::~AFormattedText()
    {
        memory::ThreadVirtualHeapManager::external_free(sizeof(AFormattedText)
            + text::ExternalText::total_mem_used(text_line));
        text::ExternalText::clear_text_line(text_line);
    }

    // ----------------------------------------------------------------------
    AFormattedText& AFormattedText::operator=(const AFormattedText &rhs)
    {
        assign(rhs);
        return *this;
    }

    // ----------------------------------------------------------------------
    void AFormattedText::assign(const AFormattedText &text)
    {
        memory::ThreadVirtualHeapManager::external_free(
            text::ExternalText::total_mem_used(text_line));
        text::ExternalText::clear_text_line(text_line);

        memory::ThreadVirtualHeapManager::external_malloc(
            text::ExternalText::total_mem_used(text.get_text_line()),
            true);

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        text_line = text::ExternalText::clone_text_line(text.get_text_line());
    }

    // ----------------------------------------------------------------------
    AFormattedText& AFormattedText::append_formatted(const AString &str)
    {
        ScriptContext * const script_context_ptr =
            ScriptUtilities::get_my_script_context(engine_ptr);
        text::ExternalTextLine appended_text;

        primitives::PrimitivesAccess::instance()->system_prims().
            to_external_text(
                script_context_ptr->get_security_context(),
                str.export_to_string(),
                appended_text);

        memory::ThreadVirtualHeapManager::external_malloc(
            text::ExternalText::total_mem_used(appended_text),
            true);

        text_line.insert(
            text_line.end(),
            appended_text.begin(),
            appended_text.end());

        if (text_line.size() > MAX_TEXT_ELEMENTS)
        {
            const std::string message =
                "append_formatted(): Exceeded maximum allowed elements";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        return *this;
    }

    // ----------------------------------------------------------------------
    AFormattedText &AFormattedText::append_plain(const AString &str)
    {
        if (text_line.size() >= MAX_TEXT_ELEMENTS)
        {
            const std::string message =
                "append_plain(): Exceeded maximum allowed elements";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }
        else
        {
            text_line.push_back(new text::ExternalPlainText(
                str.export_to_string()));

            memory::ThreadVirtualHeapManager::external_malloc(
                text_line.back()->mem_used(),
                true);
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    AFormattedText &AFormattedText::append_entity(AEntity &entity)
    {
        if (text_line.size() >= MAX_TEXT_ELEMENTS)
        {
            const std::string message =
                "append_entity(): Exceeded maximum allowed elements";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }
        else
        {
            ScriptContext * const script_context_ptr =
                ScriptUtilities::get_my_script_context(engine_ptr);
            text::ExternalIdText *id_text_ptr = 0;
            const primitives::Result result =
                primitives::PrimitivesAccess::instance()->system_prims().
                    make_id_text(
                      script_context_ptr->get_security_context(),
                      entity.get_id(),
                      id_text_ptr,
                      true);

            if (not result.is_success())
            {
                throw AngelException(
                    "Unable to create ID Text",
                    result,
                    AS_OBJECT_TYPE_NAME,
                    "append_entity");
            }
            else
            {
                // If we got here, then no exceptions were thrown trying to
                // make the IdText.  Append it.
                //
                text_line.push_back(id_text_ptr);

                memory::ThreadVirtualHeapManager::external_malloc(
                    text_line.back()->mem_used(),
                    true);
            }
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    void AFormattedText::transfer(text::ExternalTextLine &destination)
    {
        memory::ThreadVirtualHeapManager::external_free(
            text::ExternalText::total_mem_used(text_line));

        destination.insert(
            destination.end(),
            text_line.begin(),
            text_line.end());

        text_line.clear();
    }

    // ----------------------------------------------------------------------
    void AFormattedText::check_register_rc(
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