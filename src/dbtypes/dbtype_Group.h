/*
 * dbtype_Group.h
 */

#ifndef MUTGOS_DBTYPE_GROUP_H_
#define MUTGOS_DBTYPE_GROUP_H_

#include <set>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/set.hpp>

#include <boost/thread/recursive_mutex.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Groups are a fundamental part of the MUTGOS security model.  What they
     * are is simple:  A list of IDs.  Because they are also Entities, they
     * can be referenced in a variety of ways.
     *
     * If an Id is in the group, it is eligible to be placed in the
     * disabled group.  When disabled, it is as if the Id is not in the group
     * at all.  This can be used for testing, temporarily disabling features,
     * etc.
     */
    class Group : public Entity
    {
    public:
        /**
         * Constructor used for deserialization of a Group.
         */
        Group();

        /**
         * Constructs a Property Group (final type).
         * @param id[in] The ID of the entity.
         */
        Group(const Id &id);

        virtual ~Group();

        /**
         * Creates a copy of this Group.
         * @param id[in] The new ID of the cloned Group.
         * @param version[in] The new version # of this Group.
         * @param instance[in] The new instance # of this Group.
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
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error.
         */
        virtual size_t mem_used_fields(void);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);

        /**
         * Adds the given ID to the group (locking).
         * This method will automatically get a lock.
         * @param id_to_add[in] The ID to add to the group.
         * @return True if successfully added.  False if it's already in the
         * group or an error occurs.
         */
        bool add_to_group(const Id &id_to_add);

        /**
         * Adds the given ID to the group.
         * @param id_to_add[in] The ID to add to the group.
         * @param token[in] The lock token.
         * @return True if successfully added.  False if it's already in the
         * group or an error occurs.
         */
        bool add_to_group(
            const Id &id_to_add,
            concurrency::WriterLockToken &token);

        /**
         * Removes the given ID from the group (locking).
         * This method will automatically get a lock.
         * @param id_to_remove[in] The ID to remove from the group.
         */
        void remove_from_group(const Id &id_to_remove);

        /**
         * Removes the given ID from the group.
         * @param id_to_remove[in] The ID to remove from the group.
         * @param token[in] The lock token.
         */
        void remove_from_group(
            const Id &id_to_remove,
            concurrency::WriterLockToken &token);


        /**
         * This method will automatically get a lock.
         * @param id_to_check[in] The ID to check.
         * @return True if the given ID is in the group.  If the ID is in the
         * disabled list or there is an error, this will return false.
         */
        bool is_in_group(const Id &id_to_check);

        /**
         * @param id_to_check[in] The ID to check.
         * @param token[in] The lock token.
         * @return True if the given ID is in the group.  If the ID is in the
         * disabled list or there is an error, this will return false.
         */
        bool is_in_group(
            const Id &id_to_check,
            concurrency::ReaderLockToken &token);


        /**
         * This method will automatically get a lock.
         * @return All entries in the group, even if excluded/disabled, and
         * with no filtering, or an empty list if error.
         */
        Entity::IdVector get_all_in_group(void);

        /**
         * @param token[in] The lock token.
         * @return All entries in the group, even if excluded/disabled, and
         * with no filtering, or an empty list if error.
         */
        Entity::IdVector get_all_in_group(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The first entry in the group, or a default ID if none.
         */
        Id first_group_entry(void);

        /**
         * @param token[in] The lock token.
         * @return The first entry in the group, or a default ID if none or
         * error.
         */
        Id first_group_entry(
            concurrency::ReaderLockToken &token);


        /**
         * This method will automatically get a lock.
         * @param current_id[in] The current position within the group.
         * @return The entry in the group after current_id, or a default ID if
         * none or error.
         */
        Id next_group_entry(const Id &current_id);

        /**
         * @param current_id[in] The current position within the group.
         * @param token[in] The lock token.
         * @return The entry in the group after current_id, or a default ID if
         * none or error.
         */
        Id next_group_entry(
            const Id &current_id,
            concurrency::ReaderLockToken &token);


        /**
         * Adds the given ID to the disabled group (locking).
         * This method will automatically get a lock.
         * @param id_to_add[in] The ID to add to the disabled group.
         * @return True if successfully added.  False if it's already in the
         * disabled group, not present in the group, or an error occurs.
         */
        bool add_to_disabled_group(const Id &id_to_add);

        /**
         * Adds the given ID to the disabled group.
         * @param id_to_add[in] The ID to add to the disabled group.
         * @param token[in] The lock token.
         * @return True if successfully added.  False if it's already in the
         * disabled group, not present in the group, or an error occurs.
         */
        bool add_to_disabled_group(
            const Id &id_to_add,
            concurrency::WriterLockToken &token);


        /**
         * Removes the given ID from the disabled group (locking).
         * This method will automatically get a lock.
         * @param id_to_remove[in] The ID to remove from the disabled group.
         */
        void remove_from_disabled_group(const Id &id_to_remove);

        /**
         * Removes the given ID from the disabled group.
         * @param id_to_remove[in] The ID to remove from the disabled group.
         * @param token[in] The lock token.
         */
        void remove_from_disabled_group(
            const Id &id_to_remove,
            concurrency::WriterLockToken &token);


        /**
         * This method will automatically get a lock.
         * @param id_to_check[in] The ID to check.
         * @return True if the given ID is in the disabled group.  If
         * there is an error, this will return false.
         */
        bool is_in_disabled_group(const Id &id_to_check);

        /**
         * @param id_to_check[in] The ID to check.
         * @param token[in] The lock token.
         * @return True if the given ID is in the disabled group.  If
         * there is an error, this will return false.
         */
        bool is_in_disabled_group(
            const Id &id_to_check,
            concurrency::ReaderLockToken &token);


        /**
         * This method will automatically get a lock.
         * @return The first entry in the disabled group, or a default ID if
         * none.
         */
        Id first_disabled_group_entry(void);

        /**
         * @param token[in] The lock token.
         * @return The first entry in the disabled group, or a default ID if
         * none or error.
         */
        Id first_disabled_group_entry(
            concurrency::ReaderLockToken &token);


        /**
         * This method will automatically get a lock.
         * @param current_id[in] The current position within the disabled group.
         * @return The entry in the disabled group after current_id, or a
         * default ID if none or error.
         */
        Id next_disabled_group_entry(const Id &current_id);

        /**
         * @param current_id[in] The current position within the disabled group.
         * @param token[in] The lock token.
         * @return The entry in the disabled group after current_id, or a
         * default ID if none or error.
         */
        Id next_disabled_group_entry(
            const Id &current_id,
            concurrency::ReaderLockToken &token);

    protected:

        /**
         * Constructs a Group with a provided type.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        Group(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * Copies fields from this Group to the provided Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:

        typedef std::set<Id> GroupSet;

        GroupSet group_ids;  ///< Members of the group
        GroupSet disabled_ids; ///< Members of the group who are temporarily not a member

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Entity>(*this);

            ar & group_ids;
            ar & disabled_ids;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Entity>(*this);

            ar & group_ids;
            ar & disabled_ids;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_GROUP_H_ */
