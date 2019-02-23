/*
 * dbtype_Room.cpp
 */

#include <string>

#include "dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtype_ContainerPropertyEntity.h"
#include "dbtype_Room.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"


namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Room::Room()
        : ContainerPropertyEntity()
    {
    }

    // ----------------------------------------------------------------------
    Room::Room(const Id &id)
        : ContainerPropertyEntity(id, ENTITYTYPE_room, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Room::~Room()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Room::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Room(
                id,
                ENTITYTYPE_room,
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
    Room::Room(
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
