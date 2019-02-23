/*
 * dbtype_Capability.h
 */

#ifndef MUTGOS_DBTYPE_CAPABILITY_H_
#define MUTGOS_DBTYPE_CAPABILITY_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Group.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Capabilities are just like Groups, except their name is unique
     * amongst all Capabilities on the world instance.  It is used to
     * specifically designate a list of IDs that can perform a certain
     * operation without requiring a series of properties on the root
     * object.  This also implies that creation and modification of capabilities
     * must be restricted to administrators.
     */
    class Capability : public Group
    {
    public:
        /**
         * Constructor used for deserialization of a Capability.
         */
        Capability();

        /**
         * Constructs a Capability (final type).
         * @param id[in] The ID of the entity.
         */
        Capability(const Id &id);

        virtual ~Capability();

        /**
         * Creates a copy of this Capability.
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

    protected:

        /**
         * Constructs a Capability with a provided type.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        Capability(
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
            ar & boost::serialization::base_object<Group>(*this);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Group>(*this);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_CAPABILITY_H_ */
