/*
 * security_TransferEntityChecker.cpp
 */

#include "security_TransferEntityChecker.h"

#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Room.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "security_CheckerHelpers.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    TransferEntityChecker::TransferEntityChecker(void)
    {
    }

    // ----------------------------------------------------------------------
    TransferEntityChecker::~TransferEntityChecker()
    {
    }

    // ----------------------------------------------------------------------
    Result TransferEntityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        Result result = RESULT_SKIP;

        // Do some initial checks to validate we can cast everything into
        // containers, and if so determine which ones are rooms.
        //
        dbtype::ContainerPropertyEntity *target_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(entity_target.get());
        dbtype::ContainerPropertyEntity *source_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(entity_source.get());

        if ((not target_cpe) or (not source_cpe))
        {
            // Must be a container property entity to continue,
            return result;
        }

        dbinterface::EntityRef source_container =
            dbinterface::DatabaseAccess::instance()->get_entity(
                source_cpe->get_contained_by());
        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());
        dbtype::ContainerPropertyEntity *source_container_cpe = 0;

        if (source_container.valid())
        {
            source_container_cpe =
                dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    source_container.get());
        }

        if (not source_container_cpe)
        {
            // Must be contained by a valid container.
            return result;
        }

        // Determine:
        //   Is Source's container a room?
        //   Is Target a room?
        // This will be used to determine which sub-checker we will use.
        //
        const bool source_container_is_room = dynamic_cast<dbtype::Room *>(
            source_container_cpe);
        const bool target_is_room = dynamic_cast<dbtype::Room *>(target_cpe);

        if (source_container_is_room != target_is_room)  // XOR
        {
            result = check_room_to_entity(
                context,
                requester,
                program,
                entity_source,
                source_container,
                source_container_is_room,
                entity_target,
                target_is_room);
        }
        else if (source_container_is_room and target_is_room)
        {
            result = check_for_owner(
                context,
                requester,
                program,
                source_container,
                entity_target);
        }
        else if (context.has_run_as_requester())
        {
            result = check_for_owner(
                context,
                requester,
                program,
                entity_source,
                entity_target);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result TransferEntityChecker::check_for_owner(
        security::Context &context,
        dbinterface::EntityRef &requester,
        dbinterface::EntityRef &program,
        dbinterface::EntityRef &first,
        dbinterface::EntityRef &second)
    {
        Result result = RESULT_SKIP;
        Result source_result = RESULT_SKIP;
        Result target_result = RESULT_SKIP;

        if (CheckerHelpers::is_admin(
            source_result,
            context.has_run_as_requester(),
            first,
            requester,
            context.get_program(),
            program) and
        CheckerHelpers::is_admin(
            target_result,
            context.has_run_as_requester(),
            second,
            requester,
            context.get_program(),
            program))
        {
            if ((source_result == RESULT_ACCEPT) and
                target_result == RESULT_ACCEPT)
            {
                result = RESULT_ACCEPT;
            }
            else
            {
                result = RESULT_DENY;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result TransferEntityChecker::check_room_to_entity(
        security::Context &context,
        dbinterface::EntityRef &requester,
        dbinterface::EntityRef &program,
        dbinterface::EntityRef &to_move,
        dbinterface::EntityRef &to_move_container,
        const bool to_move_container_is_room,
        dbinterface::EntityRef &target,
        const bool target_is_room)
    {
        Result result = RESULT_SKIP;

        if (to_move_container_is_room == target_is_room)
        {
            // Not supported.
            return result;
        }

        // Check to see if both are in the same room.  Requester must be in
        // the same room too.
        //
        if (context.has_run_as_requester())
        {
            bool all_in_room = false;
            Result admin_result = RESULT_DENY;

            if (to_move_container_is_room)
            {
                // Picking up from room
                //
                all_in_room =
                    CheckerHelpers::is_entity_local(
                        to_move_container,
                        requester,
                        true) and
                    CheckerHelpers::is_entity_local(
                        to_move_container,
                        target,
                        true);

                if (all_in_room)
                {
                    CheckerHelpers::is_admin(
                        admin_result,
                        context.has_run_as_requester(),
                        target,
                        requester,
                        context.get_program(),
                        program);
                }
            }
            else
            {
                // Dropping off to room.
                //
                all_in_room =
                    CheckerHelpers::is_entity_local(
                        target,
                        requester,
                        true) and
                    CheckerHelpers::is_entity_local(
                        target,
                        to_move_container,
                        true);

                if (all_in_room)
                {
                    CheckerHelpers::is_admin(
                        admin_result,
                        context.has_run_as_requester(),
                        to_move_container,
                        requester,
                        context.get_program(),
                        program);
                }
            }

            if (all_in_room and (admin_result == RESULT_ACCEPT))
            {
                result = RESULT_ACCEPT;
            }
            else
            {
                result = RESULT_DENY;
            }
        }

        // If still not good, allow if context is owner/admin of to_move and
        // the target.
        //
        if (result != RESULT_ACCEPT)
        {
            CheckerHelpers::is_admin(
                result,
                context.has_run_as_requester(),
                to_move,
                requester,
                context.get_program(),
                program);

            if (result != RESULT_ACCEPT)
            {
                if (not CheckerHelpers::is_admin(
                    result,
                    context.has_run_as_requester(),
                    target,
                    requester,
                    context.get_program(),
                    program))
                {
                    result = RESULT_SKIP;
                }
            }
        }

        return result;
    }
}
}
