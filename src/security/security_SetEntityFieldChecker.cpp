/*
 * security_SetEntityFieldChecker.cpp
 */

#include "security_SetEntityFieldChecker.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "dbtypes/dbtype_Group.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    SetEntityFieldChecker::SetEntityFieldChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    SetEntityFieldChecker::~SetEntityFieldChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    Result SetEntityFieldChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        Result result = RESULT_SKIP;

        // No need to check for site admin; should be covered by admin checker.

        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());

        // Security and ownership can only be changed by an admin, but the
        // others can be changed by those with correct permissions.
        //
        if ((entity_field == dbtype::ENTITYFIELD_owner) or
            (entity_field == dbtype::ENTITYFIELD_security))
        {
            CheckerHelpers::is_admin(
                result,
                context.has_run_as_requester(),
                entity_target,
                requester,
                context.get_program(),
                program);
        }
        else
        {
            CheckerHelpers::has_permission(
                result,
                dbtype::SECURITYFLAG_write,
                BHANDLING_EXCLUDE_BASIC,
                context.has_run_as_requester(),
                entity_target,
                requester,
                context.get_program(),
                program);
        }

        return result;
    }
}
}