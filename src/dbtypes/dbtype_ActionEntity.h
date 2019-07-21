/*
 * dbtype_ActionEntity.h
 */

#ifndef MUTGOS_DBTYPE_ACTIONENTITY_H
#define MUTGOS_DBTYPE_ACTIONENTITY_H

#include <stddef.h>
#include <string>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "dbtype_Id.h"
#include "dbtype_Entity.h"
#include "dbtype_PropertyEntity.h"
#include "dbtype_Lock.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents an ActionEntity type.  This abstract type is used by
     * actions, exits, etc... anything that when 'used' by the user in-game
     * causes something to happen.
     *
     * The command aliases here have nothing to do with the Command Entity
     * type, although a Command Entity does inherit from thise.
     */
    class ActionEntity : public PropertyEntity
    {
    public:
        typedef std::vector<std::string> CommandList;

        /**
         *  Constructor used for deserialization of an ActionEntity.
         */
        ActionEntity();

        /**
         * Constructs an ActionEntity (final type).
         * @param id[in] The ID of the entity.
         */
        ActionEntity(const Id &id);

        /**
         * Destructor.
         */
        virtual ~ActionEntity();

        /**
         * Creates a copy of this ActionEntity.
         * @param id[in] The new ID of the cloned ActionEntity.
         * @param version[in] The new version # of this ActionEntity.
         * @param instance[in] The new instance # of this ActionEntity.
         * @param token[in] The lock token.
         * @return The clone as a pointer.  Caller must manage the pointer!
         * Null is returned if there is an error, such as an incorrect
         * lock token.
         */
        virtual Entity *clone(
            const Id &id,
            const VersionType version,
            const InstanceType instance,
            concurrency::ReaderLockToken &token);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);

        /**
         * Adds an ID to the action target.  If the ID already exists, nothing
         * will happen.
         * @param target[in] The ID of the target to add.
         * @param token[in] The lock token.
         * @return True if success or ID already a target, false if error.
         */
        bool add_action_target(
            const Id &target,
            concurrency::WriterLockToken &token);

        /**
         * Adds an ID to the action target.  If the ID already exists, nothing
         * will happen.
         * This method will automatically get a lock.
         * @param target[in] The ID of the target to add.
         * @return True if success or ID already a target, false if error.
         */
        bool add_action_target(
            const Id &target);

        /**
         * Removes an ID from the action target.  If the ID does not exist,
         * nothing will happen.
         * @param target[in] The ID of the target to remove.
         * @param token[in] The lock token.
         * @return True if success or nothing to remove, false if error.
         */
        bool remove_action_target(
            const Id &target,
            concurrency::WriterLockToken &token);

        /**
         * Removes an ID from the action target.  If the ID does not exist,
         * nothing will happen.
         * This method will automatically get a lock.
         * @param target[in] The ID of the target to remove.
         * @return True if success or nothing to remove, false if error.
         */
        bool remove_action_target(
            const Id &target);

        /**
         * Clears the action target.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool clear_action_target(concurrency::WriterLockToken &token);

        /**
         * Clears the action target.
         * This method will automatically get a lock.
         * @return True if success.
         */
        bool clear_action_target(void);

        /**
         * Replaces the current targets with a new set.
         * @param targets[in] The new targets.
         * @param token[in] The lock token.
         * @return True if success, false if error.
         */
        bool set_action_target(
            const Entity::IdVector &targets,
            concurrency::WriterLockToken &token);

        /**
         * Replaces the current targets with a new set.
         * This method will automatically get a lock.
         * @param targets[in] The new targets.
         * @return True if success, false if error.
         */
        bool set_action_target(
            const Entity::IdVector &targets);

        /**
         * @param token[in] The lock token.
         * @return The first action target, or default if none or error.
         */
        Id get_first_action_target(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The first action target, or default if none or error.
         */
        Id get_first_action_target(void);

        /**
         * Takes the current ID and returns the target ID after it.
         * @param id[in] The current ID within the list.
         * @param token[in] The lock token.
         * @return The target after 'id', or default if not found or no more
         * IDs.
         */
        Id get_next_action_target(
            const Id &id,
            concurrency::ReaderLockToken &token);

        /**
         * Takes the current ID and returns the target ID after it.
         * This method will automatically get a lock.
         * @param id[in] The current ID within the list.
         * @return The target after 'id', or default if not found or no more
         * IDs.
         */
        Id get_next_action_target(const Id &id);

        /**
         * @param token[in] The lock token.
         * @return The last action target, or default if none or error.
         */
        Id get_last_action_target(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The last action target, or default if none or error.
         */
        Id get_last_action_target(void);

        /**
         * @param token[in] The lock token.
         * @return A copy of all the action targets, or empty if none
         * or error.
         */
        Entity::IdVector get_action_targets(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return A copy of all the action targets, or empty if none or
         * error.
         */
        Entity::IdVector get_action_targets(void);

        /**
         * @param token[in] The lock token.
         * @return The number of action targets, or 0 if none or error.
         */
        size_t get_action_targets_size(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The number of action targets, or 0 if none or error.
         */
        size_t get_action_targets_size(void);

        /**
         * Set the lock associated with the action entity.
         * @param lock[in] The lock to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_lock(
            const Lock &lock,
            concurrency::WriterLockToken &token);

        /**
         * Set the lock associated with the action entity.
         * This method will automatically get a lock.
         * @param lock[in] The lock to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_lock(const Lock &lock);

        /**
         * @param token[in] The lock token.
         * @return A copy of the lock associated with the action entity, or
         * default/invalid if error.
         */
        Lock get_action_lock(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return A copy of the lock associated with the action entity, or
         * default/invalid if error.
         */
        Lock get_action_lock(void);

        /**
         * Sets the success message (to the user) associated with the action
         * entity.
         * @param message[in] The message to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_success_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the success message (to the user) associated with the action
         * entity.
         * This method will automatically get a lock.
         * @param message[in] The message to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_success_message(
            const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The success message (to the user) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_success_message(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The success message (to the user) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_success_message(void);

        /**
         * Sets the success message (to the room) associated with the action
         * entity.
         * @param message[in] The message to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_success_room_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the success message (to the room) associated with the action
         * entity.
         * This method will automatically get a lock.
         * @param message[in] The message to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_success_room_message(const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The success message (to the room) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_success_room_message(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The success message (to the room) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_success_room_message(void);

        /**
         * Sets the fail message (to the user) associated with the action
         * entity.
         * @param message[in] The message to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_fail_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the fail message (to the user) associated with the action
         * entity.
         * This method will automatically get a lock.
         * @param message[in] The message to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_fail_message(
            const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The fail message (to the user) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_fail_message(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The fail message (to the user) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_fail_message(void);

        /**
         * Sets the fail message (to the room) associated with the action
         * entity.
         * @param message[in] The message to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_fail_room_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the fail message (to the room) associated with the action
         * entity.
         * This method will automatically get a lock.
         * @param message[in] The message to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_fail_room_message(const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The fail message (to the room) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_fail_room_message(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The fail message (to the room) associated with the action
         * entity, or empty if none or error.
         */
        std::string get_action_fail_room_message(void);

        /**
         * Sets what contains (holds) this action.
         * @param container[in] The action container.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_contained_by(
            const Id &container,
            concurrency::WriterLockToken &token);

        /**
         * Sets what contains (holds) this action.
         * This method will automatically get a lock.
         * @param container[in] The action container.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_contained_by(const Id &container);

        /**
         * @param token[in] The lock token.
         * @return What contains this action, or default if none or error.
         */
        Id get_action_contained_by(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return What contains this action, or default if none or error.
         */
        Id get_action_contained_by(void);

        /**
         * Sets the list of commands that refer to activating this ActionEntity.
         * @param commands[in] The commands to set.
         * @param token[in] The lock token.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_commands(
            const CommandList &commands,
            concurrency::WriterLockToken &token);

        /**
         * Sets the list of commands that refer to activating this ActionEntity.
         * This method will automatically get a lock.
         * @param commands[in] The commands to set.
         * @return True if success, false if error such as bad token.
         */
        bool set_action_commands(const CommandList &commands);

        /**
         * @param token[in] The lock token.
         * @return The list of commands that refer to activating this
         * ActionEntity, or empty if none or error.
         */
        CommandList get_action_commands(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The list of commands that refer to activating this
         * ActionEntity, or empty if none or error.
         */
        CommandList get_action_commands(void);

        /**
         * @param token[in] The lock token.
         * @return The 'primary' (first in list) command for this action, or
         * empty string if no commands or error.
         */
        std::string get_primary_action_command(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The 'primary' (first in list) command for this action, or
         * empty string if no commands or error.
         */
        std::string get_primary_action_command(void);

        /**
         * @param token[in] The lock token.
         * @return The number of commands, or 0 if none or error.
         */
        size_t get_action_commands_size(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The number of commands, or 0 if none or error.
         */
        size_t get_action_commands_size(void);

        /**
         * @param command[in] The command to find (not case sensitive).
         * @param token[in] The lock token.
         * @return True if this action has the command alias, or false if not
         * or error.
         */
        bool has_action_command(
            const std::string &command,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param command[in] The command to find (not case sensitive).
         * @return True if this action has the command alias, or false if not
         * or error.
         */
        bool has_action_command(const std::string &command);

        /**
         * Like has_action_command, but requires/assumes command to find
         * is already lowercase.
         * @param command_lower[in] The command to find (MUST be lowercase).
         * @param token[in] The lock token.
         * @return True if this action has the command alias, or false if not
         * or error.
         */
        bool has_action_command_lower(
            const std::string &command_lower,
            concurrency::ReaderLockToken &token);

        /**
         * Like has_action_command, but requires/assumes command to find
         * is already lowercase.
         * This method will automatically get a lock.
         * @param command_lower[in] The command to find (MUST be lowercase).
         * @return True if this action has the command alias, or false if not
         * or error.
         */
        bool has_action_command_lower(const std::string &command_lower);

    protected:
        /**
         * Constructs an Entity with a provided type.  Used by subclasses.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        ActionEntity(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error.
         */
        virtual size_t mem_used_fields(void);

        /**
         * Copies fields from this ActionEntity to the provided Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:

        /**
         * Normalizes action_entity_commands and places the results in
         * action_entity_commands_normalized.  Currently this just makes
         * everything lowercase.
         */
        void normalize_commands(void);

        /**
         * Assumes read locking has occurred.  This will check the
         * normalized (lowercase) commands for an exact match of the
         * provided lowercase command.
         * @param command_lower[in] The lowercase command alias to check for.
         * @return True if this action has the command alias, or false if not
         * or error.
         */
        bool has_action_command_internal(
            const std::string &command_lower) const;

        Entity::IdVector action_entity_targets; ///< What this targets
        Lock *action_entity_lock_ptr; ///< Lock users must pass in order to activate
        std::string action_entity_succ_msg; ///< Message shown to user on succ
        std::string action_entity_succ_room_msg; ///< Shown to room on success
        std::string action_entity_fail_msg; ///< Message shown to user on failure
        std::string action_entity_fail_room_msg; ///< Shown to room on failure
        Id action_entity_contained_by; ///< Who has this action
        CommandList action_entity_commands; ///< Commands (alias) to use action
        CommandList action_entity_commands_normalized; ///< Lowercase (normalized) commands to use action

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & action_entity_targets;

            const bool has_lock = (action_entity_lock_ptr != 0);
            ar & has_lock;

            if (has_lock)
            {
                ar & *action_entity_lock_ptr;
            }

            ar & action_entity_succ_msg;
            ar & action_entity_succ_room_msg;
            ar & action_entity_fail_msg;
            ar & action_entity_fail_room_msg;
            ar & action_entity_contained_by;
            ar & action_entity_commands;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & action_entity_targets;

            bool has_lock = false;
            ar & has_lock;

            if (has_lock)
            {
                delete action_entity_lock_ptr;
                action_entity_lock_ptr = new Lock();
                ar & *action_entity_lock_ptr;
            }

            ar & action_entity_succ_msg;
            ar & action_entity_succ_room_msg;
            ar & action_entity_fail_msg;
            ar & action_entity_fail_room_msg;
            ar & action_entity_contained_by;
            ar & action_entity_commands;

            action_entity_commands.shrink_to_fit();
            normalize_commands();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */


#endif //MUTGOS_DBTYPE_ACTIONENTITY_H
