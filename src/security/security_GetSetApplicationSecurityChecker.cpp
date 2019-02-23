/*
 * security_GetSetApplicationSecurityChecker.cpp
 */

#include "security_GetSetApplicationSecurityChecker.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    GetSetApplicationSecurityChecker::GetSetApplicationSecurityChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    GetSetApplicationSecurityChecker::~GetSetApplicationSecurityChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    Result GetSetApplicationSecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        Result result = RESULT_SKIP;

        // No need to check for site admin; should be covered by admin checker.

        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());

        CheckerHelpers::is_admin(
            result,
            context.has_run_as_requester(),
            entity_target,
            application,
            requester,
            context.get_program(),
            program);

        return result;
    }
}
}