/*
 * dbtype_ActionEntity.cpp
 */

#include <stddef.h>
#include <string>
#include <iostream>
#include <algorithm>

#include "dbtype_ActionEntity.h"
#include "dbtype_PropertyEntity.h"
#include "dbtype_Entity.h"
#include "dbtype_EntityType.h"

#include "logging/log_Logger.h"

#include "text/text_StringConversion.h"

namespace mutgos
{
namespace dbtype
{
    // TODO Add code to concatonate actions together to make searching really quick

    // ----------------------------------------------------------------------
    ActionEntity::ActionEntity()
      : PropertyEntity(),
        action_entity_lock_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    ActionEntity::ActionEntity(const Id &id)
      : PropertyEntity(id, ENTITYTYPE_action, 0, 0),
        action_entity_lock_ptr(0)
    {

    }

    // ----------------------------------------------------------------------
    ActionEntity::~ActionEntity()
    {
        delete action_entity_lock_ptr;
        action_entity_lock_ptr = 0;
    }

    // ----------------------------------------------------------------------
    Entity *ActionEntity::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new ActionEntity(
                id,
                ENTITYTYPE_action,
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
    std::string ActionEntity::to_string()
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << PropertyEntity::to_string()
                  << "Action Entity Command Aliases:";

        for (CommandList::const_iterator command_iter =
                action_entity_commands.begin();
             command_iter != action_entity_commands.end();
             ++command_iter)
        {
            strstream << std::string("   ") << *command_iter;
        }

        strstream << std::endl
                  << "Action Entity Contained By: "
                  << action_entity_contained_by.to_string() << std::endl
                  << "Action Entity Lock: "
                  << (action_entity_lock_ptr ?
                      action_entity_lock_ptr->to_string() : "None")
                  << std::endl
                  << "Action Entity Success Message: "
                  << action_entity_succ_msg  << std::endl
                  << "Action Entity Success Room Message: "
                  << action_entity_succ_room_msg  << std::endl
                  << "Action Entity Failure Message: "
                  << action_entity_fail_msg  << std::endl
                  << "Action Entity Failure Room Message: "
                  << action_entity_fail_room_msg  << std::endl
                  << "Action Entity Targets:";

        for (IdVector::const_iterator id_iter = action_entity_targets.begin();
             id_iter != action_entity_targets.end();
             ++id_iter)
        {
            strstream << std::string(" ") << id_iter->to_string(true);
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::add_action_target(
        const Id &target,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            bool found_existing = false;

            for (IdVector::const_iterator target_iter =
                    action_entity_targets.begin();
                target_iter != action_entity_targets.end();
                ++target_iter)
            {
                if (*target_iter == target)
                {
                    found_existing = true;
                    break;
                }
            }

            if (not found_existing)
            {
                action_entity_targets.push_back(target);
                added_id(ENTITYFIELD_action_targets, target);
                notify_field_changed(ENTITYFIELD_action_targets);
            }

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "add_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::add_action_target(
        const Id &target)
    {
        concurrency::WriterLockToken token(*this);

        return add_action_target(target, token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::remove_action_target(
        const Id &target,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            bool found_existing = false;

            for (IdVector::iterator target_iter =
                    action_entity_targets.begin();
                 target_iter != action_entity_targets.end();
                 ++target_iter)
            {
                if (*target_iter == target)
                {
                    found_existing = true;
                    action_entity_targets.erase(target_iter);
                    break;
                }
            }

            if (found_existing)
            {
                removed_id(ENTITYFIELD_action_targets, target);
                notify_field_changed(ENTITYFIELD_action_targets);
            }

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "remove_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::remove_action_target(
        const Id &target)
    {
        concurrency::WriterLockToken token(*this);

        return remove_action_target(target, token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::clear_action_target(concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (not action_entity_targets.empty())
            {
                for (IdVector::const_iterator iter = action_entity_targets.begin();
                    iter != action_entity_targets.end();
                    ++iter)
                {
                    removed_id(ENTITYFIELD_action_targets, *iter);
                }

                action_entity_targets.clear();
                notify_field_changed(ENTITYFIELD_action_targets);
            }

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "clear_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::clear_action_target(void)
    {
        concurrency::WriterLockToken token(*this);

        return clear_action_target(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_target(
        const Entity::IdVector &targets,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (clear_action_target(token))
            {
                action_entity_targets = targets;

                for (IdVector::const_iterator iter = action_entity_targets.begin();
                     iter != action_entity_targets.end();
                     ++iter)
                {
                    added_id(ENTITYFIELD_action_targets, *iter);
                }

                if (not action_entity_targets.empty())
                {
                    notify_field_changed(ENTITYFIELD_action_targets);
                }

                result = true;
            }
        }
        else
        {
            LOG(error, "dbtype", "set_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_target(const Entity::IdVector &targets)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_target(targets, token);
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_first_action_target(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not action_entity_targets.empty())
            {
                result = *(action_entity_targets.begin());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_first_action_target(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_action_target(token);
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_next_action_target(
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            Entity::IdVector::const_iterator iter = std::find(
                    action_entity_targets.begin(),
                    action_entity_targets.end(),
                    id);

            if (iter != action_entity_targets.end())
            {
                result = *iter;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_next_action_target(
        const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_action_target(id, token);
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_last_action_target(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not action_entity_targets.empty())
            {
                result = *(action_entity_targets.end() --);
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_action_target",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_last_action_target(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_action_target(token);
    }

    // ----------------------------------------------------------------------
    Entity::IdVector ActionEntity::get_action_targets(
        concurrency::ReaderLockToken &token)
    {
        Entity::IdVector result;

        if (token.has_lock(*this))
        {
            result = action_entity_targets;
        }
        else
        {
            LOG(error, "dbtype", "get_action_targets",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Entity::IdVector ActionEntity::get_action_targets(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_targets(token);
    }

    // ----------------------------------------------------------------------
    size_t ActionEntity::get_action_targets_size(
        concurrency::ReaderLockToken &token)
    {
        size_t result = 0;

        if (token.has_lock(*this))
        {
            result = action_entity_targets.size();
        }
        else
        {
            LOG(error, "dbtype", "get_action_targets_size",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    size_t ActionEntity::get_action_targets_size(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_targets_size(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_lock(
        const Lock &lock,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (action_entity_lock_ptr)
            {
                *action_entity_lock_ptr = lock;
            }
            else
            {
                action_entity_lock_ptr = new Lock(lock);
            }

            notify_field_changed(ENTITYFIELD_action_lock);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_lock",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_lock(const Lock &lock)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_lock(lock, token);
    }

    // ----------------------------------------------------------------------
    Lock ActionEntity::get_action_lock(concurrency::ReaderLockToken &token)
    {
        Lock result;

        if (token.has_lock(*this))
        {
            if (action_entity_lock_ptr)
            {
                result = *action_entity_lock_ptr;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_action_lock",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Lock ActionEntity::get_action_lock(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_lock(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_success_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            action_entity_succ_msg = message;
            notify_field_changed(ENTITYFIELD_action_succ_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_success_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_success_message(const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_success_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_success_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = action_entity_succ_msg;
        }
        else
        {
            LOG(error, "dbtype", "get_action_success_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_success_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_success_message(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_success_room_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            action_entity_succ_room_msg = message;
            notify_field_changed(ENTITYFIELD_action_succ_room_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_success_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_success_room_message(
        const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_success_room_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_success_room_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = action_entity_succ_room_msg;
        }
        else
        {
            LOG(error, "dbtype", "get_action_success_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_success_room_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_success_room_message(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_fail_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            action_entity_fail_msg = message;
            notify_field_changed(ENTITYFIELD_action_fail_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_fail_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_fail_message(const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_fail_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_fail_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = action_entity_fail_msg;
        }
        else
        {
            LOG(error, "dbtype", "get_action_fail_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_fail_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_fail_message(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_fail_room_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            action_entity_fail_room_msg = message;
            notify_field_changed(ENTITYFIELD_action_fail_room_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_fail_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_fail_room_message(
        const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_fail_room_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_fail_room_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = action_entity_fail_room_msg;
        }
        else
        {
            LOG(error, "dbtype", "get_action_fail_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_action_fail_room_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_fail_room_message(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_contained_by(
        const Id &container,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            set_single_id_field(
                ENTITYFIELD_action_contained_by,
                action_entity_contained_by,
                container);
            action_entity_contained_by = container;
            notify_field_changed(ENTITYFIELD_action_contained_by);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_contained_by",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_contained_by(
        const Id &container)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_contained_by(container, token);
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_action_contained_by(
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = action_entity_contained_by;
        }
        else
        {
            LOG(error, "dbtype", "get_action_contained_by",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id ActionEntity::get_action_contained_by(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_contained_by(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_commands(
        const CommandList &commands,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            action_entity_commands = commands;
            action_entity_commands.shrink_to_fit();
            normalize_commands();
            notify_field_changed(ENTITYFIELD_action_commands);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_action_commands",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::set_action_commands(
        const CommandList &commands)
    {
        concurrency::WriterLockToken token(*this);

        return set_action_commands(commands, token);
    }

    // ----------------------------------------------------------------------
    ActionEntity::CommandList ActionEntity::get_action_commands(
        concurrency::ReaderLockToken &token)
    {
        CommandList result;

        if (token.has_lock(*this))
        {
            result = action_entity_commands;
        }
        else
        {
            LOG(error, "dbtype", "get_action_commands",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    ActionEntity::CommandList ActionEntity::get_action_commands(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_commands(token);
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_primary_action_command(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            if (not action_entity_commands.empty())
            {
                result = action_entity_commands[0];
            }
        }
        else
        {
            LOG(error, "dbtype", "get_primary_action_command",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string ActionEntity::get_primary_action_command(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_primary_action_command(token);
    }

    // ----------------------------------------------------------------------
    size_t ActionEntity::get_action_commands_size(
        concurrency::ReaderLockToken &token)
    {
        size_t result = 0;

        if (token.has_lock(*this))
        {
            result = action_entity_commands.size();
        }
        else
        {
            LOG(error, "dbtype", "get_action_commands_size",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    size_t ActionEntity::get_action_commands_size(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_action_commands_size(token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::has_action_command(
        const std::string &command,
        concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            const std::string command_normalized =
                text::to_lower_copy(command);

            result = has_action_command_internal(command_normalized);
        }
        else
        {
            LOG(error, "dbtype", "has_action_command",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::has_action_command(const std::string &command)
    {
        concurrency::ReaderLockToken token(*this);

        return has_action_command(command, token);
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::has_action_command_lower(
        const std::string &command_lower,
        concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            result = has_action_command_internal(command_lower);
        }
        else
        {
            LOG(error, "dbtype", "has_action_command_lower",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::has_action_command_lower(
        const std::string &command_lower)
    {
        concurrency::ReaderLockToken token(*this);

        return has_action_command_lower(command_lower, token);
    }

    // ----------------------------------------------------------------------
    ActionEntity::ActionEntity(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : PropertyEntity(id, type, version, instance, restoring),
        action_entity_lock_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    size_t ActionEntity::mem_used_fields(void)
    {
        size_t field_sizes = PropertyEntity::mem_used_fields()
          + sizeof(action_entity_lock_ptr)
          + (action_entity_lock_ptr ? action_entity_lock_ptr->mem_used() : 0)
          + sizeof(action_entity_succ_msg) + action_entity_succ_msg.size()
          + sizeof(action_entity_succ_room_msg) + action_entity_succ_room_msg.size()
          + sizeof(action_entity_fail_msg) + action_entity_fail_msg.size()
          + sizeof(action_entity_fail_room_msg) + action_entity_fail_room_msg.size()
          + action_entity_contained_by.mem_used()
          + sizeof(action_entity_targets);

        // Add up IDs in the targets
        for (Entity::IdVector::const_iterator iter =
                action_entity_targets.begin();
             iter != action_entity_targets.end();
             ++iter)
        {
            field_sizes += iter->mem_used();
        }

        // Add up commands
        for (CommandList::const_iterator command_iter =
            action_entity_commands.begin();
             command_iter != action_entity_commands.end();
             ++command_iter)
        {
            field_sizes += sizeof(*command_iter) + command_iter->size();
        }

        for (CommandList::const_iterator command_iter =
            action_entity_commands_normalized.begin();
             command_iter != action_entity_commands_normalized.end();
             ++command_iter)
        {
            field_sizes += sizeof(*command_iter) + command_iter->size();
        }

        return field_sizes;
    }

    // ----------------------------------------------------------------------
    void ActionEntity::copy_fields(Entity *entity_ptr)
    {
        PropertyEntity::copy_fields(entity_ptr);

        ActionEntity *cast_ptr = 0;

        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<ActionEntity *>(entity_ptr))) != 0))
        {
            cast_ptr->action_entity_targets = action_entity_targets;

            for (Entity::IdVector::const_iterator iter =
                    cast_ptr->action_entity_targets.begin();
                 iter != cast_ptr->action_entity_targets.end();
                 ++iter)
            {
                cast_ptr->added_id(ENTITYFIELD_action_targets, *iter);
            }

            cast_ptr->notify_field_changed(ENTITYFIELD_action_targets);

            if (action_entity_lock_ptr)
            {
                cast_ptr->action_entity_lock_ptr =
                    new Lock(*action_entity_lock_ptr);
                cast_ptr->notify_field_changed(ENTITYFIELD_action_lock);
            }

            cast_ptr->action_entity_commands = action_entity_commands;
            cast_ptr->action_entity_commands_normalized =
                action_entity_commands_normalized;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_commands);

            cast_ptr->action_entity_succ_msg = action_entity_succ_msg;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_succ_msg);

            cast_ptr->action_entity_succ_room_msg = action_entity_succ_room_msg;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_succ_room_msg);

            cast_ptr->action_entity_fail_msg = action_entity_fail_msg;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_fail_msg);

            cast_ptr->action_entity_fail_room_msg = action_entity_fail_room_msg;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_fail_room_msg);

            cast_ptr->set_single_id_field(
                ENTITYFIELD_action_contained_by,
                cast_ptr->action_entity_contained_by,
                action_entity_contained_by);
            cast_ptr->action_entity_contained_by = action_entity_contained_by;
            cast_ptr->notify_field_changed(ENTITYFIELD_action_contained_by);
        }
    }

    // ----------------------------------------------------------------------
    void ActionEntity::normalize_commands(void)
    {
        action_entity_commands_normalized.clear();
        action_entity_commands_normalized.shrink_to_fit();
        action_entity_commands_normalized.reserve(
            action_entity_commands.size());

        for (CommandList::const_iterator command_iter =
                action_entity_commands.begin();
            command_iter != action_entity_commands.end();
            ++command_iter)
        {
            action_entity_commands_normalized.push_back(
                text::to_lower_copy(*command_iter));
        }
    }

    // ----------------------------------------------------------------------
    bool ActionEntity::has_action_command_internal(
        const std::string &command_lower) const
    {
        bool result = false;

        for (CommandList::const_iterator iter =
            action_entity_commands_normalized.begin();
             iter != action_entity_commands_normalized.end();
             ++iter)
        {
            if (*iter == command_lower)
            {
                result = true;
                break;
            }
        }

        return result;
    }
} /* namespace dbtype */
} /* namespace mutgos */
