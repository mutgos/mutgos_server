/*
 * dbtype_Region.h
 */

#ifndef MUTGOS_DBTYPE_REGION_H_
#define MUTGOS_DBTYPE_REGION_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtype_ContainerPropertyEntity.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"


namespace mutgos
{
namespace dbtype
{
    /**
     * A special type of Entity that can contain Rooms.  Rooms can only
     * be contained by a Region.  Regions may contain other Regions.
     */
    class Region : public ContainerPropertyEntity
    {
    public:
        /**
         * Constructor used for deserialization of a Region.
         */
        Region();

        /**
         * Constructs a Region (final type).
         * @param id[in] The ID of the entity.
         */
        Region(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Region();

        /**
         * Creates a copy of this Region.
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
         * Constructs an Entity with a provided type.  Used by subclasses.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        Region(
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
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

    };
}
}

#endif //MUTGOS_DBTYPE_REGION_H_
