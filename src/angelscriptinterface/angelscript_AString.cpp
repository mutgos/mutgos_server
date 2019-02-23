/*
 * angelscript_AString.cpp
 */

#include <string>
#include <angelscript.h>

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "angelscript_AString.h"
#include "angelscript_AngelException.h"
#include "angelscript_ScriptUtilities.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

namespace
{
    // While most classes will have capital letters, this one is lowercase
    // to remain consistent with the addon version.
    const std::string AS_OBJECT_TYPE_NAME = "string";
    const std::string TRUE_VALUE = "true";
    const std::string FALSE_VALUE = "false";
}

// TODO Retrofit to new exception throwing and handling

namespace mutgos
{
namespace angelscript
{
    // Statics.
    //
    const AString::StringPos AString::NOT_FOUND = std::string::npos;

    // ----------------------------------------------------------------------
    // We assume we have C++11 support.
    // String factory registration is done elsewhere.
    bool AString::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        // Register object type itself.
        //
        rc = engine.RegisterObjectType(
            AS_OBJECT_TYPE_NAME.c_str(),
            sizeof(AString),
            asOBJ_REF | asOBJ_GC);
        check_register_rc(rc, __LINE__, result);

        // Register normal and copy constructor, generic so we can get the
        // engine.
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "string@ string()",
            asFUNCTION(string_factory), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "string@ string(const string &in)",
            asFUNCTION(string_factory_copy), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        // Since this string class supports references, we need to register
        // the garbage collection stuff
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETREFCOUNT,
            "int f()",
            asMETHOD(AString, get_ref_count), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_SETGCFLAG,
            "void f()",
            asMETHOD(AString, set_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETGCFLAG,
            "bool f()",
            asMETHOD(AString, get_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            asMETHOD(AString, enum_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ADDREF,
            "void f()",
            asMETHOD(AString, add_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASE,
            "void f()",
            asMETHOD(AString, release_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            asMETHOD(AString, release_all_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        // Register typedefs
        //
        rc = engine.RegisterTypedef("StringPos", "uint");
        check_register_rc(rc, __LINE__, result);
        // TODO put this somewhere common
        rc = engine.RegisterTypedef("MG_UnsignedInt", "uint");
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterTypedef("MG_LongUnsignedInt", "uint64");
        check_register_rc(rc, __LINE__, result);

        // Register object methods
        //
        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void assign(const string &in str)",
            asMETHODPR(AString, assign, (const AString &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string & opAssign(const string &in str)",
            asMETHODPR(AString, operator=, (const AString &), AString &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "StringPos size(void) const",
            asMETHODPR(AString, size, (void) const, StringPos), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool empty(void) const",
            asMETHODPR(AString, empty, (void) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void clear(void)",
            asMETHODPR(AString, clear, (void), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool opEquals(const string &in rhs) const",
            asMETHODPR(AString, operator==, (const AString &) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "int opCmp(const string &in rhs) const",
            asMETHODPR(AString, compare, (const AString &) const, int), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ opAdd(const string &in rhs) const",
            asMETHODPR(AString, concatonate, (const AString &) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string & opAddAssign(const string &in rhs)",
            asMETHODPR(AString, operator+=, (const AString &), AString &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void append(const string &in rhs)",
            asMETHODPR(AString, append, (const AString &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);
/** TODO May not be needed since uint64 can upconvert
//
        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ opAdd(const MG_UnsignedInt rhs) const",
            asMETHODPR(AString, concatonate, (const MG_UnsignedInt) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string & opAddAssign(const MG_UnsignedInt rhs)",
            asMETHODPR(AString, operator+=, (const MG_UnsignedInt), AString &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void append(const MG_UnsignedInt rhs)",
            asMETHODPR(AString, append, (const MG_UnsignedInt), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);
*/
        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ opAdd(const MG_LongUnsignedInt rhs) const",
            asMETHODPR(AString, concatonate, (const MG_UnsignedInt) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string & opAddAssign(const MG_LongUnsignedInt rhs)",
            asMETHODPR(AString, operator+=, (const MG_UnsignedInt), AString &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void append(const MG_LongUnsignedInt rhs)",
            asMETHODPR(AString, append, (const MG_UnsignedInt), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ opAdd(const bool rhs) const",
            asMETHODPR(AString, concatonate, (const bool) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string & opAddAssign(const bool rhs)",
            asMETHODPR(AString, operator+=, (const bool), AString &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void append(const bool rhs)",
            asMETHODPR(AString, append, (const bool), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void fill(const string &in str, const StringPos repeats)",
            asMETHODPR(AString, fill, (const AString &, const StringPos), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "StringPos find(const string &in str) const",
            asMETHODPR(AString, find, (const AString &) const, StringPos), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "StringPos find(const string &in str, const StringPos pos) const",
            asMETHODPR(AString, find, (const AString &, const StringPos) const, StringPos), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "StringPos find_last(const string &in str) const",
            asMETHODPR(AString, find_last, (const AString &) const, StringPos), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "StringPos find_last(const string &in str, const StringPos pos) const",
            asMETHODPR(AString, find_last, (const AString &, const StringPos) const, StringPos), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ substring(const StringPos start_pos) const",
            asMETHODPR(AString, substring, (const StringPos) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ substring(const StringPos start_pos, const StringPos len) const",
            asMETHODPR(AString, substring, (const StringPos, const StringPos) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ char_at(const StringPos pos) const",
            asMETHODPR(AString, char_at, (const StringPos) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ opIndex(const StringPos pos) const",
            asMETHODPR(AString, char_at, (const StringPos) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void erase(const StringPos start_pos)",
            asMETHODPR(AString, erase, (const StringPos), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void erase(const StringPos start_pos, const StringPos len)",
            asMETHODPR(AString, erase, (const StringPos, const StringPos), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool equals(const string &in rhs) const",
            asMETHODPR(AString, equals, (const AString &) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool equals_ignore_case(const string &in rhs) const",
            asMETHODPR(AString, equals_ignore_case, (const AString &) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ to_upper(void) const",
            asMETHODPR(AString, to_upper, (void) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ to_lower(void) const",
            asMETHODPR(AString, to_lower, (void) const, AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void strip(void)",
            asMETHODPR(AString, strip, (void), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        return result;
    }

    // ----------------------------------------------------------------------
    AString::AString(
        asIScriptEngine *engine,
        const bool register_with_gc)
        : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, register_with_gc)
    {
        // This occurs potentially on a stack, but we are counting it
        // towards the heap usage.
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AString),
            true);
    }

    // ----------------------------------------------------------------------
    AString::AString(asIScriptEngine *engine)
      : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, true)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AString),
            true);
    }

    // ----------------------------------------------------------------------
    AString::AString(asIScriptEngine *engine, const char *data, size_t length)
        : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, false),
          string_value(data, length)
    {
        // This occurs potentially on a stack, but we are counting it
        // towards the heap usage.
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AString),
            true);
    }

    // ----------------------------------------------------------------------
    AString::~AString()
    {
        memory::ThreadVirtualHeapManager::external_free(sizeof(AString));
    }

    // ----------------------------------------------------------------------
    void AString::string_factory(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "string_factory", "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();

        *(AString **)gen_ptr->GetAddressOfReturnLocation() =
            new AString(engine);
    }

    // ----------------------------------------------------------------------
    void AString::string_factory_copy(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "string_factory_copy", "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();
        void * const raw_arg_ptr = gen_ptr->GetArgObject(0);

        if (raw_arg_ptr)
        {
            AString * const string_ptr = new AString(engine);
            string_ptr->assign(*reinterpret_cast<AString *>(raw_arg_ptr));
            *(AString **)gen_ptr->GetAddressOfReturnLocation() = string_ptr;
        }
    }

    // ----------------------------------------------------------------------
    const char *AString::get_raw_data(void) const
    {
        return string_value.c_str();
    }

    // ----------------------------------------------------------------------
    void AString::assign(const AString &str)
    {
        string_value.assign(str.string_value);
    }

    // ----------------------------------------------------------------------
    void AString::assign(const std::string &str)
    {
        string_value.assign(str.c_str(), str.size());
    }

    // ----------------------------------------------------------------------
    AString &AString::operator=(const AString &rhs)
    {
        string_value = rhs.string_value;
        return *this;
    }

    // ----------------------------------------------------------------------
    std::string AString::export_to_string(void) const
    {
        return std::string(string_value.c_str(), string_value.size());
    }

    // ----------------------------------------------------------------------
    void AString::import_from_string(const std::string &str)
    {
        assign(str);
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::size(void) const
    {
        return string_value.size();
    }

    // ----------------------------------------------------------------------
    bool AString::empty(void) const
    {
        return string_value.empty();
    }

    // ----------------------------------------------------------------------
    void AString::clear(void)
    {
        string_value.clear();
        string_value.shrink_to_fit();
    }

    // ----------------------------------------------------------------------
    bool AString::operator==(const AString &rhs) const
    {
        return string_value == rhs.string_value;
    }

    // ----------------------------------------------------------------------
    bool AString::operator<(const AString &rhs) const
    {
        return string_value < rhs.string_value;
    }

    // ----------------------------------------------------------------------
    int AString::compare(const AString &rhs) const
    {
        return string_value.compare(rhs.string_value);
    }

    // ----------------------------------------------------------------------
    AString *AString::concatonate(const AString &rhs) const
    {
        AString * const result = new AString(engine_ptr);

        result->string_value.assign(string_value);
        result->string_value += rhs.string_value;

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const AString &rhs)
    {
        string_value += rhs.string_value;
        return *this;
    }

    // ----------------------------------------------------------------------
    void AString::append(const AString &rhs)
    {
        string_value.append(rhs.string_value);
    }

    // ----------------------------------------------------------------------
    AString *AString::concatonate(const MG_UnsignedInt rhs) const
    {
        AString * const result = new AString(engine_ptr);

        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);

        result->string_value.assign(string_value);
        result->string_value.append(rhs_string.c_str(), rhs_string.size());

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const MG_UnsignedInt rhs)
    {
        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);

        string_value.append(rhs_string.c_str(), rhs_string.size());

        return *this;
    }

    // ----------------------------------------------------------------------
    void AString::append(const MG_UnsignedInt rhs)
    {
        operator+=(rhs);
    }

    // ----------------------------------------------------------------------
    AString *AString::concatonate(const MG_LongUnsignedInt rhs) const
    {
        AString * const result = new AString(engine_ptr);

        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);

        result->string_value.assign(string_value);
        result->string_value.append(rhs_string.c_str(), rhs_string.size());

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const MG_LongUnsignedInt rhs)
    {
        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);

        string_value.append(rhs_string.c_str(), rhs_string.size());

        return *this;
    }

    // ----------------------------------------------------------------------
    void AString::append(const MG_LongUnsignedInt rhs)
    {
        operator+=(rhs);
    }

    // ----------------------------------------------------------------------
    AString *AString::concatonate(const bool rhs) const
    {
        AString * const result = new AString(engine_ptr);

        result->string_value.assign(string_value);

        if (rhs)
        {
            result->string_value.append(TRUE_VALUE.c_str(), TRUE_VALUE.size());
        }
        else
        {
            result->string_value.append(FALSE_VALUE.c_str(), FALSE_VALUE.size());
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const bool rhs)
    {
        if (rhs)
        {
            string_value.append(TRUE_VALUE.c_str(), TRUE_VALUE.size());
        }
        else
        {
            string_value.append(FALSE_VALUE.c_str(), FALSE_VALUE.size());
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    void AString::append(const bool rhs)
    {
        operator+=(rhs);
    }

    // ----------------------------------------------------------------------
    void AString::fill(const AString &str, const AString::StringPos repeats)
    {
        if (repeats and (not str.empty()))
        {
            for (StringPos count = 0; count < repeats; ++count)
            {
                string_value.append(str.string_value);
            }
        }
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::find(const AString &str) const
    {
        StringPos result = NOT_FOUND;
        const size_t find_result = string_value.find(str.string_value);

        if (find_result != std::string::npos)
        {
            result = (StringPos) find_result;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString::StringPos AString::find(
        const AString &str,
        const AString::StringPos pos) const
    {
        if (pos >= string_value.size())
        {
            const std::string message =
                "find(): Starting position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        StringPos result = NOT_FOUND;
        const size_t find_result = string_value.find(str.string_value, pos);

        if (find_result != std::string::npos)
        {
            result = (StringPos) find_result;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::find_last(const AString &str) const
    {
        StringPos result = NOT_FOUND;
        const size_t find_result = string_value.rfind(str.string_value);

        if (find_result != std::string::npos)
        {
            result = (StringPos) find_result;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString::StringPos AString::find_last(
        const AString &str,
        const AString::StringPos pos) const
    {
        if (pos >= string_value.size())
        {
            const std::string message =
                "find_last(): End position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        StringPos result = NOT_FOUND;
        const size_t find_result = string_value.rfind(str.string_value, pos);

        if (find_result != std::string::npos)
        {
            result = (StringPos) find_result;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::substring(const AString::StringPos start_pos) const
    {
        if (start_pos >= string_value.size())
        {
            const std::string message =
                "substring(): Start position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        AString * const result = new AString(engine_ptr);

        result->string_value.assign(string_value.substr(start_pos));

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::substring(
        const AString::StringPos start_pos,
        const AString::StringPos len) const
    {
        if (start_pos >= string_value.size())
        {
            const std::string message =
                "substring(): Start position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        AString * const result = new AString(engine_ptr);

        if (start_pos != std::string::npos)
        {
            result->string_value.assign(string_value.substr(start_pos, len));
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::char_at(const AString::StringPos pos) const
    {
        if (pos >= string_value.size())
        {
            const std::string message =
                "char_at(): Position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        AString * const result = new AString(engine_ptr);

        result->string_value = string_value[pos];

        return result;
    }

    // ---------------------------------------------------------------------
    void AString::erase(const AString::StringPos start_pos)
    {
        if (start_pos >= string_value.size())
        {
            const std::string message =
                "erase(): Start position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        string_value.erase(start_pos);
    }

    // ---------------------------------------------------------------------
    void AString::erase(
        const AString::StringPos start_pos,
        const AString::StringPos len)
    {
        if (start_pos >= string_value.size())
        {
            const std::string message =
                "erase(): Start position is out of bounds.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }

        string_value.erase(start_pos, len);
    }

    // ---------------------------------------------------------------------
    bool AString::equals(const AString &rhs) const
    {
        return operator==(rhs);
    }

    // ---------------------------------------------------------------------
    bool AString::equals_ignore_case(const AString &rhs) const
    {
        const std::string lhs_lower = text::to_lower_copy(export_to_string());
        const std::string rhs_lower = text::to_lower_copy(
            rhs.export_to_string());

        return lhs_lower == rhs_lower;
    }

    // ---------------------------------------------------------------------
    AString *AString::to_upper(void) const
    {
        AString * const result = new AString(engine_ptr);

        std::string exported_string = export_to_string();
        text::to_upper(exported_string);
        result->import_from_string(exported_string);

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::to_lower(void) const
    {
        AString * const result = new AString(engine_ptr);

        std::string exported_string = export_to_string();
        text::to_lower(exported_string);
        result->import_from_string(exported_string);

        return result;
    }

    // ---------------------------------------------------------------------
    void AString::strip(void)
    {
        std::string exported_string = export_to_string();
        text::trim(exported_string);
        import_from_string(exported_string);
    }

    // ----------------------------------------------------------------------
    void AString::check_register_rc(
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
