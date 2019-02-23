/*
 * dbtype_Region.cpp
 */

#include <string>

#include "dbtype_Entity.h"
#include "dbtype_ContainerPropertyEntity.h"
#include "dbtype_Region.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"


namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Region::Region()
      : ContainerPropertyEntity()
    {
    }

    // ----------------------------------------------------------------------
    Region::Region(const Id &id)
      : ContainerPropertyEntity(id, ENTITYTYPE_region, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Region::~Region()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Region::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Region(
                id,
                ENTITYTYPE_region,
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
    Region::Region(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : ContainerPropertyEntity(id, type, version, instance, restoring)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */
