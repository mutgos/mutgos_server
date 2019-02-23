/*
 * dbtype_Capability.cpp
 */

#include "dbtypes/dbtype_Capability.h"

#include "logging/log_Logger.h"

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
    // ----------------------------------------------------------------------
    Capability::Capability()
      : Group()
    {
    }

    // ----------------------------------------------------------------------
    Capability::Capability(const Id &id)
    : Group (
        id,
        ENTITYTYPE_capability,
        0,
        0)
    {
    }

    // ----------------------------------------------------------------------
    Capability::~Capability()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Capability::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Capability(
                id,
                ENTITYTYPE_capability,
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
    Capability::Capability(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : Group(id, type, version, instance, restoring)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */
