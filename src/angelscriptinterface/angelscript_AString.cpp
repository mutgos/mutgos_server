/*
 * angelscript_AString.cpp
 */

#include <string>
#include <angelscript.h>

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"
#include "text/text_Utf8Tools.h"
#include "utilities/mutgos_config.h"

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


//
// Important implementation note:  If you allocate a new AString within a
// call and plan to return it to AngelScript, you need to check for bad_alloc
// being thrown when adding string data to it.  If bad_alloc does get
// thrown, then release a reference and throw the exception.
//
// This is because by default all instances have a refcount of 1 (which is
// required by AngelScript).  But if an exception is thrown, AngelScript
// does not get the pointer directly returned to it, so it can't hook it in
// to anything.  By releasing the reference, you allow AngelScript to
// garbage collect it later, because the pointer does indeed have no references
// to it.
//


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
        : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, register_with_gc),
          string_size(0)
    {
        // This occurs potentially on a stack, but we are counting it
        // towards the heap usage.
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AString),
            true);
    }

    // ----------------------------------------------------------------------
    AString::AString(asIScriptEngine *engine)
      : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME, true),
        string_size(0)
    {
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

            try
            {
                string_ptr->assign(*reinterpret_cast<AString *>(raw_arg_ptr));
                *(AString **)gen_ptr->GetAddressOfReturnLocation() = string_ptr;
            }
            catch (std::bad_alloc &ex)
            {
                string_ptr->release_ref();
                throw;
            }
        }
    }

    // ----------------------------------------------------------------------
    const char *AString::get_raw_data(void) const
    {
        return string_value.c_str();
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::get_raw_size(void) const
    {
        return string_value.size();
    }

    // ----------------------------------------------------------------------
    void AString::assign(const AString &str)
    {
        string_value.assign(str.string_value);
        string_size = str.string_size;
    }

    // ----------------------------------------------------------------------
    void AString::assign(const std::string &str)
    {
        const size_t new_size = text::utf8_size(str);
        check_exceed_max(new_size);

        string_value.assign(str.c_str(), str.size());
        string_size = new_size;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator=(const AString &rhs)
    {
        string_value = rhs.string_value;
        string_size = rhs.string_size;
        return *this;
    }

    // ----------------------------------------------------------------------
    std::string AString::export_to_string(void) const
    {
        if (not string_size)
        {
            return std::string();
        }

        return std::string(string_value.c_str(), string_value.size());
    }

    // ----------------------------------------------------------------------
    void AString::import_from_string(const std::string &str)
    {
        assign(str);
    }

    // ----------------------------------------------------------------------
    void AString::import_from_string(const char *data, const size_t length)
    {
        const size_t new_size = text::utf8_size(data, length);

        check_exceed_max(new_size);
        string_value.assign(data, length);
        string_size = new_size;
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::size(void) const
    {
        return string_size;
    }

    // ----------------------------------------------------------------------
    bool AString::empty(void) const
    {
        return not string_size;
    }

    // ----------------------------------------------------------------------
    void AString::clear(void)
    {
        string_value.clear();
        string_value.shrink_to_fit();
        string_size = 0;
    }

    // ----------------------------------------------------------------------
    bool AString::operator==(const AString &rhs) const
    {
        return (string_size == rhs.string_size) and
               (string_value == rhs.string_value);
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

        try
        {
            const StringPos new_size = string_size + rhs.string_size;

            check_exceed_max(new_size);
            result->string_value.assign(string_value);
            result->string_size = string_size;
            result->string_value += rhs.string_value;
            result->string_size = new_size;
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }
        catch (AngelException &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const AString &rhs)
    {
        const size_t new_size = string_size + rhs.string_size;

        check_exceed_max(new_size);
        string_value += rhs.string_value;
        string_size = new_size;

        return *this;
    }

    // ----------------------------------------------------------------------
    void AString::append(const AString &rhs)
    {
        const size_t new_size = string_size + rhs.string_size;

        check_exceed_max(new_size);
        string_value.append(rhs.string_value);
        string_size = new_size;
    }

    // ----------------------------------------------------------------------
    AString *AString::concatonate(const MG_UnsignedInt rhs) const
    {
        AString * const result = new AString(engine_ptr);

        try
        {
            // Intermediate because of different allocators
            const std::string rhs_string = text::to_string(rhs);
            const size_t new_size = string_size + text::utf8_size(rhs_string);

            check_exceed_max(new_size);
            result->string_value.assign(string_value);
            result->string_size = string_size;
            result->string_value.append(rhs_string.c_str(), rhs_string.size());
            result->string_size = new_size;
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }
        catch (AngelException &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const MG_UnsignedInt rhs)
    {
        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);
        const size_t new_size = string_size + text::utf8_size(rhs_string);

        check_exceed_max(new_size);
        string_value.append(rhs_string.c_str(), rhs_string.size());
        string_size = new_size;

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

        try
        {
            // Intermediate because of different allocators
            const std::string rhs_string = text::to_string(rhs);
            const size_t new_size = string_size + text::utf8_size(rhs_string);

            check_exceed_max(new_size);
            result->string_value.assign(string_value);
            result->string_size = string_size;
            result->string_value.append(rhs_string.c_str(), rhs_string.size());
            result->string_size = new_size;
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }
        catch (AngelException &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const MG_LongUnsignedInt rhs)
    {
        // Intermediate because of different allocators
        const std::string rhs_string = text::to_string(rhs);
        const size_t new_size = string_size + text::utf8_size(rhs_string);

        check_exceed_max(new_size);
        string_value.append(rhs_string.c_str(), rhs_string.size());
        string_size = new_size;

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

        try
        {
            // Limit checks are done after the fact to keep the flow simpler,
            // and also because booleans can't get that big.
            //

            check_exceed_max(string_size);
            result->string_value.assign(string_value);
            result->string_size = string_size;

            if (rhs)
            {
                result->string_value.append(
                    TRUE_VALUE.c_str(),
                    TRUE_VALUE.size());

                result->string_size = string_size + text::utf8_size(TRUE_VALUE);
            }
            else
            {
                result->string_value.append(
                    FALSE_VALUE.c_str(),
                    FALSE_VALUE.size());

                result->string_size = string_size + text::utf8_size(FALSE_VALUE);
            }

            check_exceed_max(result->string_size);
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }
        catch (AngelException &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    AString &AString::operator+=(const bool rhs)
    {
        // Limit checks are done after the fact to keep the flow simpler,
        // and also because booleans can't get that big.
        //
        check_exceed_max(string_size);

        if (rhs)
        {
            string_value.append(TRUE_VALUE.c_str(), TRUE_VALUE.size());
            string_size += text::utf8_size(TRUE_VALUE);
        }
        else
        {
            string_value.append(FALSE_VALUE.c_str(), FALSE_VALUE.size());
            string_size += text::utf8_size(FALSE_VALUE);
        }

        check_exceed_max(string_size);

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
            const size_t new_size = string_size + (str.string_size * repeats);
            check_exceed_max(new_size);

            for (StringPos count = 0; count < repeats; ++count)
            {
                string_value.append(str.string_value);
                string_size += str.string_size;
            }
        }
    }

    // ----------------------------------------------------------------------
    AString::StringPos AString::find(const AString &str) const
    {
        StringPos result = NOT_FOUND;
        const size_t find_result = text::utf8_find(
            get_raw_data(),
            get_raw_size(),
            str.get_raw_data(),
            str.get_raw_size());

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
        if (pos >= string_size)
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
        const size_t find_result = text::utf8_find(
            get_raw_data(),
            get_raw_size(),
            str.get_raw_data(),
            str.get_raw_size(),
            pos);

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
        const size_t find_result = text::utf8_find_last(
            get_raw_data(),
            get_raw_size(),
            str.get_raw_data(),
            str.get_raw_size());

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
        if (pos >= string_size)
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

        size_t str_find_size = get_raw_size();

        if (pos < (string_size - 1))
        {
            // Not starting search at the end, so we need to convert to byte
            // index.  Get the byte right after the one we're searching, so
            // it can be used as the size.  This will exclude the undesired
            // tail end of the string.
            str_find_size = text::utf8_index_to_byte(
                get_raw_data(),
                get_raw_size(),
                pos + 1);
        }

        const size_t find_result = text::utf8_find_last(
            get_raw_data(),
            str_find_size,
            str.get_raw_data(),
            str.get_raw_size());

        if (find_result != std::string::npos)
        {
            result = (StringPos) find_result;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::substring(const AString::StringPos start_pos) const
    {
        if (start_pos >= string_size)
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

        const size_t start_byte_index = text::utf8_index_to_byte(
            get_raw_data(),
            get_raw_size(),
            start_pos);

        try
        {
            result->string_value.assign(
                string_value.substr(start_byte_index));
            result->string_size = text::utf8_size(
                result->string_value.c_str(),
                result->string_value.size());
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::substring(
        const AString::StringPos start_pos,
        const AString::StringPos len) const
    {
        if (start_pos >= string_size)
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
        const StringPos end_pos = start_pos + len;
        const size_t start_byte_index = text::utf8_index_to_byte(
            get_raw_data(),
            get_raw_size(),
            start_pos);
        size_t end_byte_index = get_raw_size();

        if (end_pos < string_size)
        {
            end_byte_index = text::utf8_index_to_byte(
                get_raw_data(),
                get_raw_size(),
                end_pos);
        }

        try
        {
            result->string_value.assign(string_value.substr(
                start_byte_index,
                end_byte_index - start_byte_index));
            result->string_size = text::utf8_size(
                result->string_value.c_str(),
                result->string_value.size());
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::char_at(const AString::StringPos pos) const
    {
        if (pos >= string_size)
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
        const std::string found_char = text::utf8_char_at(
            get_raw_data(),
            get_raw_size(),
            pos);

        try
        {
            result->string_value.assign(found_char.c_str(), found_char.size());
            result->string_size = (found_char.empty() ? 0 : 1);
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    void AString::erase(const AString::StringPos start_pos)
    {
        if (start_pos >= string_size)
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

        const size_t start_byte = text::utf8_index_to_byte(
            string_value.c_str(),
            string_value.size(),
            start_pos);
        string_value.erase(start_byte);
        string_size = text::utf8_size(get_raw_data(), get_raw_size());
    }

    // ---------------------------------------------------------------------
    void AString::erase(
        const AString::StringPos start_pos,
        const AString::StringPos len)
    {
        if (start_pos >= string_size)
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

        if (not len)
        {
            // No change.
            return;
        }

        const StringPos end_pos = start_pos + len;
        const size_t start_byte_index = text::utf8_index_to_byte(
            string_value.c_str(),
            string_value.size(),
            start_pos);
        size_t end_byte_index = get_raw_size();

        if (end_pos < string_size)
        {
            end_byte_index = text::utf8_index_to_byte(
                string_value.c_str(),
                string_value.size(),
                end_pos);
        }

        if (end_byte_index)
        {
            string_value.erase(
                start_byte_index,
                end_byte_index - start_byte_index);
        }
        else
        {
            // End went past bounds, so just erase whatever's left.
            string_value.erase(start_byte_index);
        }

        string_size = text::utf8_size(get_raw_data(), get_raw_size());
    }

    // ---------------------------------------------------------------------
    bool AString::equals(const AString &rhs) const
    {
        return operator==(rhs);
    }

    // ---------------------------------------------------------------------
    bool AString::equals_ignore_case(const AString &rhs) const
    {
        std::string lhs_lower = export_to_string();
        text::to_lower(lhs_lower);

        std::string rhs_lower = rhs.export_to_string();
        text::to_lower(rhs_lower);

        return lhs_lower == rhs_lower;
    }

    // ---------------------------------------------------------------------
    AString *AString::to_upper(void) const
    {
        AString * const result = new AString(engine_ptr);

        try
        {
            std::string exported_string = export_to_string();
            text::to_upper(exported_string);
            result->import_from_string(exported_string);
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }

        return result;
    }

    // ---------------------------------------------------------------------
    AString *AString::to_lower(void) const
    {
        AString * const result = new AString(engine_ptr);

        try
        {
            std::string exported_string = export_to_string();
            text::to_lower(exported_string);
            result->import_from_string(exported_string);
        }
        catch (std::bad_alloc &ex)
        {
            result->release_ref();
            throw;
        }

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

    // ----------------------------------------------------------------------
    void AString::check_exceed_max(const size_t size) const
    {
        if (size > config::angelscript::max_string_size())
        {
            const std::string message =
                "check_exceed_max(): String size exceeds maximum allowed.";

            ScriptUtilities::set_exception_info(
                engine_ptr,
                ScriptContext::EXCEPTION_ANGEL,
                message);

            // Out of bounds
            throw AngelException(message);
        }
    }
}
}
