/*
 * dbtype_Thing.h
 */

#ifndef MUTGOS_DBTYPE_THING_H
#define MUTGOS_DBTYPE_THING_H


#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Lock.h"

#include "dbtype_ContainerPropertyEntity.h"
#include "dbtype_PropertyDirectory.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a thing, item, etc in the game.  What a 'thing' is can
     * vary widely from world to world, builder to builder.
     */
    class Thing : public ContainerPropertyEntity
    {
    public:
        /**
         * Constructor used for deserialization of a Thing.
         */
        Thing();

        /**
         * Constructs a Thing (final type).
         * @param id[in] The ID of the entity.
         */
        Thing(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Thing();

        /**
         * Creates a copy of this Thing.
         * @param id[in] The new ID of the cloned Puppet.
         * @param version[in] The new version # of this Puppet.
         * @param instance[in] The new instance # of this Puppet.
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
         * Sets the Thing's home.
         * @param home[in] The home for the Thing.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_thing_home(
            const Id &home,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Thing's home.
         * This method will automatically get a lock.
         * @param home[in] The home for the Thing.
         * @return True if success.
         */
        bool set_thing_home(const Id &home);

        /**
         * @param token[in] token The lock token.
         * @return The Thing's home, or default if none or error.
         */
        Id get_thing_home(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The Thing's home, or default if none or error.
         */
        Id get_thing_home(void);

        /**
         * Sets the Thing's Lock.
         * @param lock[in] The lock to set.
         * @param token[in] token The lock token.
         * @return True if success.
         */
        bool set_thing_lock(
            const Lock &lock,
            concurrency::WriterLockToken &token);

        /**
         * Sets the Thing's Lock.
         * This method will automatically get a lock.
         * @param lock[in] The lock to set.
         * @return True if success.
         */
        bool set_thing_lock(const Lock &lock);

        /**
         * @param token[in] token The lock token.
         * @return The Thing's Lock, or an invalid Lock if error.
         */
        Lock get_thing_lock(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The Thing's Lock, or an invalid Lock if error.
         */
        Lock get_thing_lock(void);

        /**
         * An 'invalid' lock means it is unlocked.
         * @param token[in] The lock token.
         * @return The type of lock on this Thing, or invalid if error.
         */
        Lock::LockType get_thing_lock_type(concurrency::ReaderLockToken &token);

        /**
         * If lock type is by ID or group, this will indicate what ID or
         * group it is locked against.
         * @param token[in] The lock token.
         * @return The ID associated with the lock, or default if invalid or
         * not the correct type of lock, or error.
         */
        Id get_thing_lock_id(concurrency::ReaderLockToken &token);

        /**
         * If lock type is by property, this will indicate what property will
         * be retrieved.
         * @param token[in] The lock token.
         * @return The path associated with the lock, or an empty string if
         * invalid, not the correct type of lock, or error.
         */
        PropertyDirectory::PathString get_thing_lock_path(
            concurrency::ReaderLockToken &token);

        /**
         * Evaluates the lock to see if the entity passes.  Used for property
         * or by ID type locks.
         * @param token[in] The lock token for this Thing.
         * @param entity_ptr[in] The Entity to evaluate.
         * @param entity_token[in] The lock token for the entity being
         * evaluated.
         * @return True if the lock passes, false if not or error.
         */
        bool evalute_lock(
            concurrency::ReaderLockToken &token,
            Entity *entity_ptr,
            concurrency::WriterLockToken &entity_token);

        /**
         * Evaluates the lock to see if the entity passes.  Used for group
         * type locks.
         * @param token[in] The lock token for this Thing.
         * @param entity_ptr[in] The Entity to evaluate.
         * @param group_ptr[in] The Group we are locked against.
         * @param group_token[in] The lock for the Group.
         * @return True if the lock passes, false if not or error.
         */
        bool evaluate_lock(
            concurrency::ReaderLockToken &token,
            Entity *entity_ptr,
            Group *group_ptr,
            concurrency::ReaderLockToken &group_token);

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
        Thing(
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
         * Copies fields from this Thing to the provided
         * Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:

        Id thing_home; ///< Where the Thing goes when sent home
        Lock thing_lock; ///< Pickup lock

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);

            ar & thing_home;
            ar & thing_lock;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);

            ar & thing_home;
            ar & thing_lock;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_THING_H
