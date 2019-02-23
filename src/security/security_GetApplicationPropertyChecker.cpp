/*
 * security_GetApplicationPropertyChecker.cpp
 */

#include "security_GetApplicationPropertyChecker.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    GetApplicationPropertyChecker::GetApplicationPropertyChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    GetApplicationPropertyChecker::~GetApplicationPropertyChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    Result GetApplicationPropertyChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        // TODO Will need to enhance to allow reading of properties only if in same room, to prevent people from seeing descs remotely

        Result result = RESULT_SKIP;

        // No need to check for site admin; should be covered by admin checker.

        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());

        CheckerHelpers::has_permission(
            result,
            dbtype::SECURITYFLAG_read,
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
