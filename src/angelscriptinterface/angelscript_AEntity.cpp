/*
 * angelscript_AEntity.cpp
 */

#include <string>
#include <exception>
#include <limits>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_DatabasePrims.h"
#include "primitives/primitives_MovementPrims.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_PropertyDataType.h"

#include "angelscript_AEntity.h"
#include "angelscript_SimpleGCObject.h"
#include "angelscript_AString.h"
#include "angelscript_ScriptContext.h"
#include "angelscript_ScriptUtilities.h"
#include "angelscript_AngelException.h"

#include "add_on/scriptarray.h"

namespace
{
    const std::string AS_OBJECT_TYPE_NAME = "Entity";
    const std::string AS_ENTITY_TYPE_NAME = "EntityType";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    bool AEntity::register_methods(asIScriptEngine &engine)
    {
        bool result = true;
        int rc = 0;

        // Register enums
        //
        rc = engine.RegisterEnum(AS_ENTITY_TYPE_NAME.c_str());
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_invalid",
            dbtype::ENTITYTYPE_invalid);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_entity",
            dbtype::ENTITYTYPE_entity);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_property_entity",
            dbtype::ENTITYTYPE_property_entity);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_container_property_entity",
            dbtype::ENTITYTYPE_container_property_entity);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_region",
            dbtype::ENTITYTYPE_region);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_room",
            dbtype::ENTITYTYPE_room);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_player",
            dbtype::ENTITYTYPE_player);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_guest",
            dbtype::ENTITYTYPE_guest);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_thing",
            dbtype::ENTITYTYPE_thing);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_puppet",
            dbtype::ENTITYTYPE_puppet);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_vehicle",
            dbtype::ENTITYTYPE_vehicle);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_group",
            dbtype::ENTITYTYPE_group);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_capability",
            dbtype::ENTITYTYPE_capability);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_program",
            dbtype::ENTITYTYPE_program);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_action",
            dbtype::ENTITYTYPE_action);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_exit",
            dbtype::ENTITYTYPE_exit);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_command",
            dbtype::ENTITYTYPE_command);
        check_register_rc(rc, __LINE__, result);
        rc = engine.RegisterEnumValue(
            AS_ENTITY_TYPE_NAME.c_str(),
            "ENTITYTYPE_END",
            dbtype::ENTITYTYPE_END);
        check_register_rc(rc, __LINE__, result);


        // TODO put this somewhere common
        rc = engine.RegisterTypedef("MG_SignedInt", "int");
        check_register_rc(rc, __LINE__, result);


        // Register object type itself.
        //
        rc = engine.RegisterObjectType(
            AS_OBJECT_TYPE_NAME.c_str(),
            sizeof(AEntity),
            asOBJ_REF | asOBJ_GC);
        check_register_rc(rc, __LINE__, result);

        // Register normal and copy constructor, generic so we can get the
        // engine.
        //
        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "Entity@ Entity()",
            asFUNCTION(entity_factory), asCALL_GENERIC);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectBehaviour(
            AS_OBJECT_TYPE_NAME.c_str(),
            asBEHAVE_FACTORY,
            "Entity@ Entity(const Entity &in)",
            asFUNCTION(entity_factory_copy), asCALL_GENERIC);
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
            "bool opEquals(const Entity &in rhs) const",
            asMETHODPR(AEntity, operator==, (const AEntity &) const, bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "Entity & opAssign(const Entity &in rhs)",
            asMETHODPR(AEntity, operator=, (const AEntity &), AEntity &), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_valid()",
            asMETHODPR(AEntity, is_valid, (void), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "EntityType get_type()",
            asMETHODPR(AEntity, get_type, (void), dbtype::EntityType), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_action()",
            asMETHODPR(AEntity, is_action, (void), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_room()",
            asMETHODPR(AEntity, is_room, (void), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_thing()",
            asMETHODPR(AEntity, is_thing, (void), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_player()",
            asMETHODPR(AEntity, is_player, (void), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ to_string()",
            asMETHODPR(AEntity, to_string, (void), AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "Entity@ get_location()",
            asMETHODPR(AEntity, get_location, (void), AEntity *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "array<Entity> @get_contents()",
            asMETHODPR(AEntity, get_contents, (void), CScriptArray *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ get_name(const bool append_id)",
            asMETHODPR(AEntity, get_name, (const bool), AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void delete_entity(void)",
            asMETHODPR(AEntity, delete_entity, (void), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_security_other(const bool readwrite_flag)",
            asMETHODPR(AEntity, set_security_other, (const bool), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_security_add_admin(Entity &in admin_entity)",
            asMETHODPR(AEntity, set_security_add_admin, (AEntity &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_security_remove_admin(Entity &in admin_entity)",
            asMETHODPR(AEntity, set_security_remove_admin, (AEntity &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_application_security_other(const string &in application, const bool write_flag)",
            asMETHODPR(AEntity, set_application_security_other, (const AString &, const bool), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_application_security_add_admin(const string &in application, Entity &in admin_entity)",
            asMETHODPR(AEntity, set_application_security_add_admin, (const AString &, AEntity &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_application_security_remove_admin(const string &in application, Entity &in admin_entity)",
            asMETHODPR(AEntity, set_application_security_remove_admin, (const AString &, AEntity &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool prop_exists(const string &in property)",
            asMETHODPR(AEntity, prop_exists, (const AString &), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "bool is_prop_document(const string &in property)",
            asMETHODPR(AEntity, is_prop_document, (const AString &), bool), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "MG_SignedInt get_int_prop(const string &in property)",
            asMETHODPR(AEntity, get_int_prop, (const AString &), MG_SignedInt), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ get_string_prop(const string &in property)",
            asMETHODPR(AEntity, get_string_prop, (const AString &), AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "string@ get_prop_as_string(const string &in property)",
            asMETHODPR(AEntity, get_prop_as_string, (const AString &), AString *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "array<string> @get_document_prop(const string &in property)",
            asMETHODPR(AEntity, get_document_prop, (const AString &), CScriptArray *), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_prop(const string &in property, const MG_SignedInt value)",
            asMETHODPR(AEntity, set_prop, (const AString &, const MG_SignedInt), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        rc = engine.RegisterObjectMethod(
            AS_OBJECT_TYPE_NAME.c_str(),
            "void set_prop(const string &in property, const string &in value)",
            asMETHODPR(AEntity, set_prop, (const AString &, const AString &), void), asCALL_THISCALL);
        check_register_rc(rc, __LINE__, result);

        return result;
    }

    // ----------------------------------------------------------------------
    void AEntity::entity_factory(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "entity_factory", "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        asIScriptEngine * const engine = gen_ptr->GetEngine();

        *(AEntity **)gen_ptr->GetAddressOfReturnLocation() =
            new AEntity(engine);
    }

    // ----------------------------------------------------------------------
    void AEntity::entity_factory_copy(asIScriptGeneric *gen_ptr)
    {
        if (not gen_ptr)
        {
            LOG(fatal, "angelscript", "entity_factory_copy", "gen_ptr is null");
            return;
        }

        memory::ThreadVirtualHeapManager::check_overallocation(true);

        void * const raw_arg_ptr = gen_ptr->GetArgObject(0);

        if (raw_arg_ptr)
        {
            AEntity * const rhs_ptr = reinterpret_cast<AEntity *>(raw_arg_ptr);
            AEntity * const new_entity_ptr = new AEntity(*rhs_ptr);
            *(AEntity **)gen_ptr->GetAddressOfReturnLocation() = new_entity_ptr;
        }
    }

    // ----------------------------------------------------------------------
    AEntity::AEntity(asIScriptEngine *engine)
      : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AEntity),
            true);
    }

    // ----------------------------------------------------------------------
    AEntity::AEntity(asIScriptEngine *engine, const dbtype::Id &id)
      : SimpleGCObject(engine, AS_OBJECT_TYPE_NAME),
        entity_id(id)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AEntity),
            true);
    }

    // ----------------------------------------------------------------------
    AEntity::AEntity(const AEntity &rhs)
      : SimpleGCObject(rhs, AS_OBJECT_TYPE_NAME),
        entity_id(rhs.entity_id)
    {
        memory::ThreadVirtualHeapManager::external_malloc(
            sizeof(AEntity),
            true);
    }

    // ----------------------------------------------------------------------
    AEntity::~AEntity()
    {
        memory::ThreadVirtualHeapManager::external_free(sizeof(AEntity));
    }

    // ----------------------------------------------------------------------
    bool AEntity::operator==(const AEntity &rhs) const
    {
        return entity_id == rhs.entity_id;
    }

    // ----------------------------------------------------------------------
    AEntity &AEntity::operator=(const AEntity &rhs)
    {
        entity_id = rhs.entity_id;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_valid(void)
    {
        const primitives::Result prim_result =
            primitives::PrimitivesAccess::instance()->
                database_prims().is_entity_valid(
                    *ScriptUtilities::get_my_security_context(engine_ptr),
                    entity_id);

        return prim_result.is_success();
    }

    // ----------------------------------------------------------------------
    dbtype::EntityType AEntity::get_type(void)
    {
        dbtype::EntityType type = dbtype::ENTITYTYPE_invalid;

        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_entity_type(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        type);
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

        return type;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_action(void)
    {
        bool result = false;
        const dbtype::EntityType type = get_type();

        result = (type == dbtype::ENTITYTYPE_action) or
            (type == dbtype::ENTITYTYPE_exit) or
            (type == dbtype::ENTITYTYPE_command);

        return result;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_room(void)
    {
        bool result = false;
        const dbtype::EntityType type = get_type();

        result = (type == dbtype::ENTITYTYPE_room);

        return result;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_thing(void)
    {
        bool result = false;
        const dbtype::EntityType type = get_type();

        result = (type == dbtype::ENTITYTYPE_thing) or
            (type == dbtype::ENTITYTYPE_puppet) or
            (type == dbtype::ENTITYTYPE_vehicle);

        return result;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_player(void)
    {
        bool result = false;
        const dbtype::EntityType type = get_type();

        result = (type == dbtype::ENTITYTYPE_player) or
                 (type == dbtype::ENTITYTYPE_guest);

        return result;
    }

    // ----------------------------------------------------------------------
    AString *AEntity::to_string(void)
    {
        AString *result = 0;

        try
        {
            std::string stringed_entity;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().entity_to_string(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        stringed_entity);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "to_string()");
            }
            else
            {
                result = new AString(engine_ptr);
                result->import_from_string(stringed_entity);
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

        return result;
    }

    // ----------------------------------------------------------------------
    AEntity *AEntity::get_location(void)
    {
        AEntity *result = 0;

        try
        {
            dbtype::Id location;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_entity_location(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        location);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_location()");
            }
            else
            {
                result = new AEntity(engine_ptr, location);
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

        return result;
    }

    // ----------------------------------------------------------------------
    CScriptArray *AEntity::get_contents(void)
    {
        CScriptArray *result = 0;

        try
        {
            dbtype::Entity::IdVector contents_vector;

            primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_contents(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        primitives::DatabasePrims::CONTENTS_ALL,
                        contents_vector,
                        false);

            if (prim_result.is_security_violation())
            {
                // We can't get everything, so try and get non-actions only.
                // If that fails then we're finished and the exception can be
                // thrown.
                prim_result = primitives::PrimitivesAccess::instance()->
                    database_prims().get_contents(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        primitives::DatabasePrims::CONTENTS_NON_ACTIONS_ONLY,
                        contents_vector);
            }

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_contents()");
            }
            else
            {
                result = ScriptUtilities::create_array(
                    engine_ptr,
                    AS_OBJECT_TYPE_NAME,
                    contents_vector.size(),
                    false);

                if (result)
                {
                    size_t inserts = 0;

                    // Have the array, now populate it.
                    //
                    for (dbtype::Entity::IdVector::const_iterator iter =
                            contents_vector.begin();
                        iter != contents_vector.end();
                        ++iter)
                    {
                        result->InsertLast(new AEntity(engine_ptr, *iter));
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

        return result;
    }

    // ----------------------------------------------------------------------
    AString *AEntity::get_name(const bool append_id)
    {
        AString *result = 0;

        try
        {
            std::string entity_name;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().convert_id_to_name(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        append_id,
                        entity_name);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_name(bool)");
            }
            else
            {
                result = new AString(engine_ptr);
                result->import_from_string(entity_name);
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

        return result;
    }

    // ----------------------------------------------------------------------
    void AEntity::delete_entity(void)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().delete_entity(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "delete_entity()");
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
    void AEntity::set_security_other(const bool readwrite_flag)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_entity_security_other(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        readwrite_flag);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_security_other(bool)");
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
    void AEntity::set_security_add_admin(AEntity &admin_entity)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_entity_security_add_admin(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        admin_entity.get_id());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_security_add_admin(Entity)");
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
    void AEntity::set_security_remove_admin(AEntity &admin_entity)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_entity_security_remove_admin(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        admin_entity.get_id());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_security_remove_admin(Entity)");
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
    void AEntity::set_application_security_other(
        const AString &application,
        const bool write_flag)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_application_security_other(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        application.export_to_string(),
                        write_flag);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_application_security_other(string, bool)");
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
    void AEntity::set_application_security_add_admin(
        const AString &application,
        AEntity &admin_entity)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_application_security_add_admin(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        application.export_to_string(),
                        admin_entity.get_id());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_application_security_add_admin(string, Entity)");
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
    void AEntity::set_application_security_remove_admin(
        const AString &application,
        AEntity &admin_entity)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_application_security_remove_admin(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        application.export_to_string(),
                        admin_entity.get_id());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_application_security_remove_admin(string, Entity)");
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
    bool AEntity::prop_exists(const AString &property)
    {
        bool result = false;

        try
        {
            dbtype::PropertyDataType data_type =
                dbtype::PROPERTYDATATYPE_invalid;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property_type(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        data_type);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "prop_exists(string)");
            }
            else
            {
                result = (data_type != dbtype::PROPERTYDATATYPE_invalid);
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

        return result;
    }

    // ----------------------------------------------------------------------
    bool AEntity::is_prop_document(const AString &property)
    {
        bool result = false;

        try
        {
            dbtype::PropertyDataType data_type =
                dbtype::PROPERTYDATATYPE_invalid;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property_type(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        data_type);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "is_prop_document(string)");
            }
            else
            {
                result = (data_type == dbtype::PROPERTYDATATYPE_document);
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

        return result;
    }

    // ----------------------------------------------------------------------
    MG_SignedInt AEntity::get_int_prop(const AString &property)
    {
        MG_SignedInt result = 0;

        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        result);

            if ((prim_result.get_status() != primitives::Result::STATUS_OK) and
                (prim_result.get_status() != primitives::Result::STATUS_BAD_ARGUMENTS))
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_int_prop(string)");
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

        return result;
    }

    // ----------------------------------------------------------------------
    AString *AEntity::get_string_prop(const AString &property)
    {
        AString *result = 0;

        try
        {
            std::string string_data;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        string_data,
                        false);

            if ((prim_result.get_status() != primitives::Result::STATUS_OK) and
                (prim_result.get_status() != primitives::Result::STATUS_BAD_ARGUMENTS))
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_string_prop(string)");
            }
            else
            {
                // Either we got the string data or it was a mismatch/doesn't
                // exist. Either way it is safe to convert.
                //
                result = new AString(engine_ptr);
                result->import_from_string(string_data);
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

        return result;
    }

    // ----------------------------------------------------------------------
    AString *AEntity::get_prop_as_string(const AString &property)
    {
        AString *result = 0;

        try
        {
            std::string string_data;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        string_data,
                        true);

            if ((prim_result.get_status() != primitives::Result::STATUS_OK) and
                (prim_result.get_status() != primitives::Result::STATUS_BAD_ARGUMENTS))
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_prop_as_string(string)");
            }
            else
            {
                // Either we got the string data or it was a mismatch/doesn't
                // exist. Either way it is safe to convert.
                //
                result = new AString(engine_ptr);
                result->import_from_string(string_data);
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

        return result;
    }

    // ----------------------------------------------------------------------
    CScriptArray* AEntity::get_document_prop(const AString &property)
    {
        CScriptArray *result = 0;

        try
        {
            primitives::DatabasePrims::DocumentContents contents;

            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().get_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        contents);

            if ((prim_result.get_status() != primitives::Result::STATUS_OK) and
                (prim_result.get_status() != primitives::Result::STATUS_BAD_ARGUMENTS))
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "get_document_prop(string)");
            }
            else
            {
                result = ScriptUtilities::create_array(
                    engine_ptr,
                    "string",
                    contents.size(),
                    false);

                if (result)
                {
                    size_t inserts = 0;

                    // Have the document array, now populate it.
                    //
                    for (primitives::DatabasePrims::DocumentContents::const_iterator
                            iter = contents.begin();
                         iter != contents.end();
                         ++iter)
                    {
                        AString * const converted_line = new AString(engine_ptr);
                        converted_line->import_from_string(*iter);

                        result->InsertLast(converted_line);

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

        return result;
    }

    // ----------------------------------------------------------------------
    void AEntity::set_prop(const AString &property, const MG_SignedInt value)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        value);

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_prop(string, int)");
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
    void AEntity::set_prop(const AString &property, const AString &value)
    {
        try
        {
            const primitives::Result prim_result =
                primitives::PrimitivesAccess::instance()->
                    database_prims().set_application_property(
                        *ScriptUtilities::get_my_security_context(engine_ptr),
                        entity_id,
                        property.export_to_string(),
                        value.export_to_string());

            if (not prim_result.is_success())
            {
                throw AngelException(
                    "",
                    prim_result,
                    AS_OBJECT_TYPE_NAME,
                    "set_prop(string, string)");
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
    void AEntity::check_register_rc(
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
