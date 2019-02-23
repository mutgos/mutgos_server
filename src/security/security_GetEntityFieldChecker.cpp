/*
 * security_GetEntityFieldChecker.cpp
 */

#include "security_GetEntityFieldChecker.h"
#include "security_OperationsCapabilities.h"
#include "security_CheckerHelpers.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_EntityType.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    GetEntityFieldChecker::GetEntityFieldChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    GetEntityFieldChecker::~GetEntityFieldChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    Result GetEntityFieldChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_DENY;

        if ((operation == OPERATION_ENTITY_TOSTRING))
        {
            // TODO Temp!  Allows toString() if read permissions.

            // Pick an attribute that requires full read permissions.
            result = security_check(
                operation,
                context,
                entity_target,
                dbtype::ENTITYFIELD_note);
        }
        else if (operation == OPERATION_GET_CONTAINS)
        {
            result = locality_security_check(context, entity_target);
        }
        else if ((operation == OPERATION_GET_ACTIONS) and entity_target.valid())
        {
            const dbtype::EntityType type = entity_target.type();

            if ((type == dbtype::ENTITYTYPE_player) or
                (type == dbtype::ENTITYTYPE_guest))
            {
                // You can only get actions of these types if you have explicit
                // read permission.  This is to avoid personal player
                // actions from being accidentally used by others.
                //
                result = security_check(
                    operation,
                    context,
                    entity_target,
                    dbtype::ENTITYFIELD_references);
            }
            else
            {
                // Other types can get their actions listed normally.
                result = locality_security_check(context, entity_target);
            }
        }
        else
        {
            // Not an operation we support.
            result = RESULT_SKIP;
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result GetEntityFieldChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        Result result = RESULT_DENY;

        switch (entity_field)
        {
            case dbtype::ENTITYFIELD_id:
            {
                result = RESULT_ACCEPT;
                break;
            }

            case dbtype::ENTITYFIELD_name:
            case dbtype::ENTITYFIELD_player_display_name:
            case dbtype::ENTITYFIELD_puppet_display_name:
            {
                // Special rules
                result = name_security_check(context, entity_target);
                break;
            }

            case dbtype::ENTITYFIELD_type:
            case dbtype::ENTITYFIELD_owner:
            case dbtype::ENTITYFIELD_flags:
            case dbtype::ENTITYFIELD_deleted_flag:
            case dbtype::ENTITYFIELD_contained_by:
            case dbtype::ENTITYFIELD_action_contained_by:
            case dbtype::ENTITYFIELD_action_commands:
            {
                result = locality_security_check(context, entity_target);
                break;
            }

            default:
            {
                // Attribute is not considered 'base'.
                // Only allow if has proper 'read' (not base) permissions,
                // regardless of where the Entity is located.
                //
                dbinterface::EntityRef requester =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        context.get_requester());
                dbinterface::EntityRef program =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        context.get_program());

                result = RESULT_SKIP;

                CheckerHelpers::has_permission(
                    result,
                    dbtype::SECURITYFLAG_read,
                    BHANDLING_EXCLUDE_BASIC,
                    context.has_run_as_requester(),
                    entity_target,
                    requester,
                    context.get_program(),
                    program);

                break;
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    // TODO Should name be considered a 'base' attribute?
    Result GetEntityFieldChecker::name_security_check(
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_DENY;

        if (not entity_target.valid())
        {
            // Invalid target, can't evaluate it.
            return RESULT_SKIP;
        }
        else if (context.has_capability(CAPABILITY_ANY_ID_TO_NAME))
        {
            // If you have the capability, you can always get the name.
            return RESULT_ACCEPT;
        }
        else
        {
            // No capability, see if target is owned, has permission, is in
            // inventory (if running as requester), or is in the same room.
            //
            if (context.has_run_as_requester() and
                (entity_target->get_entity_owner() == context.get_requester()))
            {
                // Running as requester and the requester owns it.
                result = RESULT_ACCEPT;
            }
            else
            {
                dbinterface::EntityRef requester =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        context.get_requester());

                if (not requester.valid())
                {
                    result = RESULT_SKIP;
                }
                else
                {
                    // See if we are local.
                    //
                    if (CheckerHelpers::is_entity_local_via_inventory(
                        requester,
                        entity_target,
                        true))
                    {
                        result = RESULT_ACCEPT;
                    }

                    if (result == RESULT_DENY)
                    {
                        // Lastly, simply see if we have read permissions,
                        // which means we can get the name no matter where or
                        // what the Entity is.
                        //
                        dbinterface::EntityRef program =
                            dbinterface::DatabaseAccess::instance()->get_entity(
                                context.get_program());

                        CheckerHelpers::has_permission(
                            result,
                            dbtype::SECURITYFLAG_read,
                            BHANDLING_EXCLUDE_BASIC,
                            context.has_run_as_requester(),
                            entity_target,
                            requester,
                            context.get_program(),
                            program);


                        /**  Ignore for now; base flag isn't meant to be
                         *   used like this.
                        // Lastly, simply see if we have base permissions,
                        // which means we can get the name no matter where or
                        // what the Entity is.
                        //
                        if (context.has_run_as_requester())
                        {
                            // Use requester and program permissions.
                            //
                            if (context.get_program().is_default())
                            {
                                // Check requester only (native program, etc)
                                //
                                result = (CheckerHelpers::has_permission(
                                    entity_target.id(),
                                    entity_target->get_entity_security(),
                                    requester,
                                    dbtype::SECURITYFLAG_read,
                                    BASEHANDLING_INCLUDE_BASE_BUT_IGNORE_OTHER) ?
                                        RESULT_ACCEPT : RESULT_DENY);
                            }
                            else
                            {
                                // Check requester and program.
                                //
                                dbinterface::EntityRef program =
                                    dbinterface::DatabaseAccess::instance()->
                                        get_entity(context.get_program());

                                if (not program.valid())
                                {
                                    // If we're running as a program that might
                                    // be deleted or invalid, we can't evaluate
                                    // security.
                                    result = RESULT_SKIP;
                                }
                                else
                                {
                                     result = (CheckerHelpers::has_permission(
                                            entity_target.id(),
                                            entity_target->get_entity_security(),
                                            requester,
                                            program,
                                            dbtype::SECURITYFLAG_read,
                                            BASEHANDLING_INCLUDE_BASE_BUT_IGNORE_OTHER) ?
                                                RESULT_ACCEPT : RESULT_DENY);
                                }
                            }
                        }
                        else
                        {
                            // Use program permissions only.
                            //
                            if (context.get_program().is_default())
                            {
                                // Native program.  No way to directly refer to
                                // it in-game, so it does not have permission.
                                 result = RESULT_DENY;
                            }
                            else
                            {
                                dbinterface::EntityRef program =
                                    dbinterface::DatabaseAccess::instance()->
                                        get_entity(context.get_program());

                                if (not program.valid())
                                {
                                    // If we're running as a program that might
                                    // be deleted or invalid, we can't evaluate
                                    // security.
                                    result = RESULT_SKIP;
                                }
                                else
                                {
                                    result = (CheckerHelpers::has_permission(
                                            entity_target.id(),
                                            entity_target->get_entity_security(),
                                            program,
                                            dbtype::SECURITYFLAG_read,
                                            BASEHANDLING_INCLUDE_BASE_BUT_IGNORE_OTHER) ?
                                                RESULT_ACCEPT : RESULT_DENY);
                                }
                            }
                        }

                        */
                    }
                }
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result GetEntityFieldChecker::locality_security_check(
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_SKIP;
        dbinterface::EntityRef requester =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbinterface::EntityRef program =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_program());

        if (requester.valid())
        {
            if (CheckerHelpers::is_entity_local_via_inventory(
                requester,
                entity_target,
                context.has_run_as_requester()))
            {
                // Local.  Base or read flag gives access.
                //
                CheckerHelpers::has_permission(
                    result,
                    dbtype::SECURITYFLAG_read,
                    BHANDLING_INCLUDE_BASIC,
                    context.has_run_as_requester(),
                    entity_target,
                    requester,
                    context.get_program(),
                    program);
            }
            else
            {
                // Not local.  Read flag (or base on list) only gives
                // access.
                //
                CheckerHelpers::has_permission(
                    result,
                    dbtype::SECURITYFLAG_read,
                    BHANDLING_INCLUDE_BASIC_BUT_IGNORE_OTHER,
                    context.has_run_as_requester(),
                    entity_target,
                    requester,
                    context.get_program(),
                    program);
            }
        }

        return result;
    }
}
}