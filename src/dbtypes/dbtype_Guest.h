/*
 * dbtype_Guest.h
 */

#ifndef MUTGOS_DBTYPE_GUEST_H
#define MUTGOS_DBTYPE_GUEST_H

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"

#include "dbtype_Player.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a 'Guest', which is a restricted form of a player.  Typically
     * used by people who want to explore a world before signing up for proper
     * access.
     */
    class Guest : public Player
    {
    public:
        /**
         * Constructor used for deserialization of a Guest.
         */
        Guest();

        /**
         * Constructs a Guest (final type).
         * @param id[in] The ID of the entity.
         */
        Guest(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Guest();

        /**
         * Creates a copy of this Guest.
         * @param id[in] The new ID of the cloned Capability.
         * @param version[in] The new version # of this Capability.
         * @param instance[in] The new instance # of this Capability.
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
         * Guests cannot really have a password; this call will always fail.
         * @param new_password[in] The new, unencrypted password.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_password(
            const std::string &new_password,
            concurrency::WriterLockToken &token);

        /**
         * Guests cannot really have a password; this call will always fail.
         * @param password[in] The unencrypted password to compare the
         * stored password against.
         * @param token[in] The lock token.
         * @return True if the passwords match, false if not or error.
         */
        bool check_password(
            const std::string &password,
            concurrency::ReaderLockToken &token);

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
        Guest(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

    private:

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Player>(*this);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Player>(*this);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_GUEST_H
