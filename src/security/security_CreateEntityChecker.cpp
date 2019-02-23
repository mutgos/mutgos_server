/*
 * security_CreateEntityChecker.cpp
 */

#include "security_CreateEntityChecker.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    CreateEntityChecker::CreateEntityChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    CreateEntityChecker::~CreateEntityChecker()
    {
    }

    // ----------------------------------------------------------------------
    Result CreateEntityChecker::security_check(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type)
    {
        Result result = RESULT_DENY;

        switch (entity_type)
        {
            case dbtype::ENTITYTYPE_player:
            case dbtype::ENTITYTYPE_guest:
            {
                if (context.has_capability(CAPABILITY_CREATE_PLAYER))
                {
                    result = RESULT_ACCEPT;
                }

                break;
            }

            case dbtype::ENTITYTYPE_capability:
            case dbtype::ENTITYTYPE_program:
            {
                if (context.has_admin())
                {
                    result = RESULT_ACCEPT;
                }

                break;
            }

            default:
            {
                if (context.has_capability(CAPABILITY_BUILDER))
                {
                    result = RESULT_ACCEPT;
                }

                break;
            }
        }

        return result;
    }
}
}
