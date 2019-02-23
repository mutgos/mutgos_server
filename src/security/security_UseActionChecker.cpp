/*
 * security_UseActionChecker.cpp
 */

#include "security_UseActionChecker.h"

#include "dbtypes/dbtype_ActionEntity.h"
#include "dbtypes/dbtype_Group.h"
#include "dbtypes/dbtype_Lock.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    UseActionChecker::UseActionChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    UseActionChecker::~UseActionChecker()
    {
    }

    // ----------------------------------------------------------------------
    Result UseActionChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_SKIP;

        // No need to check for site admin; should be covered by admin checker.

        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        // Left at default as a variable to avoid compile issue
        dbinterface::EntityRef program;

        // TODO May need to add a special 'inherited locality' check so you can't just execute random exits

        CheckerHelpers::has_permission(
            result,
            dbtype::SECURITYFLAG_read,
            BHANDLING_INCLUDE_BASIC,
            false,
            entity_target,
            requester,
            dbtype::Id(),
            program);

        if (result == RESULT_ACCEPT)
        {
            dbtype::ActionEntity *action_ptr =
                dynamic_cast<dbtype::ActionEntity *>(entity_target.get());

            if (action_ptr)
            {
                dbtype::Lock lock = action_ptr->get_action_lock();

                switch (lock.get_lock_type())
                {
                    case dbtype::Lock::LOCK_BY_ID:
                    case dbtype::Lock::LOCK_BY_PROPERTY:
                    {
                        // TODO How to do security for property checking?  More difficult than it sounds!

                        concurrency::WriterLockToken token(*requester.get());

                        if (not lock.evaluate(requester.get(), token))
                        {
                            result = RESULT_DENY;
                        }

                        break;
                    }

                    case dbtype::Lock::LOCK_BY_GROUP:
                    {
                        dbinterface::EntityRef group_ref =
                            dbinterface::DatabaseAccess::instance()->get_entity(
                                lock.get_id());

                        if (not group_ref.valid())
                        {
                            result = RESULT_SKIP;
                        }
                        else
                        {
                            dbtype::Group *group_ptr =
                                dynamic_cast<dbtype::Group *>(group_ref.get());

                            if (not group_ptr)
                            {
                                result = RESULT_SKIP;
                            }
                            else
                            {
                                concurrency::ReaderLockToken token(*group_ptr);

                                if (not lock.evaluate(
                                    requester.get(),
                                    group_ptr,
                                    token))
                                {
                                    result = RESULT_DENY;
                                }
                            }
                        }

                        break;
                    }

                    default:
                    {
                        // TODO Enhance this later.  Logging?
                        result = RESULT_SKIP;
                        break;
                    }
                }
            }
        }

        return result;
    }
}
}