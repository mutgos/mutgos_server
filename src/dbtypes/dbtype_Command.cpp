/*
 * dbtype_Command.cpp
 */

#include "dbtype_Command.h"

#include "dbtype_Entity.h"
#include "dbtype_ActionEntity.h"

#include "logging/log_Logger.h"


namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Command::Command(void)
      : ActionEntity()
    {
    }

    // ----------------------------------------------------------------------
    Command::Command(const Id &id)
        : ActionEntity(id, ENTITYTYPE_command, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Command::~Command()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Command::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Command(
                id,
                ENTITYTYPE_command,
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
    Command::Command(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : ActionEntity(id, type, version, instance, restoring)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */
