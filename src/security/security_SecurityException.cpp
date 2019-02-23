/*
 * security_SecurityException.cpp
 */

#include "security_SecurityException.h"

#include "security_OperationsCapabilities.h"
#include "security_Context.h"

#include "dbinterface/dbinterface_EntityRef.h"

#include "dbtypes/dbtype_EntityType.h"

namespace
{
    static const std::string INVALID = "INVALID";
}

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context)
    {
        message = "Permission denied for operation "
            + operation_to_string(operation);
    }

    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type)
    {
        message = "Permission denied for operation "
            + operation_to_string(operation)
            + ", entity type: " + dbtype::entity_type_to_string(entity_type);
    }

    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        message = "Permission denied for operation "
                  + operation_to_string(operation)
                  + ", entity target: ";

        if (entity_target.valid())
        {
            message += entity_target.id().to_string(true);
        }
        else
        {
            message += INVALID;
        }
    }

    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        message = "Permission denied for operation "
                  + operation_to_string(operation)
                  + ", entity target: ";

        if (entity_target.valid())
        {
            message += entity_target.id().to_string(true);
        }
        else
        {
            message += INVALID;
        }

        message += ", application: " + application;
    }

    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        message = "Permission denied for operation "
                  + operation_to_string(operation)
                  + ", entity target: ";

        if (entity_target.valid())
        {
            message += entity_target.id().to_string(true);
        }
        else
        {
            message += INVALID;
        }

        message += ", field: " + dbtype::entity_field_to_string(entity_field);
    }

    // ----------------------------------------------------------------------
    SecurityException::SecurityException(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        message = "Permission denied for operation "
                  + operation_to_string(operation)
                  + ", entity target: ";

        if (entity_target.valid())
        {
            message += entity_target.id().to_string(true);
        }
        else
        {
            message += INVALID;
        }

        message += ", entity source: ";

        if (entity_source.valid())
        {
            message += entity_source.id().to_string(true);
        }
        else
        {
            message += INVALID;
        }
    }

    // ----------------------------------------------------------------------
    SecurityException::~SecurityException()
    {
    }
}

}
