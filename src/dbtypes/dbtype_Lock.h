/*
 * Lock.h
 */

#ifndef MUTGOS_DBTYPE_LOCK_H
#define MUTGOS_DBTYPE_LOCK_H

#include <string>
#include <stddef.h>

#include "logging/log_Logger.h"

#include "dbtype_Id.h"
#include "dbtype_Entity.h"
#include "dbtype_Group.h"
#include "dbtype_PropertyData.h"
#include "dbtype_PropertyDataSerializer.h"
#include "dbtype_PropertyDirectory.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include "text/text_StringConversion.h"

#include "concurrency/concurrency_ReaderLockToken.h"

namespace mutgos
{
namespace dbtype
{
    // TODO Enhance to be a proper lock

    /**
     * A Lock is used to allow only selected Entities access to something.
     * Unlike Groups or other permission types, a Lock is more dynamic and
     * must be evaluated each time to determine if something passes the lock.
     *
     * Currently a Lock can lock against these things (only one at a time may
     * be used):
     * 1.  A specific Entity
     * 2.  A Group (Entity being evaluated must be in group)
     * 3.  A property (Entity being evaluated must have the property and the
     * specified value).
     *
     * This is a very basic, proof of concept implementation that will
     * need to be enhanced at a later time.  It is not designed to be
     * subclassed.  Security is bypassed when reading - it is assumed to
     * have been validated earlier.
     *
     * All locking must be provided by whatever class is holding an instance
     * of this one.
     *
     * This class is self contained.  It is unable to retrieve Entities
     * directly, nor does it update references.  It cannot be subclassed.
     */
    class Lock
    {
    public:
        /**
         * The lock types supported.
         */
        enum LockType
        {
            LOCK_INVALID, // Default, Lock not valid.
            LOCK_BY_ID,  // Passes lock if specific ID
            LOCK_BY_GROUP, // Passes lock if in group
            LOCK_BY_PROPERTY // Passes lock if property exists with value
        };

        /**
         * Default constructor, used for new lock or restoring.
         */
        Lock();

        /**
         * Copy constructor.
         */
        Lock(const Lock &rhs);

        /**
         * Destructor.
         */
        ~Lock();

        /**
         * Assignment operator.
         */
        Lock &operator=(const Lock &rhs);

        /**
         * @return The approximate amount of memory used by this Lock, in bytes.
         */
        size_t mem_used(void) const;

        /**
         * @return Detailed information about the lock in string format,
         * suitable for a log.
         */
        std::string to_string(void) const;

        /**
         * @return True if this lock is valid (not defaulted).
         */
        bool lock_valid(void) const;

        /**
         * @return The lock type.
         */
        LockType get_lock_type(void) const;

        /**
         * @return The ID locked against (could be Group or generic Entity).
         * If not ID locked against, returns a default ID.
         */
        const Id &get_id(void) const;

        /**
         * @return The path (including application) locked against, or empty
         * string if none.
         */
        const PropertyDirectory::PathString &get_path(void) const;

        /**
         * Do not delete the returned pointer!
         * @return The lock path data (the value being compared against),
         * or null if not locked by data.
         */
        const PropertyData *get_path_data(void) const;

        /**
         * 'Unlocks' the Lock by clearing out all lock parameters, marking it
         * invalid.
         */
        void unlock(void);

        /**
         * Locks against the given Entity (may also be a group).
         * This overwrites any existing locking on this instance.
         * @param entity[in] The Entity to lock against.
         * @param token[in] The lock token for the Entity.
         * @param not_result[in] When evaluated, if true the result is to
         * be 'not'ed.
         * @return True if success.
         */
        bool lock_by_entity(
            Entity *entity,
            concurrency::ReaderLockToken &token,
            const bool not_result = false);

        /**
         * Locks against the given property and value.
         * This overwrites any existing locking on this instance.
         * @param path[in] The full path (including application) to the property
         * locked against.
         * @param data[in] The contents of the property.  Anything wishing
         * to pass the lock must have the same contents.
         * @param not_result[in] When evaluated, if true the result is to
         * be 'not'ed.
         */
        bool lock_by_property(
            const PropertyDirectory::PathString &path,
            const PropertyData &data,
            const bool not_result = false);

        /**
         * If this is locked against a property or an Entity (non-Group),
         * evaluate the lock against the provided Entity.
         * Security checks are assumed to have already been performed.
         * @param entity_ptr[in] The Entity to test (non-group lock only).
         * @param token[in] The lock token for entity.
         * @return True if entity passes the lock, false if not or error or
         * not valid.
         */
        bool evaluate(Entity *entity_ptr, concurrency::WriterLockToken &token);

        /**
         * If this is locked against a group, evaluate the entity against the
         * group.
         * @param entity_ptr[in] The Entity to test (non-group lock only).
         * @param group_ptr[in] The Group this is locked against.  Must be the
         * same group it was locked with.
         * @param group_token[in] The lock token for the Group.
         * @return True if entity passes the lock, false if not or error or
         * not valid.
         */
        bool evaluate(
            Entity *entity_ptr,
            Group *group_ptr,
            concurrency::ReaderLockToken &group_token);

    private:

        LockType lock_type; ///< What type of lock this is
        Id *lock_id; ///< The ID being locked against, null if N/A
        PropertyDirectory::PathString *lock_path; ///< Path of lock property
        PropertyData *lock_path_data; ///< Data value for locked property
        bool operation_not; ///< True if evaluation result is to be 'not'ed.

        // Disabled
        bool operator==(const Lock &rhs) const;

        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & lock_type;

            // Save only the attributes in use by the lock type
            switch (lock_type)
            {
                case LOCK_BY_ID:
                case LOCK_BY_GROUP:
                {
                    ar & (*lock_id);
                    break;
                }

                case LOCK_BY_PROPERTY:
                {
                    ar & *lock_path;
                    PropertyDataSerializer::save(lock_path_data, ar, version);
                    break;
                }

                default:
                {
                    const std::string enum_string = text::to_string(
                        (int) lock_type);

                    LOG(fatal, "dbtypes", "save()",
                        "lock_type is unknown:  " + enum_string +
                        ".   Will not deserialize properly.");

                    break;
                }
            }

            ar & operation_not;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            delete lock_id;
            lock_id = 0;

            delete lock_path;
            lock_path = 0;

            delete lock_path_data;
            lock_path_data = 0;

            ar & lock_type;

            switch (lock_type)
            {
                case LOCK_BY_ID:
                case LOCK_BY_GROUP:
                {
                    lock_id = new Id();
                    ar & (*lock_id);
                    break;
                }

                case LOCK_BY_PROPERTY:
                {
                    lock_path = new PropertyDirectory::PathString();
                    ar & (*lock_path);
                    lock_path_data = PropertyDataSerializer::load(ar, version);

                    if (not lock_path_data)
                    {
                        LOG(fatal, "dbtypes", "load()",
                            "Could not deserialize lock_path_data!");
                    }

                    break;
                }

                default:
                {
                    const std::string enum_string = text::to_string(
                            (int) lock_type);

                    LOG(fatal, "dbtypes", "load()",
                        "lock_type is unknown:  " + enum_string +
                        ".   Will not deserialize properly.");

                    break;
                }
            }

            ar & operation_not;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_LOCK_H
