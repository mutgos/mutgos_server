/*
 * security_DeleteEntityChecker.cpp
 */

#include "security_DeleteEntityChecker.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    DeleteEntityChecker::DeleteEntityChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    DeleteEntityChecker::~DeleteEntityChecker()
    {
    }

    // ----------------------------------------------------------------------
    Result DeleteEntityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_SKIP;

        // No need to check for site admin; should be covered by admin checker.

        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());

        if (not context.has_capability(CAPABILITY_BUILDER))
        {
            // Must have builder capability to even try and delete stuff.
            result = RESULT_DENY;
        }
        else
        {
            CheckerHelpers::is_admin(
                result,
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