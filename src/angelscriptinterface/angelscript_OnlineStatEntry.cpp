/*
 * angelscript_OnlineStatEntry.cpp
 */

#include <string>
#include <angelscript.h>

#include "dbtypes/dbtype_Id.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "angelscript_SimpleGCObject.h"
#include "osinterface/osinterface_OsTypes.h"
#include "utilities/memory_VirtualHeapAllocator.h"

#include "angelscript_OnlineStatEntry.h"
#include "angelscript_AEntity.h"

#include "comminterface/comm_SessionStats.h"

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "OnlineStatEntry";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool OnlineStatEntry::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        // Register object type itself.
        //
        rc = engine.RegisterObjectType(
            AS_OBJECT_TYPE_NAME.c_str(),
            sizeof(OnlineStatEntry),
            asOBJ_REF | asOBJ_GC);
        check_register_rc(rc, __LINE__, result);

        // Register typedefs
        //
        rc = engine.RegisterTypedef("TimeDiff", "int64");
        check_register_rc(rc, __LINE__, result);

        // Register normal and copy constructor, generic so we can get the
        // engine.
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "OnlineStatEntry@ OnlineStatEntry()",
            asFUNCTION(online_stat_entry_factory), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "OnlineStatEntry@ OnlineStatEntry(const OnlineStatEntry &in)",
            asFUNCTION(online_stat_entry_factory_copy), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        // Since this string class supports references, we need to register
        // the garbage collection stuff
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETREFCOUNT,
            "int f()",
            asMETHOD(AEntity, get_ref_count), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_SETGCFLAG,
            "void f()",
            asMETHOD(AEntity, set_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_GETGCFLAG,
            "bool f()",
            asMETHOD(AEntity, get_gc_flag), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            asMETHOD(AEntity, enum_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_ADDREF,
            "void f()",
            asMETHOD(AEntity, add_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASE,
            "void f()",
            asMETHOD(AEntity, release_ref), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            asMETHOD(AEntity, release_all_references), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        // Register object methods
        //
        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "OnlineStatEntry & opAssign(const OnlineStatEntry &in entry)",
            asMETHODPR(OnlineStatEntry, operator=, (const OnlineStatEntry &), OnlineStatEntry &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool opEquals(const OnlineStatEntry &in rhs) const",
            asMETHODPR(OnlineStatEntry, operator==, (const OnlineStatEntry &) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_valid(void) const",
            asMETHODPR(OnlineStatEntry, is_valid, (void) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "Entity@ get_entity(void)",
            asMETHODPR(OnlineStatEntry, get_entity, (void), AEntity *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_connected(void)",
            asMETHODPR(OnlineStatEntry, is_connected, (void) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "TimeDiff get_time_online_secs(void)",
            asMETHODPR(OnlineStatEntry, get_time_online_secs, (void) const, asINT64), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "TimeDiff get_idle_time_secs(void)",
            asMETHODPR(OnlineStatEntry, get_idle_time_secs, (void) const, asINT64), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        return result;
    }

    // ----------------------------------------------------------------------
    void OnlineStatEntry::online_stat_entry_factory(asIScriptGeneric *gen_ptr)
    {
        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();

        *(OnlineStatEntry **)gen_ptr->GetAddressOfReturnLocation() =
            new OnlineStatEntry(engine);
    }

    // ----------------------------------------------------------------------
    void OnlineStatEntry::online_stat_entry_factory_copy(
        asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "online_stat_entry_factory_copy",
                "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        void * const raw_arg_ptr = gen_ptr->GetArgObject(0);

        if (raw_arg_ptr)
        {
            OnlineStatEntry * const rhs_ptr =
                reinterpret_cast<OnlineStatEntry *>(raw_arg_ptr);
            OnlineStatEntry * const new_stat_ptr = new OnlineStatEntry(*rhs_ptr);
            *(OnlineStatEntry **)gen_ptr->GetAddressOfReturnLocation() =
                new_stat_ptr;
        }
    }

    // ----------------------------------------------------------------------
    OnlineStatEntry::OnlineStatEntry(asIScriptEngine *engine)
        : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME),
          connected(false),
          elapsed_secs_connection(0),
          elapsed_secs_idle(0)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(OnlineStatEntry),
            true);
    }

    // ----------------------------------------------------------------------
    OnlineStatEntry::OnlineStatEntry(const OnlineStatEntry &rhs)
        : SimpleGCObject(rhs, AS_OBJECT_TYPE_NAME),
          entity_id(rhs.entity_id),
          connected(rhs.connected),
          elapsed_secs_connection(rhs.elapsed_secs_connection),
          elapsed_secs_idle(rhs.elapsed_secs_idle)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(OnlineStatEntry),
            true);
    }

    // ----------------------------------------------------------------------
    OnlineStatEntry::OnlineStatEntry(
        asIScriptEngine *engine,
        const comm::SessionStats &stats)
        : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME),
          entity_id(stats.get_entity_id()),
          connected(stats.is_connected()),
          elapsed_secs_connection(
              stats.get_connection_time().get_relative_seconds()),
          elapsed_secs_idle(
              stats.get_last_activity_time().get_relative_seconds())
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(OnlineStatEntry),
            true);
    }

    // ----------------------------------------------------------------------
    OnlineStatEntry::~OnlineStatEntry()
    {
        memory::ThreadVirtualHeapManager::external_free(sizeof(OnlineStatEntry));
    }

    // ----------------------------------------------------------------------
    OnlineStatEntry &OnlineStatEntry::operator=(const OnlineStatEntry &rhs)
    {
        entity_id = rhs.entity_id;
        connected = rhs.connected;
        elapsed_secs_connection = rhs.elapsed_secs_connection;
        elapsed_secs_idle = rhs.elapsed_secs_idle;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool OnlineStatEntry::operator==(const OnlineStatEntry &rhs) const
    {
        return ((entity_id == rhs.entity_id) and (connected == rhs.connected)
            and (elapsed_secs_connection == rhs.elapsed_secs_connection)
            and (elapsed_secs_idle == rhs.elapsed_secs_idle));
    }

    // ----------------------------------------------------------------------
    bool OnlineStatEntry::is_valid(void) const
    {
        return not entity_id.is_default();
    }

    // ----------------------------------------------------------------------
    AEntity *OnlineStatEntry::get_entity(void)
    {
        memory::ThreadVirtualHeapManager::check_overallocation(true);

        return new AEntity(engine_ptr, entity_id);
    }

    // ----------------------------------------------------------------------
    bool OnlineStatEntry::is_connected(void) const
    {
        return connected;
    }

    // ----------------------------------------------------------------------
    asINT64 OnlineStatEntry::get_time_online_secs(void) const
    {
        return (asINT64) elapsed_secs_connection;
    }

    // ----------------------------------------------------------------------
    asINT64 OnlineStatEntry::get_idle_time_secs(void) const
    {
        return (asINT64) elapsed_secs_idle;
    }

    // ----------------------------------------------------------------------
    void OnlineStatEntry::check_register_rc(
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
