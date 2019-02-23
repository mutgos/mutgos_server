/*
 * security_CheckerHelpers.cpp
 */

#include "security_CheckerHelpers.h"

#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Group.h"
#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_ActionEntity.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace security
{
    // TODO Could stand to do some refactoring here...

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_entity_local(
        dbinterface::EntityRef &source,
        dbinterface::EntityRef &target,
        const bool include_source_inventory)
    {
        bool result = false;

        if ((not source.valid()) or (not target.valid()))
        {
            return result;
        }

        dbtype::ContainerPropertyEntity * const source_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                source.get());
        dbtype::ContainerPropertyEntity * target_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                target.get());
        dbinterface::EntityRef action_container;

        if (not target_cpe)
        {
            // This might be an Action, so set the target to the Entity that
            // contains the Action.
            //
            dbtype::ActionEntity * const action_ptr =
                dynamic_cast<dbtype::ActionEntity *>(target.get());

            if (action_ptr)
            {
                action_container =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                      action_ptr->get_action_contained_by());

                if (action_container.valid())
                {
                    // Only allow this if action is not on player, guest,
                    // puppet different from source.  Those are never
                    // considered local because it would be confusing if you
                    // could use another player's actions, even if you have
                    // permission on them.
                    //
                    const dbtype::EntityType type =
                        action_container->get_entity_type();
                    const bool action_on_player =
                        (type == dbtype::ENTITYTYPE_player) or
                        (type == dbtype::ENTITYTYPE_guest) or
                        (type == dbtype::ENTITYTYPE_puppet);

                    if ((not action_on_player) or (source == action_container))
                    {
                        target_cpe =
                            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                                action_container.get());
                    }
                }
            }
        }

        if (source_cpe and target_cpe)
        {
            // Only ContainerPropertyEntities have the concept of being
            // contained.  Everything else cannot be checked for locality.
            //
            if (target_cpe->get_entity_id() == source_cpe->get_entity_id())
            {
                // Likely an action attached to the source; it's local.
                result = true;
            }
            else if (include_source_inventory and
              (target_cpe->get_contained_by() == source_cpe->get_entity_id()))
            {
                // The entity is in the inventory; it's local.
                result = true;
            }
            else if (source_cpe->get_contained_by() ==
                     target_cpe->get_contained_by())
            {
                // The Entity is in the same room as us; it's local.
                result = true;
            }
            else if (source_cpe->get_contained_by() ==
                    target_cpe->get_entity_id())
            {
                // An action attached to the room we're in or the room itself.
                result = true;
            }
            else
            {
                // Do a check to see if target is in a Region above us.
                // Since only Actions are allowed in Regions (and we modified
                // target to be the container of the action), we only have to
                // check if the target ID is any of the Regions above us.
                //
                // Start it out by going to the room containing the source.
                //
                const dbtype::Id target_id = target_cpe->get_entity_id();
                dbtype::ContainerPropertyEntity *region_ptr = 0;
                dbinterface::EntityRef region_ref =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        source_cpe->get_contained_by());

                while (region_ref.valid())
                {
                    if (region_ref.id() == target_id)
                    {
                        // Found a match.
                        result = true;
                        break;
                    }
                    else
                    {
                        // No match, go up if we can.
                        //
                        region_ptr =
                            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                                region_ref.get());

                        if (region_ptr)
                        {
                            region_ref = dbinterface::DatabaseAccess::instance()->
                                get_entity(region_ptr->get_contained_by());
                            region_ptr = 0;
                        }
                        else
                        {
                            // At top.
                            region_ref.clear();
                            break;
                        }
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_entity_local_via_inventory(
        dbinterface::EntityRef &source,
        dbinterface::EntityRef &target,
        const bool include_source_inventory)
    {
        bool result = false;

        if ((not source.valid()) or (not target.valid()))
        {
            return result;
        }

        dbtype::ContainerPropertyEntity * const source_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                source.get());
        dbtype::ContainerPropertyEntity * target_cpe =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                target.get());

        if (source_cpe and target_cpe)
        {
            // See if target is contained by some Entity that is contained
            // by what the source is in.  In other words:
            //   target's container's container == source's container.
            //
            dbinterface::EntityRef target_container_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(
                    target_cpe->get_contained_by());

            if (target_container_ref.valid())
            {
                dbtype::ContainerPropertyEntity * const target_container_cpe =
                    dynamic_cast<dbtype::ContainerPropertyEntity *>(
                        target_container_ref.get());

                if (target_container_cpe)
                {
                    result = target_container_cpe->get_contained_by() ==
                        source_cpe->get_contained_by();
                }
            }
        }

        if (not result)
        {
            result = is_entity_local(source, target, include_source_inventory);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission(
        security::Result &result,
        const dbtype::SecurityFlag flag,
        const BasicFlagHandling include_basic,
        const bool running_as_requester,
        dbinterface::EntityRef &target,
        dbinterface::EntityRef &requester,
        const dbtype::Id &program_id,
        dbinterface::EntityRef &program)
    {
        bool has_determined = false;

        if (not target.valid())
        {
            return has_determined;
        }

        // Determine if we need to check permissions for the program only
        // (not running as requester), or the program and requester (if running
        // as requester).
        //
        if (running_as_requester)
        {
            if (requester.valid())
            {
                // Use requester and program permissions.
                //
                if (program_id.is_default())
                {
                    // Check requester only (native program, etc)
                    //
                    result = (has_permission(
                        target->get_entity_owner(),
                        target->get_entity_security(),
                        requester,
                        flag,
                        include_basic) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
                else
                {
                    // Check requester and program.
                    //
                    if (program.valid())
                    {
                        result = (has_permission(
                            target->get_entity_owner(),
                            target->get_entity_security(),
                            requester,
                            program,
                            flag,
                            include_basic) ? RESULT_ACCEPT : RESULT_DENY);
                        has_determined = true;
                    }
                }
            }
        }
        else
        {
            // Use program permissions only.
            //
            if (program_id.is_default())
            {
                // Native program, use requester's permissions.
                result = (has_permission(
                    target->get_entity_owner(),
                    target->get_entity_security(),
                    requester,
                    flag,
                    include_basic) ? RESULT_ACCEPT : RESULT_DENY);
                has_determined = true;
            }
            else
            {
                if (program.valid())
                {
                    result = (has_permission(
                        target->get_entity_owner(),
                        target->get_entity_security(),
                        program,
                        flag,
                        include_basic) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
            }
        }

        return has_determined;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission(
        Result &result,
        const dbtype::SecurityFlag flag,
        const bool running_as_requester,
        dbinterface::EntityRef &target,
        const std::string &target_application,
        dbinterface::EntityRef &requester,
        const dbtype::Id &program_id,
        dbinterface::EntityRef &program)
    {
        bool has_determined = false;

        if (not target.valid())
        {
            return has_determined;
        }

        // Retrieve the application security and owner, if supported.
        //
        dbtype::PropertyEntity * const target_pe =
            dynamic_cast<dbtype::PropertyEntity *>(target.get());
        dbtype::PropertyEntity::ApplicationOwnerSecurity
            application_security =
                std::make_pair(dbtype::Id(), dbtype::PropertySecurity());

        if (not target_pe)
        {
            // Not able to contain properties at all.
            return has_determined;
        }
        else
        {
            application_security = target_pe->get_application_security_settings(
                target_application);
        }

        if (application_security.first.is_default())
        {
            // Not a valid application name.
            return has_determined;
        }

        // Determine if we need to check permissions for the program only
        // (not running as requester), or the program and requester (if running
        // as requester).
        //
        if (running_as_requester)
        {
            if (requester.valid())
            {
                // Use requester and program permissions.
                //
                if (program_id.is_default())
                {
                    // Check requester only (native program, etc)
                    //
                    result = (has_permission(
                        application_security.first,
                        application_security.second,
                        requester,
                        flag) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
                else
                {
                    // Check requester and program.
                    //
                    if (program.valid())
                    {
                        result = (has_permission(
                            application_security.first,
                            application_security.second,
                            requester,
                            program,
                            flag) ? RESULT_ACCEPT : RESULT_DENY);
                        has_determined = true;
                    }
                }
            }
        }
        else
        {
            // Use program permissions only.
            //
            if (program_id.is_default())
            {
                // Native program, use requester's permissions
                result = (has_permission(
                    application_security.first,
                    application_security.second,
                    requester,
                    flag) ? RESULT_ACCEPT : RESULT_DENY);
                has_determined = true;
            }
            else
            {
                if (program.valid())
                {
                    result = (has_permission(
                        application_security.first,
                        application_security.second,
                        program,
                        flag) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
            }
        }

        return has_determined;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity,
        const dbtype::SecurityFlag flag,
        const BasicFlagHandling include_basic)
    {
        if (not entity.valid())
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = owner_id == entity.id();

        // Check 'other' second, as that's easiest.
        //
        if (not permission)
        {
            permission = security.get_other_security_flag(flag) or
                ((include_basic == BHANDLING_INCLUDE_BASIC)  and
                security.get_other_security_flag(dbtype::SECURITYFLAG_basic));
        }

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id());
        }

        if ((not permission) and (security.get_list_security_flag(flag) or
            ((include_basic > BHANDLING_EXCLUDE_BASIC) and
             security.get_list_security_flag(dbtype::SECURITYFLAG_basic))))
        {
            // Check 'list', but only if it can even do the flag
            permission = is_in_list(security.get_list_ids(), entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity,
        dbinterface::EntityRef &other_entity,
        const dbtype::SecurityFlag flag,
        const BasicFlagHandling include_basic)
    {
        if ((not entity.valid()) or (not other_entity.valid()))
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = (owner_id == entity.id()) or
            (owner_id == other_entity.id());

        // Check 'other' second, as that's easiest.
        //
        if (not permission)
        {
            permission = security.get_other_security_flag(flag) or
                              ((include_basic == BHANDLING_INCLUDE_BASIC) and
                               security.get_other_security_flag(
                                   dbtype::SECURITYFLAG_basic));
        }

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id()) or
                    is_in_list(security.get_admin_ids(), other_entity.id());
        }

        if ((not permission) and (security.get_list_security_flag(flag) or
                                  ((include_basic > BHANDLING_EXCLUDE_BASIC) and
                                   security.get_list_security_flag(
                                       dbtype::SECURITYFLAG_basic))))
        {
            // Check 'list', but only if it can even do the flag
            permission = is_in_list(
                security.get_list_ids(),
                entity.id(),
                other_entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission_explicit(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity,
        const dbtype::SecurityFlag flag,
        const BasicFlagHandling include_basic)
    {
        if (not entity.valid())
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = owner_id == entity.id();

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id());
        }

        if ((not permission) and (security.get_list_security_flag(flag) or
                                  ((include_basic > BHANDLING_EXCLUDE_BASIC) and
                                   security.get_list_security_flag(
                                       dbtype::SECURITYFLAG_basic))))
        {
            // Check 'list', but only if it can even do the flag
            permission = is_in_list(security.get_list_ids(), entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::has_permission_explicit(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity,
        dbinterface::EntityRef &other_entity,
        const dbtype::SecurityFlag flag,
        const BasicFlagHandling include_basic)
    {
        if ((not entity.valid()) or (not other_entity.valid()))
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = (owner_id == entity.id()) or
                          (owner_id == other_entity.id());

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id()) or
                    is_in_list(security.get_admin_ids(), other_entity.id());
        }

        if ((not permission) and (security.get_list_security_flag(flag) or
                                  ((include_basic > BHANDLING_EXCLUDE_BASIC) and
                                   security.get_list_security_flag(
                                       dbtype::SECURITYFLAG_basic))))
        {
            // Check 'list', but only if it can even do the flag
            permission = is_in_list(
                security.get_list_ids(),
                entity.id(),
                other_entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_admin(
        Result &result,
        const bool running_as_requester,
        dbinterface::EntityRef &target,
        dbinterface::EntityRef &requester,
        const dbtype::Id &program_id,
        dbinterface::EntityRef &program)
    {
        bool has_determined = false;

        if (not target.valid())
        {
            return has_determined;
        }

        // Determine if we need to check permissions for the program only
        // (not running as requester), or the program and requester (if running
        // as requester).
        //
        if (running_as_requester)
        {
            if (requester.valid())
            {
                // Use requester and program permissions.
                //
                if (program_id.is_default())
                {
                    // Check requester only (native program, etc)
                    //
                    result = (is_admin(
                        target->get_entity_owner(),
                        target->get_entity_security(),
                        requester) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
                else
                {
                    // Check requester and program.
                    //
                    if (program.valid())
                    {
                        result = (is_admin(
                            target->get_entity_owner(),
                            target->get_entity_security(),
                            requester,
                            program) ? RESULT_ACCEPT : RESULT_DENY);
                        has_determined = true;
                    }
                }
            }
        }
        else
        {
            // Use program permissions only.
            //
            if (program_id.is_default())
            {
                // Native program, use requester's permissions
                result = (is_admin(
                    target.id(),
                    target->get_entity_security(),
                    requester) ? RESULT_ACCEPT : RESULT_DENY);
                has_determined = true;
            }
            else
            {
                if (program.valid())
                {
                    result = (is_admin(
                        target.id(),
                        target->get_entity_security(),
                        program) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
            }
        }

        return has_determined;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_admin(
        Result &result,
        const bool running_as_requester,
        dbinterface::EntityRef &target,
        const std::string &target_application,
        dbinterface::EntityRef &requester,
        const dbtype::Id &program_id,
        dbinterface::EntityRef &program)
    {
        bool has_determined = false;

        if (not target.valid())
        {
            return has_determined;
        }

        // Retrieve the application security and owner, if supported.
        //
        dbtype::PropertyEntity * const target_pe =
            dynamic_cast<dbtype::PropertyEntity *>(target.get());
        dbtype::PropertyEntity::ApplicationOwnerSecurity
            application_security =
            std::make_pair(dbtype::Id(), dbtype::PropertySecurity());

        if (not target_pe)
        {
            // Not able to contain properties at all.
            return has_determined;
        }
        else
        {
            application_security = target_pe->get_application_security_settings(
                target_application);
        }

        if (application_security.first.is_default())
        {
            // Not a valid application name.
            return has_determined;
        }

        // Determine if we need to check permissions for the program only
        // (not running as requester), or the program and requester (if running
        // as requester).
        //
        if (running_as_requester)
        {
            if (requester.valid())
            {
                // Use requester and program permissions.
                //
                if (program_id.is_default())
                {
                    // Check requester only (native program, etc)
                    //
                    result = (is_admin(
                        application_security.first,
                        application_security.second,
                        requester) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
                else
                {
                    // Check requester and program.
                    //
                    if (program.valid())
                    {
                        result = (is_admin(
                            application_security.first,
                            application_security.second,
                            requester,
                            program) ? RESULT_ACCEPT : RESULT_DENY);
                        has_determined = true;
                    }
                }
            }
        }
        else
        {
            // Use program permissions only.
            //
            if (program_id.is_default())
            {
                // Native program, use requester's permissions
                result = (is_admin(
                    application_security.first,
                    application_security.second,
                    requester) ? RESULT_ACCEPT : RESULT_DENY);
                has_determined = true;
            }
            else
            {
                if (program.valid())
                {
                    result = (is_admin(
                        application_security.first,
                        application_security.second,
                        program) ? RESULT_ACCEPT : RESULT_DENY);
                    has_determined = true;
                }
            }
        }

        return has_determined;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_admin(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity)
    {
        if (not entity.valid())
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = owner_id == entity.id();

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_admin(
        const dbtype::Id &owner_id,
        const dbtype::Security &security,
        dbinterface::EntityRef &entity,
        dbinterface::EntityRef &other_entity)
    {
        if ((not entity.valid()) or (not other_entity.valid()))
        {
            return false;
        }

        // Check owner first, as that's easiest.
        //
        bool permission = (owner_id == entity.id()) or
                          (owner_id == other_entity.id());

        if (not permission)
        {
            // Check to see if admin
            permission = is_in_list(security.get_admin_ids(), entity.id()) or
                          is_in_list(security.get_admin_ids(), other_entity.id());
        }

        return permission;
    }

    // ----------------------------------------------------------------------
    bool CheckerHelpers::is_in_list(
        const dbtype::Security::SecurityIds &list_contents,
        const dbtype::Id &id_to_check,
        const dbtype::Id &other_id_to_check)
    {
        bool found = false;

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();

        for (dbtype::Security::SecurityIds::const_iterator list_iter =
                list_contents.begin();
            list_iter != list_contents.end();
            ++list_iter)
        {
            if ((*list_iter == id_to_check) or
                ((not other_id_to_check.is_default()) and
                  (*list_iter == other_id_to_check)))
            {
                // Found it!
                found = true;
                break;
            }
            else if (db_access->get_entity_type(*list_iter) ==
                dbtype::ENTITYTYPE_group)
            {
                // Not our ID, but it is a group, which may contain our
                // ID.  Go check it...
                //
                dbinterface::EntityRef entity_ref = db_access->get_entity(
                    *list_iter);

                if (entity_ref.valid() and
                    (entity_ref.type() == dbtype::ENTITYTYPE_group) and
                    (not entity_ref.is_delete_pending()))
                {
                    dbtype::Group * const group_ptr =
                        static_cast<dbtype::Group *>(entity_ref.get());

                    if (group_ptr->is_in_group(id_to_check) or
                        ((not other_id_to_check.is_default()) and
                          group_ptr->is_in_group(other_id_to_check)))
                    {
                        found = true;
                        break;
                    }
                }
            }
        }

        return found;
    }
}
}
