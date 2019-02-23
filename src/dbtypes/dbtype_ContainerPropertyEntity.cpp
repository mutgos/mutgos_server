/*
 * dbtype_ContainerPropertyEntity.cpp
 */

#include <string>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    ContainerPropertyEntity::ContainerPropertyEntity()
      : PropertyEntity()
    {
    }

    // ----------------------------------------------------------------------
    ContainerPropertyEntity::ContainerPropertyEntity(const Id &id)
      : PropertyEntity(id, ENTITYTYPE_container_property_entity, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    ContainerPropertyEntity::~ContainerPropertyEntity()
    {
    }

    // ----------------------------------------------------------------------
    Entity *ContainerPropertyEntity::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new ContainerPropertyEntity(
                id,
                ENTITYTYPE_container_property_entity,
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
    size_t ContainerPropertyEntity::mem_used_fields(void)
    {
        size_t total_memory = PropertyEntity::mem_used_fields();

        total_memory += (contained_by.mem_used() *
            (linked_programs.size() + 1)) + sizeof(bool);

        return total_memory;
    }

    // ----------------------------------------------------------------------
    std::string ContainerPropertyEntity::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << PropertyEntity::to_string()
                  << "Contained By: " << contained_by.to_string() << std::endl
                  << "Linked Progs: ";

        for (IdSet::iterator programs_iter =
            linked_programs.begin();
             programs_iter != linked_programs.end();
             ++programs_iter)
        {
            strstream << programs_iter->to_string(true) << "  ";
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::set_contained_by(
        const Id &id,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            if (contained_by != id)
            {
                // Journal the removal of the old ID
                //
                removed_id(ENTITYFIELD_contained_by, contained_by);
                notify_field_changed(ENTITYFIELD_contained_by);

                contained_by = id;

                // Add the new ID
                //
                added_id(ENTITYFIELD_contained_by, contained_by);
                notify_field_changed(ENTITYFIELD_contained_by);
            }

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_contained_by",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::set_contained_by(const Id &id)
    {
        concurrency::WriterLockToken token(*this);

        return set_contained_by(id, token);
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_contained_by(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return contained_by;
        }
        else
        {
            LOG(error, "dbtype", "get_contained_by",
                "Using the wrong lock token!");
        }

        // Some sort of error happened.
        return Id();
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_contained_by(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_contained_by(token);
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::add_program(
        const Id &id,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            if (linked_programs.insert(id).second)
            {
                // Inserted a new program
                added_id(ENTITYFIELD_linked_programs, id);
                notify_field_changed(ENTITYFIELD_linked_programs);
            }

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "add_program",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::add_program(const Id &id)
    {
        concurrency::WriterLockToken token(*this);

        return add_program(id, token);
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::remove_program(
        const Id &id,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            IdSet::iterator prog_iter = linked_programs.find(id);

            if (prog_iter != linked_programs.end())
            {
                linked_programs.erase(prog_iter);

                // Removed a program
                removed_id(ENTITYFIELD_linked_programs, id);
                notify_field_changed(ENTITYFIELD_linked_programs);
            }

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "remove_program",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::remove_program(const Id &id)
    {
        concurrency::WriterLockToken token(*this);

        return remove_program(id, token);
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::is_program_linked(
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        bool linked = false;

        if (token.has_lock(*this))
        {
            linked = linked_programs.find(id) != linked_programs.end();
        }
        else
        {
            LOG(error, "dbtype", "is_program_linked",
                "Using the wrong lock token!");
        }

        return linked;
    }

    // ----------------------------------------------------------------------
    bool ContainerPropertyEntity::is_program_linked(const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return is_program_linked(id, token);
    }

    // ----------------------------------------------------------------------
    size_t ContainerPropertyEntity::linked_programs_size(
        concurrency::ReaderLockToken &token)
    {
        size_t result = 0;

        if (token.has_lock(*this))
        {
            result = linked_programs.size();
        }
        else
        {
            LOG(error, "dbtype", "linked_programs_size",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    size_t ContainerPropertyEntity::linked_programs_size(void)
    {
        concurrency::ReaderLockToken token(*this);

        return linked_programs_size(token);
    }

    // ----------------------------------------------------------------------
    Entity::IdVector ContainerPropertyEntity::get_linked_programs(
        concurrency::ReaderLockToken &token)
    {
        IdVector result;

        if (token.has_lock(*this))
        {
            result.reserve(linked_programs.size());
            result.insert(
                result.end(),
                linked_programs.begin(),
                linked_programs.end());
        }
        else
        {
            LOG(error, "dbtype", "get_linked_programs",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Entity::IdVector ContainerPropertyEntity::get_linked_programs(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_linked_programs(token);
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_first_linked_program(
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not linked_programs.empty())
            {
                result = *linked_programs.begin();
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_linked_program",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_first_linked_program(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_linked_program(token);
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_next_linked_program(
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            IdSet::const_iterator prog_iter = linked_programs.find(id);

            if (prog_iter != linked_programs.end())
            {
                // Found our position, get the next one.
                ++prog_iter;

                if (prog_iter != linked_programs.end())
                {
                    result = *prog_iter;
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_linked_program",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_next_linked_program(
        const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_linked_program(id, token);
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_last_linked_program(
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not linked_programs.empty())
            {
                result = *(--linked_programs.end());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_linked_program",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ContainerPropertyEntity::get_last_linked_program(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_linked_program(token);
    }

    // ----------------------------------------------------------------------
    ContainerPropertyEntity::ContainerPropertyEntity(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : PropertyEntity(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    void ContainerPropertyEntity::copy_fields(Entity *entity_ptr)
    {
        PropertyEntity::copy_fields(entity_ptr);

        ContainerPropertyEntity *cast_ptr = 0;

        // Only copy if this is also a ContainerPropertyEntity.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<ContainerPropertyEntity *>(entity_ptr)))
             != 0))
        {
            cast_ptr->set_single_id_field(
                ENTITYFIELD_contained_by,
                cast_ptr->contained_by,
                contained_by);
            cast_ptr->contained_by = contained_by;
            cast_ptr->notify_field_changed(ENTITYFIELD_contained_by);

            for (IdSet::const_iterator remove_iter =
                    cast_ptr->linked_programs.begin();
                 remove_iter != cast_ptr->linked_programs.end();
                 ++remove_iter)
            {
                cast_ptr->removed_id(
                    ENTITYFIELD_linked_programs,
                    *remove_iter);
                cast_ptr->notify_field_changed(ENTITYFIELD_linked_programs);
            }

            cast_ptr->linked_programs = linked_programs;

            for (IdSet::const_iterator add_iter =
                    cast_ptr->linked_programs.begin();
                 add_iter != cast_ptr->linked_programs.end();
                 ++add_iter)
            {
                cast_ptr->added_id(
                    ENTITYFIELD_linked_programs,
                    *add_iter);
                cast_ptr->notify_field_changed(ENTITYFIELD_linked_programs);
            }
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
