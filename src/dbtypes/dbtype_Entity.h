/*
 * dbtype_Entity.h
 */

#ifndef MUTGOS_DBTYPE_ENTITY_H_
#define MUTGOS_DBTYPE_ENTITY_H_

#include <string>
#include <set>
#include <map>
#include <vector>
#include <stddef.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "osinterface/osinterface_ThreadUtils.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_TimeStamp.h"

// TODO: Copy_fields()  should only copy anything if it's the right type??
// TODO: Post-demo: Have hard string cutoff for ALL set strings, common cutoff method, UTF8 aware
// TODO: Post-demo: Convert all sets to vectors where practical.
// TODO: 'references' might need to be handled in the database, and not in here

// TODO: Reminder: May have deadlock in DB backend due to switch to shared locks

// TODO: Removed flags, need to turn into booleans post demo.  Flags becomes a freeform text field.

// TODO: Add 'symbolic link' entity type that could point to other entities and have a reg entry.

namespace mutgos
{
namespace dbtype
{
    class DatabaseEntityChangeListener;

    /**
     * Represents an Entity database type, the root object type of everything
     * in the database that is present in-world.
     *
     * This class, and all subclasses, are entirely thread safe except where
     * noted.  Also note that in order to support having multiple write and
     * read locks on the same thread for the same Entity, the first lock
     * MUST be a write lock, to establish exclusivity.  If this is not done,
     * a deadlock will result.
     *
     * Note that this class and all its subclasses are tightly coupled, and
     * should be treated as a unit.  This was to simplify the design at the
     * expense of flexibility.  Considering that the database subsystem
     * needs to make schemas based off this, it's a safe assumption that
     * the basic hierarchy and attributes will not change frequently.  If
     * they do, it will generally be additions.
     */
    class Entity : public concurrency::LockableObject
    {
    public:
        /** Container for changed fields */
        typedef std::set<EntityField> EntityFieldSet;
        /** Typedef for flag */
        typedef std::string FlagType;
        /** Container for flags */
        typedef std::set<FlagType> FlagSet;

        /** Container for sets of IDs */
        typedef std::set<Id> IdSet;
        /** Container for a vector of IDs */
        typedef std::vector<Id> IdVector;

        /** Data type for instance numbers. */
        typedef MG_UnsignedInt InstanceType;
        /** Data type for version numbers. */
        typedef MG_UnsignedInt VersionType;
        /** Data type for the access counter */
        typedef MG_VeryLongUnsignedInt AccessCountType;

        /** First is flags removed, second is flags added.  Process removals
         *  first, then adds. */
        typedef std::pair<Entity::FlagSet, Entity::FlagSet> FlagsRemovedAdded;

        /** First is IDs removed, second is IDs added.  Process removals
         * first, then adds. */
        typedef std::pair<Entity::IdSet, Entity::IdSet> IdsRemovedAdded;
        /** Maps field of type ID in Entity to the changes made to the field */
        typedef std::map<EntityField, IdsRemovedAdded> ChangedIdFieldsMap;

        /** Maps field to a set of Entity IDs whose corresponding field
         *  references this Entity */
        typedef std::map<Id, EntityFieldSet> IdFieldsMap;
        /** Maps a field to a set of IDs that use the field to reference
         * this Entity.  Each vector entry is mapped to a field enum by
         * number */
        typedef std::vector<IdSet *> FieldIdsArray;

        /** Type for the delete batch ID. */
        typedef osinterface::OsTypes::VeryLongUnsignedInt DeleteBatchId;

        /** Represents return codes for flag operations */
        enum EntityFlagReturnCode
        {
            FLAGRC_success,  ///< Successful set operation
            FLAGRC_invalid,  ///< Flag is not valid or known
            FLAGRC_set,      ///< Flag is set on Entity
            FLAGRC_not_set,  ///< Flag is not set on Entity
            FLAGRC_lock_error ///< Wrong lock token used
        };

        /**
         * Constructs an Entity (final type).
         * @param id[in] The ID of the entity.
         */
        Entity(const Id &id);

        /**
         * Constructor used for deserialization of an Entity.
         */
        Entity();

        /**
         * Destructor.
         */
        virtual ~Entity();

        //////////  Not for direct use by users, used only by Locks
        /**
         * Locks this entity for exclusive (read/write) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock(void);

        /**
         * Attempts to lock this entity for exclusive (read/write) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock(void);

        /**
         * Attempts to lock this entity for shared (read only) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock_shared(void);

        /**
         * Locks this entity for shared (read only) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock_shared(void);

        /**
         * Unlocks this entity from an exclusive lock.  Only call if lock()
         * was used!
         * @return True if success.
         */
        virtual bool unlock(void);

        /**
         * Unlocks this entity from a shared lock.  Only call if lock_shared()
         * was used!
         * @return True if success.
         */
        virtual bool unlock_shared(void);
        ///////////////////////

        /**
         * Creates a copy of this Entity.
         * @param id[in] The new ID of the cloned Entity.
         * @param version[in] The new version # of this Entity.
         * @param instance[in] The new instance # of this Entity.
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
         * Creates a copy of this Entity.  Locking is automatically performed.
         * @param id[in] The new ID of the cloned Entity.
         * @param version[in] The new version # of this Entity.
         * @param instance[in] The new instance # of this Entity.
         * @return The clone as a pointer.  Caller must manage the pointer!
         * Null is returned if there is an error.
         */
        virtual Entity *clone(
            const Id &id,
            const VersionType version,
            const InstanceType instance);

        /**
         *
         * @param token[in] The lock token.
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error (wrong token).
         */
        virtual size_t mem_used(concurrency::ReaderLockToken &token);

        /**
         * This will automatically get a lock.
         * @return Approximate memory used by this class instance, in bytes.
         */
        size_t mem_used(void);

        /**
         * < comparison.
         * @param rhs[in] The Entity to compare against.
         * @return True if this Entity is strictly less than rhs.
         */
        inline bool operator<(const Entity &rhs) const
        {
            if (entity_id < rhs.entity_id)
            {
                return true;
            }
            else if (entity_id == rhs.entity_id)
            {
                if (entity_version < rhs.entity_version)
                {
                    return true;
                }
                else if (entity_version == rhs.entity_version)
                {
                    return (entity_instance < rhs.entity_instance);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        /**
         * == comparison.
         * @param rhs[in] The Entity to compare against.
         * @return True if this Entity is equal to rhs.
         */
        inline bool operator==(const Entity &rhs) const
        {
            // No need to check entity_type as the database will
            // ensure only a single type is associated with an ID.
            return ((entity_id == rhs.entity_id) and
                    (entity_instance == rhs.entity_instance) and
                    (entity_version == rhs.entity_version));
        }

        /**
         * Adds a change listener, which is used to let a
         * subsystem know when an Entity or subclass has changed.
         * @param listener_ptr[in] The listener to add.
         */
        static void register_change_listener(
            DatabaseEntityChangeListener *listener_ptr);

        /**
         * Removes a change listener.
         * @param listener_ptr[in] The listener to remove.
         */
        static void unregister_change_listener(
            DatabaseEntityChangeListener *listener_ptr);

        /**
         * Used by the database subsystem when restoring an Entity from
         * storage, calling this allows all future changes to mark the Entity
         * as dirty.
         */
        void restore_complete(void);

        /**
         * Clears the dirty flag and any information regarding what was
         * dirty.
         * @param token[in] The lock token.
         * @return True if success (valid lock).
         */
        bool clear_dirty(concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return True if the Entity is 'dirty' (has changes not yet
         * committed to the database), false if not or error.
         */
        bool is_dirty(void);

        /**
         * @param token[in] The lock token.
         * @return True if the Entity is 'dirty' (has changes not yet
         * committed to the database), false if not or error.
         */
        bool is_dirty(concurrency::ReaderLockToken &token);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);

        ///////////////////////////////////////////////
        // Special methods used during manual deserialization

        /**
         * Sets the creation timestamp.  This is NOT thread safe and is for
         * deserialization only.
         * @param timestamp[in] The timestamp to set.
         */
        void set_entity_created_timestamp(const TimeStamp &timestamp);

        /**
         * Sets the update timestamp.  This is NOT thread safe and is for
         * deserialization only.
         * @param timestamp[in] The timestamp to set.
         */
        void set_entity_updated_timestamp(const TimeStamp &timestamp);

        /**
         * Sets the last accessed timestamp.  This is NOT thread safe and is for
         * deserialization only.
         * @param timestamp[in] The timestamp to set.
         */
        void set_entity_accessed_timestamp(const TimeStamp &timestamp);

        /**
         * Sets the access count.  This is NOT thread safe and is for
         * deserialization only.  Normally the count is automatically
         * managed.
         * @param count[in] The access count to set.
         */
        void set_entity_access_count(const AccessCountType count);

        /**
         * Sets the flags on this Entity.  This is NOT thread safe and is for
         * deserialization only.
         * @param flags
         */
        void set_entity_flags(const FlagSet &flags);

        ///////////////////////////////////////////////

        /**
         * @return The type of this Entity.
         */
        inline const EntityType get_entity_type(void) const
        {
            return entity_type;
        }

        /**
         * @return A reference to the Entity's ID.
         */
        inline const Id &get_entity_id(void) const
        {
            return entity_id;
        }

        /**
         * @return The Entity version number.
         */
        inline VersionType get_entity_version(void) const
        {
            return entity_version;
        }

        /**
         * @return The Entity instance number.
         */
        inline InstanceType get_entity_instance(void) const
        {
            return entity_instance;
        }

        /**
         * @return When the Entity was created.
         */
        inline const TimeStamp &get_entity_created_timestamp(void) const
        {
            return entity_created_timestamp;
        }

        ///////////////////////////////

        /**
         * Gets the Entity's name.
         * @param token[in] The lock token.
         * @return A copy of the Entity's name or empty if error.
         */
        std::string get_entity_name(concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's name (locking).
         * This method will automatically get a lock.
         * @return A copy of the Entity's name or empty if error.
         */
        std::string get_entity_name(void);

        /**
         * Sets the Entity's name.
         * @param name[in] The name of the Entity.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        virtual bool set_entity_name(
            const std::string &name,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Entity's name (locking).
         * This method will automatically get a lock.
         * @param name[in] The name of the Entity.
         * @return True if successfully set.
         */
        bool set_entity_name(const std::string &name);

        ///////////////////////////////

        /**
         * Gets the Entity's security flags.
         * @param token[in] The lock token.
         * @return A copy of the Entity's security flags or a default if error.
         */
        Security get_entity_security(concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's security flags.
         * This method will automatically get a lock.
         * @return A copy of the Entity's security flags or a default if error.
         */
        Security get_entity_security(void);

        /**
         * Sets the Entity's security flags.
         * @param security[in] The security flags to set.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_entity_security(
            const Security &security,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Entity's security flags.
         * This method will automatically get a lock.
         * @param security[in] The security flags to set.
         * @return True if successfully set.
         */
        bool set_entity_security(const Security &security);

        ///////////////////////////////

        /**
         * Gets the Entity's note, which is a freeform string used purely for
         * documentation purposes.
         * @param token[in] The lock token.
         * @return A copy of the Entity's note or empty if error.
         */
        std::string get_entity_note(concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's note, which is a freeform string used purely for
         * documentation purposes (locking).
         * This method will automatically get a lock.
         * @return A copy of the Entity's note or empty if error.
         */
        std::string get_entity_note(void);

        /**
         * Sets the Entity's note, which is a freeform string used purely for
         * documentation purposes.
         * @param note[in] The note to set.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_entity_note(
            const std::string &note,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Entity's note, which is a freeform string used purely for
         * documentation purposes (locking).
         * This method will automatically get a lock.
         * @param note[in] The note to set.
         * @return True if successfully set.
         */
        bool set_entity_note(const std::string &note);

        ///////////////////////////////

        /**
         * Gets the Entity's 'last updated' field.
         * @param token[in] The lock token.
         * @return A copy of the Entity's last updated timestamp or a default
         * if error.
         */
        TimeStamp get_entity_updated_timestamp(
            concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's 'last updated' field (locking).
         * This method will automatically get a lock.
         * @return A copy of the Entity's last updated timestamp or a default
         * if error.
         */
        TimeStamp get_entity_updated_timestamp(void);

        ///////////////////////////////

        /**
         * Gets the Entity's 'last accessed' field.
         * @param token[in] The lock token.
         * @return A copy of the Entity's 'last accessed' field
         * or a default if error.
         */
        TimeStamp get_entity_accessed_timestamp(
            concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's 'last accessed' field (locking).
         * This method will automatically get a lock.
         * @return A copy of the Entity's 'last accessed' field
         * or a default if error.
         */
        TimeStamp get_entity_accessed_timestamp(void);

        /**
         * Sets the Entity's 'last accessed' field to 'now'.  It will also
         * increment the access count.
         * This field is automatically updated if something is set on the
         * Entity.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_entity_accessed_timestamp(
            concurrency::WriterLockToken &token);

        /**
         * Sets the Entity's 'last accessed' field to 'now' (locking).
         * It will also increment the access count.
         * This field is automatically updated if something is set on the
         * Entity.
         * This method will automatically get a lock.
         * @return True if successfully set.
         */
        bool set_entity_accessed_timestamp(void);


        ///////////////////////////////

        /**
         * Gets the Entity's access count field.
         * @param token[in] The lock token.
         * @return A copy of the Entity's access count or 0 if error.
         */
        AccessCountType get_entity_access_count(
            concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's access count field (locking).
         * This method will automatically get a lock.
         * @param token[in] The lock token.
         * @return A copy of the Entity's access count or 0 if error.
         */
        AccessCountType get_entity_access_count(void);

        ///////////////////////////////

        /**
         * Gets the Entity's owner.
         * @param token[in] The lock token.
         * @return A copy of the Entity's owner ID,
         * or a default if error.
         */
        Id get_entity_owner(concurrency::ReaderLockToken &token);

        /**
         * Gets the Entity's owner (locking).
         * This method will automatically get a lock.
         * @return A copy of the Entity's owner ID,
         * or a default if error.
         */
        Id get_entity_owner(void);

        /**
         * Sets the Entity's owner.
         * @param owner[in] The Entity owner.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_entity_owner(
            const Id &owner,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Entity's owner (locking).
         * This method will automatically get a lock.
         * @param owner[in] The Entity owner.
         * @return True if successfully set.
         */
        bool set_entity_owner(const Id &owner);

        ///////////////////////////////

        /**
         * Sets the given flag on the entity.  If the flag is already set,
         * nothing will happen.
         * @param flag[in] The flag to set.
         * @param token[in] The lock token.
         * @return flag_invalid if the flag is not valid for this Entity,
         * flag_success if the flag has been set or is already set, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode add_entity_flag(
            const FlagType &flag,
            concurrency::WriterLockToken &token);

        /**
         * Sets the given flag on the entity (locking).  If the flag is already
         * set, nothing will happen.
         * This method will automatically get a lock.
         * @param flag[in] The flag to set.
         * @return flag_invalid if the flag is not valid for this Entity,
         * flag_success if the flag has been set or is already set, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode add_entity_flag(
            const FlagType &flag);

        /**
         * Removes the given flag on the entity.  If the flag is
         * not present, nothing will happen.
         * @param flag[in] The flag to remove.
         * @param token[in] The lock token.
         * @return flag_invalid if the flag is not valid for this Entity,
         * flag_success if the flag has been or is removed, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode remove_entity_flag(
            const FlagType &flag,
            concurrency::WriterLockToken &token);

        /**
         * Removes the given flag on the entity (locking).  If the flag is
         * not present, nothing will happen.
         * This method will automatically get a lock.
         * @param flag[in] The flag to remove.
         * @return flag_invalid if the flag is not valid for this Entity,
         * flag_success if the flag has been or is removed, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode remove_entity_flag(
            const FlagType &flag);

        /**
         * Checks to see if the given flag is set on this Entity.
         * @param flag[in] The flag to check.
         * @param token[in] The lock token.
         * @return flag_invalid if the flag is not valid for this entity,
         * flag_set if the flag is set on this Entity, flag_not_set if the
         * flag is not set on this Entity, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode check_entity_flag(
            const FlagType &flag,
            concurrency::ReaderLockToken &token);

        /**
         * Checks to see if the given flag is set on this Entity (locking).
         * This method will automatically get a lock.
         * @param flag[in] The flag to check.
         * @return flag_invalid if the flag is not valid for this entity,
         * flag_set if the flag is set on this Entity, flag_not_set if the
         * flag is not set on this Entity, and
         * flag_lock_error if the token is not for this Entity.
         */
        EntityFlagReturnCode check_entity_flag(
            const FlagType &flag);

        /**
         * Gets a copy of all flags set on this Entity (locking).  Normally
         * used for printing or serializing.
         * This method will automatically get a lock.
         * @return A copy of all flags set on this Entity or an empty set
         * if error.
         */
        FlagSet get_entity_flags(void);

        /**
         * Gets a copy of all flags set on this Entity.  Normally
         * used for printing or serializing.
         * @param token[in] The lock token.
         * @return A copy of all flags set on this Entity or an empty
         * set if error.
         */
        FlagSet get_entity_flags(
            concurrency::ReaderLockToken &token);

        ///////////////////////////////

        /**
         * Indicates another Entity is referencing this one.  Duplicates per
         * field are allowed, but will not be counted.
         * @param id[in] The ID of Entity referencing this one.
         * @param field[in] The field on the other Entity which is referencing
         * this Entity.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool add_entity_reference(
            const Id &id,
            const EntityField field,
            concurrency::WriterLockToken &token);

        /**
         * Indicates another Entity is referencing this one (locking).
         * Duplicates per field are allowed, but will not be counted.
         * This method will automatically get a lock.
         * @param id[in] The ID of Entity referencing this one.
         * @param field[in] The field on the other Entity which is referencing
         * this Entity.
         * @return True if successfully set.
         */
        bool add_entity_reference(const Id &id, const EntityField field);

        /**
         * Indicates another Entity is no longer referencing this one on a
         * particular field.
         * Duplicates per field are allowed, but will not be counted.
         * @param id[in] The ID of Entity no longer referencing this one.
         * @param field[in] The field on the other Entity which is no longer
         * referencing this Entity.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool remove_entity_reference(
            const Id &id,
            const EntityField field,
            concurrency::WriterLockToken &token);

        /**
         * Indicates another Entity is no longer referencing this one on a
         * particular field (locking).
         * Duplicates per field are allowed, but will not be counted.
         * This method will automatically get a lock.
         * @param id[in] The ID of Entity no longer referencing this one.
         * @param field[in] The field on the other Entity which is no longer
         * referencing this Entity.
         * @return True if successfully set.
         */
        bool remove_entity_reference(const Id &id, const EntityField field);

        /**
         * Indicates another Entity is no longer referencing this one at all.
         * @param id[in] The ID of Entity no longer referencing this one.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool remove_entity_reference(
            const Id &id,
            concurrency::WriterLockToken &token);

        /**
         * Indicates another Entity is no longer referencing this one at all
         * (locking).
         * This method will automatically get a lock.
         * @param id[in] The ID of Entity no longer referencing this one.
         * @return True if successfully set.
         */
        bool remove_entity_reference(const Id &id);

        /**
         * Returns a copy of all the fields the given ID is referencing
         * on this Entity.
         * @param id[in] The ID to check references.
         * @param token[in] The lock token.
         * @return A copy of the set of EntityFields the given ID is
         * referencing on this Entity, or empty if error or no references
         * found.
         */
        EntityFieldSet get_field_references(
            const Id &id,
            concurrency::ReaderLockToken &token);

        /**
         * Returns a copy of all the fields the given ID is referencing
         * on this Entity (locking).
         * This method will automatically get a lock.
         * @param id[in] The ID to check references.
         * @return A copy of the set of EntityFields the given ID is
         * referencing on this Entity, or empty if error or no references
         * found.
         */
        EntityFieldSet get_field_references(const Id &id);

        /**
         * @param token[in] The lock token.
         * @return A copy of the IDs of Entities referencing this one.
         */
        IdSet get_reference_ids(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return A copy of the IDs of Entities referencing this one.
         */
        IdSet get_reference_ids(void);

        /**
         * @param token[in] The lock token.
         * @return A copy of a map of referencing ID to referencing fields,
         * or nothing if error or nothing is referencing this Entity.
         */
        IdFieldsMap get_all_references(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return A copy of a map of referencing ID to referencing fields,
         * or nothing if error or nothing is referencing this Entity.
         */
        IdFieldsMap get_all_references(void);

        /**
         * @param token[in] The lock token.
         * @return The first ID referencing this Entity, or a default ID
         * if none.
         */
        Id get_first_reference(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The first ID referencing this Entity, or a default ID
         * if none.
         */
        Id get_first_reference(void);

        /**
         * @param id[in] The current position within the reference list.
         * @param token[in] The lock token.
         * @return the next ID referencing this Entity after the given ID, or
         * a default ID if none.
         */
        Id get_next_reference(const Id &id, concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param id[in] The current position within the reference list.
         * @return the next ID referencing this Entity after the given ID, or
         * a default ID if none.
         */
        Id get_next_reference(const Id &id);

        /**
         * @param token[in] The lock token.
         * @return The last ID referencing this Entity, or a default ID
         * if none.
         */
        Id get_last_reference(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The last ID referencing this Entity, or a default ID
         * if none.
         */
        Id get_last_reference(void);

        /**
         * @param field[in] The field to get references for.
         * @param token[in] The lock token.
         * @return The IDs of Entities whose given field references this
         * Entity, or empty if none or error.
         */
        IdVector get_reference_ids(
            const EntityField field,
            concurrency::ReaderLockToken &token);
        /**
         * This method will automatically get a lock.
         * @param field[in] The field to get references for.
         * @return The IDs of Entities whose given field references this
         * Entity, or empty if none or error.
         */
        IdVector get_reference_ids(const EntityField field);

        /**
         * @param field[in] The field to get references for.
         * @param ids[out] The IDs of Entities whose given field references
         * this Entity will be appended to this.  No change if none found or
         * error.
         * @param token[in] The lock token.
         * @return True if field found and it has Entities referencing this
         * Entity, false if not or error.
         */
        bool get_reference_ids_append(
            const EntityField field,
            IdVector &ids,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param field[in] The field to get references for.
         * @param ids[out] The IDs of Entities whose given field references
         * this Entity will be appended to this.  No change if none found or
         * error.
         * @return True if field found and it has Entities referencing this
         * Entity, false if not or error.
         */
        bool get_reference_ids_append(const EntityField field, IdVector &ids);

        /**
         * @param field[in] The field to check.
         * @param token[in] The lock token.
         * @return The first ID referencing this Entity for the given field on
         * the referencing Entity, or a default ID if none.
         */
        Id get_first_reference(
            const EntityField field,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param field[in] The field to check.
         * @return The first ID referencing this Entity for the given field on
         * the referencing Entity, or a default ID if none.
         */
        Id get_first_reference(const EntityField field);

        /**
         * @param field[in] The field to check.
         * @param id[in] The current position within the reference list.
         * @param token[in] The lock token.
         * @return the next ID referencing this Entity after the given ID,
         * for the given field on the referencing Entity,
         * or a default ID if none.
         */
        Id get_next_reference(
            const EntityField field,
            const Id &id,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param field[in] The field to check.
         * @param id[in] The current position within the reference list.
         * @return the next ID referencing this Entity after the given ID,
         * for the given field on the referencing Entity,
         * or a default ID if none.
         */
        Id get_next_reference(const EntityField field, const Id &id);

        /**
         * @param field[in] The field to check.
         * @param token[in] The lock token.
         * @return The last ID referencing this Entity for the given field on
         * the referencing Entity, or a default ID if none.
         */
        Id get_last_reference(
            const EntityField field,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param field[in] The field to check.
         * @return The last ID referencing this Entity for the given field on
         * the referencing Entity, or a default ID if none.
         */
        Id get_last_reference(EntityField field);

        /**
         * Clears all references to this Entity.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool clear_all_references(concurrency::WriterLockToken &token);

        /**
         * Clears all references to this Entity (locking).
         * This method will automatically get a lock.
         * @return True if success.
         */
        bool clear_all_references(void);

        ///////////////////////////////

        /**
         * If this returns a non-zero number, then the Entity should be
         * considered as deleted.
         * @param token[in] The lock token.
         * @return The delete batch ID.  A non-zero value indicates the
         * Entity may be deleted soon.  The ID is not unique.
         */
        DeleteBatchId get_delete_batch_id(concurrency::ReaderLockToken &token);

        /**
         * If this returns a non-zero number, then the Entity should be
         * considered as deleted (locking).
         * This method will automatically get a lock.
         * @return The delete batch ID.  A non-zero value indicates the
         * Entity may be deleted soon.  The ID is not unique.
         */
        DeleteBatchId get_delete_batch_id(void);

        /**
         * Sets the delete batch ID.  Typically only done by the database
         * infrastructure.  When this is set to something non-zero, the
         * Entity is considered as being queued for possible deletion.  The
         * ID is not unique.
         * @param delete_id[in] The delete batch ID.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_delete_batch_id(
            const DeleteBatchId delete_id,
            concurrency::WriterLockToken &token);

        /**
         * Sets the delete batch ID (locking).  Typically only done by the
         * database infrastructure.  When this is set to something non-zero, the
         * Entity is considered as being queued for possible deletion.  The
         * ID is not unique.
         * This method will automatically get a lock.
         * @param delete_id[in] The delete batch ID.
         * @return True if success.
         */
        bool set_delete_batch_id(
            const DeleteBatchId delete_id);

        ///////////////////////////////

        /**
         * The deleted flag indicates this Entity is deleted and should
         * not be linked to or referred to in any way except when being
         * purged or undeleted.
         * @param token[in] The lock token.
         * @return The status of the deleted flag, or false if error.
         */
        bool get_deleted_flag(concurrency::ReaderLockToken &token);

        /**
         * The deleted flag indicates this Entity is deleted and should
         * not be linked to or referred to in any way except when being
         * purged or undeleted (locking).
         * This method will automatically get a lock.
         * @return The status of the deleted flag, or false if error.
         */
        bool get_deleted_flag(void);

        /**
         * Sets the delete flag.  The deleted flag indicates this Entity is
         * deleted and should not be linked to or referred to in any way except
         * when being purged or undeleted.
         * @param deleted[in] The new delete flag status.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_deleted_flag(
            const bool deleted,
            concurrency::WriterLockToken &token);

        /**
         * Sets the delete flag (locking).  The deleted flag indicates this
         * Entity is deleted and should not be linked to or referred to in any
         * way except when being purged or undeleted.
         * This method will automatically get a lock.
         * @param deleted[in] The new delete flag status.
         * @return True if success.
         */
        bool set_deleted_flag(
            const bool deleted);


        /**
         * Called by anyone on this or any subclass whenever the contents
         * of a field changes.
         * @param field[in] The field that changed.
         */
        void notify_field_changed(const EntityField field);

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
        Entity(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * Copies fields from this Entity to the provided Entity.  Subclasses
         * will override this and call their parent, the chain as a whole
         * allowing for an Entity of any type to be copied.  This is a helper
         * method used with clone().
         * It can be assumed the Entity being copied to is 'fresh', without
         * any old data on it.
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

        /**
         * @return The size, in bytes, of class-based fields on this Entity.
         * Simple types (int, bool, etc) are not included.
         */
        virtual size_t mem_used_fields(void);

        /**
         * @return True if Entity is deleted.
         */
        inline bool is_deleted(void) const
        {
            return entity_deleted_flag;
        }

        /**
         * Adds an entry in the journal for an ID being added to a field.
         * @param field[in] The field who has an ID being added.
         * @param id_added[in] The ID that has been added.
         */
        void added_id(const EntityField field, const Id &id_added);

        /**
         * Adds an entry in the journal for an ID being removed from a field.
         * @param field[in] The field who has an ID being removed.
         * @param id_added[in] The ID that has been removed.
         */
        void removed_id(const EntityField field, const Id &id_removed);

        /**
         * Used when setting journaled IDs on a single element field (IE, not
         * an array), this simplifies the process.
         * @param field[in] The field who has an ID being set.
         * @param old_id[in] The previous ID of the field.
         * @param new_id[in] The new ID of the field.
         */
        void set_single_id_field(
            const EntityField field,
            const Id &old_id,
            const Id &new_id);

        /**
         * Removes all journaled IDs for the given field.  Generally used
         * for special purposes.
         * @param field[in] THe field whose journal is to be removed.
         */
        void clear_id_journal(const EntityField field);

        /**
         * Adds an entry in the journal for a flag being added to the Entity.
         * @param flag_added[in] The flag which is being added for the Entity.
         */
        void added_flag(const FlagType &flag_added);

        /**
         * Adds an entry in the journal for a flag being removed from the
         * Entity.
         * @param flag_removed[in] The flag which is being removed for the
         * Entity.
         */
        void removed_flag(const FlagType &flag_removed);

        /**
         * If any fields have changed, then notify the database listener
         * that changes have occurred.
         */
        void notify_db_listener(void);

        // Fields of Entity
        //
        EntityType entity_type; ///< The (sub)type of this Entity.
        Id entity_id; ///< The ID of this Entity.
        Security entity_security; ///< Security fields for Entity

        InstanceType entity_instance; ///< Instance # of Entity
        VersionType entity_version; ///< Version # of Entity.

        std::string entity_name; ///< The name of this Entity.
        std::string entity_note; ///< Note concerning the Entity

        TimeStamp entity_created_timestamp;  ///< When created.
        TimeStamp entity_updated_timestamp;  ///< When updated
        TimeStamp entity_accessed_timestamp;   ///< When last accessed
        AccessCountType entity_access_count; ///< How many times accessed

        Id entity_owner; ///< The owner of this Entity.

        FlagSet entity_flags; ///< Flags for this Entity.

        IdFieldsMap entity_references; ///< Who references this Entity
        FieldIdsArray entity_references_field; ///< Reverse lookup reference

        DeleteBatchId entity_delete_batch_id; ///< When > 0, Entity is deleted
        bool entity_deleted_flag;  ///< True if deleted.

    private:

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & entity_type;
            ar & entity_id;
            ar & entity_instance;
            ar & entity_version;
            ar & entity_name;
            ar & entity_note;
            ar & entity_security;
            ar & entity_created_timestamp;
            ar & entity_updated_timestamp;
            ar & entity_accessed_timestamp;
            ar & entity_access_count;
            ar & entity_owner;
            ar & entity_flags;
            ar & entity_references;
            ar & entity_delete_batch_id;
            ar & entity_deleted_flag;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & entity_type;
            ar & entity_id;
            ar & entity_instance;
            ar & entity_version;
            ar & entity_name;
            ar & entity_note;
            ar & entity_security;
            ar & entity_created_timestamp;
            ar & entity_updated_timestamp;
            ar & entity_accessed_timestamp;
            ar & entity_access_count;
            ar & entity_owner;
            ar & entity_flags;
            ar & entity_references;
            ar & entity_delete_batch_id;
            ar & entity_deleted_flag;

            populate_entity_references_field();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

        typedef std::vector<DatabaseEntityChangeListener *> DbListeners;

        /**
         * Clears entity_references_field.
         */
        void clear_entity_references_field(void);

        /**
         * Populates entity_references_field with initial data from
         * entity_references.
         */
        void populate_entity_references_field(void);

        // No copying.
        // These are disallowed due to performance and data divergence reasons.
        //
        Entity(const Entity &rhs);
        Entity &operator=(const Entity &rhs);


        static DbListeners db_listeners; ///< DB listeners
        bool need_call_listener;  ///< When true, call listener when unlocked
        bool dirty_flag; ///< When true, changes need to be saved
        bool ignore_changes; ///< Used when deserializing

        EntityFieldSet dirty_fields; ///< Set of dirty (changed) fields.
        EntityFieldSet
            diff_callback_fields; ///< Fields that changed between callbacks.
        FlagsRemovedAdded
            diff_flags_changed;  ///< Lists flags that have changed
        ChangedIdFieldsMap
            diff_ids_changed; ///< Fields with IDs that have changed

        boost::shared_mutex entity_lock; ///< The lock for the Entity.

        // Everything below is a bad workaround for limitations in boost mutexes
        // Since it cannot detect a recursive shared_mutex inside a thread,
        // we have to add additional functionality to simulate it.  Hopefully
        // someone else knows a better way to do this.
        //
        boost::mutex exclusive_thread_lock; ///< Lock for checking if thread exclusively has Entity locked
        bool locked_thread_id_valid; ///< True if locked_thread_id is valid.
        osinterface::ThreadUtils::ThreadId locked_thread_id; ///< What thread has write lock.
        osinterface::OsTypes::UnsignedInt inner_lock_count; ///< how many locks inside the write lock do we have
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_ENTITY_H_ */
