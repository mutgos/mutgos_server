/*
 * dbtype_Thing.cpp
 */

#include <string>
#include <iostream>

#include "dbtypes/dbtype_Thing.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#include "logging/log_Logger.h"


namespace mutgos
{
namespace dbtype
{
    // -----------------------------------------------------------------------
    Thing::Thing()
      : ContainerPropertyEntity()
    {
    }

    // -----------------------------------------------------------------------
    Thing::Thing(const Id &id)
        : ContainerPropertyEntity(id, ENTITYTYPE_thing, 0, 0)
    {
    }

    // -----------------------------------------------------------------------
    Thing::~Thing()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Thing::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Thing(
                id,
                ENTITYTYPE_thing,
                version,
                instance);

            copy_fields(copy_ptr);

            return copy_ptr;
        }
        else
        {
            LOG(error, "dbtype", "clone",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // ----------------------------------------------------------------------
    std::string Thing::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << ContainerPropertyEntity::to_string()
            << "Thing home: " << thing_home.to_string()
            << std::endl
            << "Thing lock: " << thing_lock.to_string()
            << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Thing::set_thing_home(
        const Id &home,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            set_single_id_field(
                ENTITYFIELD_thing_home,
                thing_home,
                home);
            thing_home = home;
            notify_field_changed(ENTITYFIELD_thing_home);
            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_thing_home",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Thing::set_thing_home(const Id &home)
    {
        concurrency::WriterLockToken token(*this);

        return set_thing_home(home, token);
    }

    // ----------------------------------------------------------------------
    Id Thing::get_thing_home(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = thing_home;
        }
        else
        {
            LOG(error, "dbtype", "get_thing_home",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Thing::get_thing_home(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_thing_home(token);
    }

    // ----------------------------------------------------------------------
    bool Thing::set_thing_lock(
        const Lock &lock,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            thing_lock = lock;
            notify_field_changed(ENTITYFIELD_thing_lock);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_thing_lock",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Thing::set_thing_lock(const Lock &lock)
    {
        concurrency::WriterLockToken token(*this);

        return set_thing_lock(lock, token);
    }

    // ----------------------------------------------------------------------
    Lock Thing::get_thing_lock(concurrency::ReaderLockToken &token)
    {
        Lock result;

        if (token.has_lock(*this))
        {
            result = thing_lock;
        }
        else
        {
            LOG(error, "dbtype", "get_thing_lock",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Lock Thing::get_thing_lock(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_thing_lock(token);
    }

    // ----------------------------------------------------------------------
    Lock::LockType Thing::get_thing_lock_type(
        concurrency::ReaderLockToken &token)
    {
        Lock::LockType result = Lock::LOCK_INVALID;

        if (token.has_lock(*this))
        {
            result = thing_lock.get_lock_type();
        }
        else
        {
            LOG(error, "dbtype", "get_thing_lock_type",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Thing::get_thing_lock_id(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = thing_lock.get_id();
        }
        else
        {
            LOG(error, "dbtype", "get_thing_lock_id",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    PropertyDirectory::PathString Thing::get_thing_lock_path(
        concurrency::ReaderLockToken &token)
    {
        PropertyDirectory::PathString result;

        if (token.has_lock(*this))
        {
            result = thing_lock.get_path();
        }
        else
        {
            LOG(error, "dbtype", "get_thing_lock_path",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Thing::evalute_lock(
        concurrency::ReaderLockToken &token,
        Entity *entity_ptr,
        concurrency::WriterLockToken &entity_token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (not entity_ptr)
            {
                LOG(error, "dbtype", "evaluate_lock(property/ID)",
                    "entity_ptr is null!");
            }

            result = thing_lock.evaluate(entity_ptr, entity_token);
        }
        else
        {
            LOG(error, "dbtype", "evaluate_lock(property/ID)",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Thing::evaluate_lock(
        concurrency::ReaderLockToken &token,
        Entity *entity_ptr,
        Group *group_ptr,
        concurrency::ReaderLockToken &group_token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (not entity_ptr)
            {
                LOG(error, "dbtype", "evaluate_lock(group)",
                    "entity_ptr is null!");
            }

            if (not group_ptr)
            {
                LOG(error, "dbtype", "evaluate_lock(group)",
                    "group_ptr is null!");
            }

            result = thing_lock.evaluate(entity_ptr, group_ptr, group_token);
        }
        else
        {
            LOG(error, "dbtype", "evaluate_lock(group)",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Thing::Thing(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : ContainerPropertyEntity(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    size_t Thing::mem_used_fields(void)
    {
        size_t total_memory = ContainerPropertyEntity::mem_used_fields();

        total_memory += thing_home.mem_used()
                        +  thing_lock.mem_used();

        return total_memory;
    }

    // ----------------------------------------------------------------------
    void Thing::copy_fields(Entity *entity_ptr)
    {
        ContainerPropertyEntity::copy_fields(entity_ptr);

        Thing *cast_ptr = 0;

        // Only copy if this is also a Thing.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Thing *>(entity_ptr))) != 0))
        {
            cast_ptr->set_single_id_field(
                ENTITYFIELD_thing_home,
                cast_ptr->thing_home,
                thing_home);
            cast_ptr->thing_home = thing_home;
            cast_ptr->notify_field_changed(ENTITYFIELD_thing_home);

            cast_ptr->thing_lock = thing_lock;
            cast_ptr->notify_field_changed(ENTITYFIELD_thing_lock);
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */