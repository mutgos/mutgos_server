/*
 * dbtype_Group.cpp
 */

#include <string>
#include <ostream>
#include <set>

#include "dbtypes/dbtype_Group.h"

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Group::Group()
      : Entity()
    {
    }

    // ----------------------------------------------------------------------
    Group::Group(const Id &id)
      : Entity (
          id,
          ENTITYTYPE_group,
          0,
          0)
    {
    }

    // ----------------------------------------------------------------------
    Group::~Group()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Group::clone(
        const Id &id,
        const Entity::VersionType version,
        const Entity::InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Group(
                id,
                ENTITYTYPE_group,
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
    size_t Group::mem_used_fields(void)
    {
        // Get parent class size
        size_t total_size = Entity::mem_used_fields();

        // Add up the sets
        total_size += sizeof(group_ids);
        total_size += sizeof(disabled_ids);

        for (GroupSet::iterator id_iter =
                group_ids.begin();
            id_iter != group_ids.end();
            ++id_iter)
        {
            total_size += id_iter->mem_used();
        }

        for (GroupSet::iterator id_iter =
                disabled_ids.begin();
            id_iter != disabled_ids.end();
            ++id_iter)
        {
            total_size += id_iter->mem_used();
        }

        return total_size;
    }

    // ----------------------------------------------------------------------
    std::string Group::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << Entity::to_string();

        strstream << "----Group IDs----" << std::endl;

        for (GroupSet::iterator id_iter =
                group_ids.begin();
            id_iter != group_ids.end();
            ++id_iter)
        {
            strstream << id_iter->to_string(true) << " ";
        }

        strstream << std::endl;

        strstream << "----Disabled Group IDs----" << std::endl;

        for (GroupSet::iterator id_iter =
                disabled_ids.begin();
            id_iter != disabled_ids.end();
            ++id_iter)
        {
            strstream << id_iter->to_string(true) << " ";
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Group::add_to_group(const Id &id_to_add)
    {
        concurrency::WriterLockToken token(*this);

        return add_to_group(id_to_add, token);
    }

    // ----------------------------------------------------------------------
    bool Group::add_to_group(
        const Id &id_to_add,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        // Cannot add ourself or an invalid ID to the group - meaningless.
        if ((id_to_add != get_entity_id()) and (not id_to_add.is_default()))
        {
            if (token.has_lock(*this))
            {
                success = group_ids.insert(id_to_add).second;

                if (success)
                {
                    added_id(ENTITYFIELD_group_ids, id_to_add);
                    notify_field_changed(ENTITYFIELD_group_ids);
                }
            }
            else
            {
                LOG(error, "dbtype", "add_to_group",
                    "Using the wrong lock token!");
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void Group::remove_from_group(const Id &id_to_remove)
    {
        concurrency::WriterLockToken token(*this);

        remove_from_group(id_to_remove, token);
    }

    // ----------------------------------------------------------------------
    void Group::remove_from_group(
        const Id &id_to_remove,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (group_ids.erase(id_to_remove))
            {
                if (disabled_ids.erase(id_to_remove))
                {
                    removed_id(ENTITYFIELD_group_disabled_ids, id_to_remove);
                    notify_field_changed(ENTITYFIELD_group_disabled_ids);
                }

                removed_id(ENTITYFIELD_group_ids, id_to_remove);
                notify_field_changed(ENTITYFIELD_group_ids);
            }
        }
        else
        {
            LOG(error, "dbtype", "remove_from_group",
                "Using the wrong lock token!");
        }
    }

    // ----------------------------------------------------------------------
    bool Group::is_in_group(const Id &id_to_check)
    {
        concurrency::ReaderLockToken token(*this);

        return is_in_group(id_to_check, token);
    }

    // ----------------------------------------------------------------------
    bool Group::is_in_group(
        const Id &id_to_check,
        concurrency::ReaderLockToken &token)
    {
        bool in_group = false;

        if (not id_to_check.is_default())
        {
            if (token.has_lock(*this))
            {
                // In the group only if it's not disabled and in the list of
                // group IDs.
                //
                in_group = (disabled_ids.find(id_to_check) == disabled_ids.end())
                           and (group_ids.find(id_to_check) != group_ids.end());
            }
            else
            {
                LOG(error, "dbtype", "is_in_group",
                    "Using the wrong lock token!");
            }
        }

        return in_group;
    }

    // ----------------------------------------------------------------------
    Entity::IdVector Group::get_all_in_group(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_all_in_group(token);
    }

    // ----------------------------------------------------------------------
    Entity::IdVector Group::get_all_in_group(
        concurrency::ReaderLockToken &token)
    {
        Entity::IdVector entries;

        if (token.has_lock(*this))
        {
            entries.reserve(group_ids.size());

            entries.insert(entries.begin(), group_ids.begin(), group_ids.end());
        }
        else
        {
            LOG(error, "dbtype", "get_all_in_group",
                "Using the wrong lock token!");
        }

        return entries;
    }

    // ----------------------------------------------------------------------
    Id Group::first_group_entry(void)
    {
        concurrency::ReaderLockToken token(*this);

        return first_group_entry(token);
    }

    // ----------------------------------------------------------------------
    Id Group::first_group_entry(
        concurrency::ReaderLockToken &token)
    {
        Id first;

        if (token.has_lock(*this))
        {
            if (not group_ids.empty())
            {
                first = *(group_ids.begin());
            }
        }
        else
        {
            LOG(error, "dbtype", "first_group_entry",
                "Using the wrong lock token!");
        }

        return first;
    }

    // ----------------------------------------------------------------------
    Id Group::next_group_entry(const Id &current_id)
    {
        concurrency::ReaderLockToken token(*this);

        return next_group_entry(current_id, token);
    }

    // ----------------------------------------------------------------------
    Id Group::next_group_entry(
        const Id &current_id,
        concurrency::ReaderLockToken &token)
    {
        Id next;

        if (token.has_lock(*this))
        {
            if (not current_id.is_default())
            {
                GroupSet::const_iterator iter = group_ids.find(current_id);

                if (iter != group_ids.end())
                {
                    ++iter;

                    if (iter != group_ids.end())
                    {
                        next = *iter;
                    }
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "next_group_entry",
                "Using the wrong lock token!");
        }

        return next;
    }

    // ----------------------------------------------------------------------
    bool Group::add_to_disabled_group(const Id &id_to_add)
    {
        concurrency::WriterLockToken token(*this);

        return add_to_disabled_group(id_to_add, token);
    }

    // ----------------------------------------------------------------------
    bool Group::add_to_disabled_group(
        const Id &id_to_add,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            // Only add if it's in the group IDs.
            if (group_ids.find(id_to_add) != group_ids.end())
            {
                success = disabled_ids.insert(id_to_add).second;

                if (success)
                {
                    added_id(ENTITYFIELD_group_disabled_ids, id_to_add);
                    notify_field_changed(ENTITYFIELD_group_disabled_ids);
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "add_to_disabled_group",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void Group::remove_from_disabled_group(const Id &id_to_remove)
    {
        concurrency::WriterLockToken token(*this);

        remove_from_disabled_group(id_to_remove, token);
    }

    // ----------------------------------------------------------------------
    void Group::remove_from_disabled_group(
        const Id &id_to_remove,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (disabled_ids.erase(id_to_remove))
            {
                removed_id(ENTITYFIELD_group_disabled_ids, id_to_remove);
                notify_field_changed(ENTITYFIELD_group_disabled_ids);
            }
        }
        else
        {
            LOG(error, "dbtype", "remove_from_disabled_group",
                "Using the wrong lock token!");
        }
    }

    // ----------------------------------------------------------------------
    bool Group::is_in_disabled_group(const Id &id_to_check)
    {
        concurrency::ReaderLockToken token(*this);

        return is_in_disabled_group(id_to_check, token);
    }

    // ----------------------------------------------------------------------
    bool Group::is_in_disabled_group(
        const Id &id_to_check,
        concurrency::ReaderLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            success = disabled_ids.find(id_to_check) != disabled_ids.end();
        }
        else
        {
            LOG(error, "dbtype", "is_in_disabled_group",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    Id Group::first_disabled_group_entry(void)
    {
        concurrency::ReaderLockToken token(*this);

        return first_disabled_group_entry(token);
    }

    // ----------------------------------------------------------------------
    Id Group::first_disabled_group_entry(
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not disabled_ids.empty())
            {
                result = *(disabled_ids.begin());
            }
        }
        else
        {
            LOG(error, "dbtype", "first_disabled_group_entry",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Group::next_disabled_group_entry(const Id &current_id)
    {
        concurrency::ReaderLockToken token(*this);

        return next_disabled_group_entry(current_id, token);
    }

    // ----------------------------------------------------------------------
    Id Group::next_disabled_group_entry(
        const Id &current_id,
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            GroupSet::const_iterator group_iter = disabled_ids.find(current_id);

            if (group_iter != disabled_ids.end())
            {
                ++group_iter;

                if (group_iter != disabled_ids.end())
                {
                    result = *group_iter;
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "next_disabled_group_entry",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Group::Group(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : Entity(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    void Group::copy_fields(Entity *entity_ptr)
    {
        Entity::copy_fields(entity_ptr);

        Group *cast_ptr = 0;

        // Only copy if this is also a PropertyEntity.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Group *>(entity_ptr))) != 0))
        {
            for (GroupSet::const_iterator remove_iter =
                    cast_ptr->group_ids.begin();
                remove_iter != cast_ptr->group_ids.end();
                ++remove_iter)
            {
                cast_ptr->removed_id(
                    ENTITYFIELD_group_ids,
                    *remove_iter);
            }

            cast_ptr->group_ids = group_ids;

            for (GroupSet::const_iterator add_iter =
                    cast_ptr->group_ids.begin();
                 add_iter != cast_ptr->group_ids.end();
                 ++add_iter)
            {
                cast_ptr->added_id(
                    ENTITYFIELD_group_ids,
                    *add_iter);
            }

            cast_ptr->disabled_ids = disabled_ids;
            cast_ptr->notify_field_changed(ENTITYFIELD_group_ids);
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
