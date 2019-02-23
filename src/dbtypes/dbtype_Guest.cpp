/*
 * dbtype_Guest.cpp
 */

#include <string>

#include "dbtype_Guest.h"

#include "dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtype_Player.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Guest::Guest()
        : Player()
    {
    }

    // ----------------------------------------------------------------------
    Guest::Guest(const Id &id)
        : Player(id, ENTITYTYPE_guest, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Guest::~Guest()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Guest::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Guest(
                id,
                ENTITYTYPE_guest,
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
    bool Guest::set_password(
        const std::string &new_password,
        concurrency::WriterLockToken &token)
    {
        return false;
    }

    // ----------------------------------------------------------------------
    bool Guest::check_password(
        const std::string &password,
        concurrency::ReaderLockToken &token)
    {
        return false;
    }

    // ----------------------------------------------------------------------
    Guest::Guest(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : Player(id, type, version, instance, restoring)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */
