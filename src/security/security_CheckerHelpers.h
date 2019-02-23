/*
 * security_CheckerHelpers.h
 */

#ifndef MUTGOS_SECURITY_CHECKERHELPERS_H
#define MUTGOS_SECURITY_CHECKERHELPERS_H

#include <string>

#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_Id.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "security/security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // TODO Will need to modify for puppets.
    //      Puppets owned by player, but should have access to their settings and properties only

    /**
     * Used to control how the 'basic' flag is evaluated.
     * Do not change the ordering without checking the code.
     */
    enum BasicFlagHandling
    {
        /** Only check for the provided flag (which could be 'basic') */
        BHANDLING_EXCLUDE_BASIC = 0,
        /** Check for the 'basic' flag in addition to what's provided as the
            flag to check */
        BHANDLING_INCLUDE_BASIC,
        /** Check for the 'basic' flag in addition to what's provided as the
            flag to check, but do not check for the additionally included basic
            on the 'other' permission */
        BHANDLING_INCLUDE_BASIC_BUT_IGNORE_OTHER
    };

    /**
     * Helper methods that are common to multiple security checkers, due to
     * being common algorithms.
     */
    class CheckerHelpers
    {
    public:
        /**
         * Determines if two non-room Entities are local/nearby to each other.
         * Local means the target has one of these relations to the source:
         *   * In the same room.
         *   * If an action, that the action container is the room, the source,
         *     an Entity in the same room as the source, or (optionally) the
         *     action is an Entity that is in the source's inventory.
         *   * If an action, it cannot be on a player, guest, puppet  different
         *     from yourself.
         *   * (optional) The target is in source's inventory (could be room
         *   contents).
         *   * Further up in a Region (one or more levels).
         * @param source[in] The Entity to check to see if it's local to
         * target.  This should never be an action or a Room.  Target is
         * checked relative to this.
         * @param target[in] The Entity being checked to see if it's
         * local to source - this may be an action or even in another Region.
         * @param include_source_inventory[in] If true, target will be checked
         * for in source's "inventory", and if found is considered local.
         * @return True if source and target are local to each other, or false
         * if not or error (incompatible Entity types, etc).
         */
        static bool is_entity_local(
            dbinterface::EntityRef &source,
            dbinterface::EntityRef &target,
            const bool include_source_inventory);

        /**
         * Calls is_entity_local(), but performs these additional checks if
         * that returns false:
         *   * If the target's container is local to the source, then it is
         *     considered local.  This is for non-action Entities
         *     (container properties) only.
         * @param source[in] The Entity to check to see if it's local to
         * target.  This should never be an action or a Room.  Target is
         * checked relative to this.
         * @param target[in] The Entity being checked to see if it's
         * local to source - this may be an action or even in another Region.
         * @param include_source_inventory[in] If true, target will be checked
         * for in source's "inventory", and if found is considered local.
         * @return True if source and target are local to each other (even if
         * target is in the inventory of an intermediate Entity), or false
         * if not or error (incompatible Entity types, etc).
         */
        static bool is_entity_local_via_inventory(
            dbinterface::EntityRef &source,
            dbinterface::EntityRef &target,
            const bool include_source_inventory);

        /**
         * Determines if requester (or optionally program) has the
         * given permission on target, either explicitly (admin or list) or
         * implicitly (other).
         * Similar to the other has_permission() methods, except it does some
         * logic related to if running as requester.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param result[out] The result of the check.
         * @param flag[in] The security flag to check.
         * @param include_basic[in] See enum documentation for details.
         * @param running_as_requester[in] True if running as requester, false
         * if only running as program.
         * @param target[in] The target to check permissions on.
         * @param requester[in] The requester.
         * @param program_id[in] The ID of the program, or default if native.
         * @param program[in] The program making the request, or invalid if
         * native or none.
         * @return True if successfuly able to determine permissions (result
         * set).  If false, result will be unchanged.
         */
        static bool has_permission(
            Result &result,
            const dbtype::SecurityFlag flag,
            const BasicFlagHandling include_basic,
            const bool running_as_requester,
            dbinterface::EntityRef &target,
            dbinterface::EntityRef &requester,
            const dbtype::Id &program_id,
            dbinterface::EntityRef &program);

        /**
         * Determines if requester (or optionally program) has the
         * given permission on the target's application , either explicitly
         * (admin or list) or implicitly (other).
         * Similar to the other has_permission() methods, except it does some
         * logic related to if running as requester.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param result[out] The result of the check.
         * @param flag[in] The security flag to check.
         * @param running_as_requester[in] True if running as requester, false
         * if only running as program.
         * @param target[in] The target to check permissions on.  Must be
         * able to contain properties or this will return false.
         * @param target_application[in] The target's application to check
         * for the flag.
         * @param requester[in] The requester.
         * @param program_id[in] The ID of the program, or default if native.
         * @param program[in] The program making the request, or invalid if
         * native or none.
         * @return True if successfuly able to determine permissions (result
         * set).  If false, result will be unchanged.  This will return
         * false if target's class cannot have properties or the property
         * is not found.
         */
        static bool has_permission(
            Result &result,
            const dbtype::SecurityFlag flag,
            const bool running_as_requester,
            dbinterface::EntityRef &target,
            const std::string &target_application,
            dbinterface::EntityRef &requester,
            const dbtype::Id &program_id,
            dbinterface::EntityRef &program);

        /**
         * Determines if the given Entity has permission to do what the
         * flag (operation) indicates.  This will check groups referenced
         * by the Security object, but will not recurse beyond the group.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @param flag[in] The security flag (operation) the Entity would like
         * to perform.
         * @param include_basic[in] See enum documentation for details.
         * @return True if the Entity has permission to do the flag (operation),
         * or can do 'basic' if include_basic is true.
         */
        static bool has_permission(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity,
            const dbtype::SecurityFlag flag,
            const BasicFlagHandling include_basic = BHANDLING_EXCLUDE_BASIC);

        /**
         * Determines if the given primary or secondary Entity has permission
         * to do what the flag (operation) indicates.  This will check groups
         * referenced by the Security object, but will not recurse beyond the
         * group.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @param other_entity[in] The secondary Entity to evaluate against
         * the security object.  Must be valid.
         * @param flag[in] The security flag (operation) the Entity would like
         * to perform.
         * @param include_basic[in] See enum documentation for details.
         * @return True if the parimary or secondary Entity has permission to
         * do the flag (operation), or can do 'basic' if include_basic is true.
         */
        static bool has_permission(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity,
            dbinterface::EntityRef &other_entity,
            const dbtype::SecurityFlag flag,
            const BasicFlagHandling include_basic = BHANDLING_EXCLUDE_BASIC);

        /**
         * Determines if the given entity has explicit permission (admin,
         * 'list', owner only) to do what the flag (operation) indicates.  This
         * will check groups referenced by the Security object, but will not
         * recurse beyond the group.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @param flag[in] The security flag (operation) the Entity would like
         * to perform.
         * @param include_basic[in] See enum documentation for details.
         * @return True if the Entity has permission to do the flag (operation),
         * or can do 'basic' if include_basic is true.
         */
        static bool has_permission_explicit(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity,
            const dbtype::SecurityFlag flag,
            const BasicFlagHandling include_basic = BHANDLING_EXCLUDE_BASIC);

        /**
         * Determines if the given primary or secondary Entity has explicit
         * permission (admin, 'list', owner only) to do what the flag
         * (operation) indicates.  This will check groups referenced by the
         * Security object, but will not recurse beyond the group.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @param other_entity[in] The secondary Entity to evaluate against
         * the security object.  Must be valid.
         * @param flag[in] The security flag (operation) the Entity would like
         * to perform.
         * @param include_basic[in] See enum documentation for details.
         * @return True if the Entity has permission to do the flag (operation),
         * or can do 'basic' if include_basic is true.
         */
        static bool has_permission_explicit(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity,
            dbinterface::EntityRef &other_entity,
            const dbtype::SecurityFlag flag,
            const BasicFlagHandling include_basic = BHANDLING_EXCLUDE_BASIC);

        /**
         * Determines if the requester (or program, if applicable) is an admin
         * or owner of the target.
         * This does not check for the admin capability.
         * @param result[out] The result of the check.
         * @param running_as_requester[in] True if running as requester, false
         * if only running as program.
         * @param target[in] The target to check permissions on.
         * @param requester[in] The requester.
         * @param program_id[in] The ID of the program, or default if native.
         * @param program[in] The program making the request, or invalid if
         * native or none.
         * @return True if successfuly able to determine permissions (result
         * set).  If false, result will be unchanged.
         */
        static bool is_admin(
            Result &result,
            const bool running_as_requester,
            dbinterface::EntityRef &target,
            dbinterface::EntityRef &requester,
            const dbtype::Id &program_id,
            dbinterface::EntityRef &program);

        /**
         * Determines if the requester (or program, if applicable) is an admin
         * or owner of the target's propeties for a specific application.
         * @param result[out] The result of the check.
         * @param running_as_requester[in] True if running as requester, false
         * if only running as program.
         * @param target[in] The target to check permissions on.  Must be
         * able to contain properties or this will return false.
         * @param target_application[in] The target's application to check
         * for ownership.
         * @param requester[in] The requester.
         * @param program_id[in] The ID of the program, or default if native.
         * @param program[in] The program making the request, or invalid if
         * native or none.
         * @return True if successfuly able to determine permissions (result
         * set).  If false, result will be unchanged.  This will return
         * false if target's class cannot have properties or the property
         * is not found.
         */
        static bool is_admin(
            Result &result,
            const bool running_as_requester,
            dbinterface::EntityRef &target,
            const std::string &target_application,
            dbinterface::EntityRef &requester,
            const dbtype::Id &program_id,
            dbinterface::EntityRef &program);

        /**
         * Determines if the given entity is an admin, based on the security
         * object and owner provided.  This will check groups referenced by the
         * Security object, but will not recurse beyond the group.
         * This does not check for the admin capability.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @return True if entity is an admin, false if not or error.
         */
        static bool is_admin(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity);

        /**
         * Determines if the given primary or secondary entity is an admin,
         * based on the security object and owner provided.  This will check
         * groups referenced by the Security object, but will not recurse
         * beyond the group.
         * This does not check for the admin capability.
         * This implies other Entities from the database may be retrieved
         * while doing the evaluation.
         * @param owner_id[in] The owner of the security object.
         * @param security[in] The security object to evaluate.
         * @param entity[in] The Entity to evaluate against the security
         * object.
         * @param other_entity[in] The secondary Entity to evaluate against the
         * security object.
         * @return True if primary or secondary entity is an admin, false if
         * not or error.
         */
        static bool is_admin(
            const dbtype::Id &owner_id,
            const dbtype::Security &security,
            dbinterface::EntityRef &entity,
            dbinterface::EntityRef &other_entity);

    private:

        /**
         * Given the contents of a security list, determine if the primary or
         * secondary ID provided is in it.  This will check groups referenced
         * within the list by the Security object, but will not recurse beyond
         * the group.  This implies other Entities from the database may be
         * retrieved while doing the evaluation.
         * @param list_contents[in] The list to check if the given ID
         * is inside it, either directly or indirectly (via another Group).
         * @param id_to_check[in] The ID to check.
         * @param other_id_to_check[in] A secondary ID to check, or default
         * if none. (optional)
         * @return True if the given primary or secondary ID is referenced by
         * the given list.
         */
        static bool is_in_list(
            const dbtype::Security::SecurityIds &list_contents,
            const dbtype::Id &id_to_check,
            const dbtype::Id &other_id_to_check = dbtype::Id());

        // Static class, no constructor should be usable.
        CheckerHelpers(void);
    };
}
}

#endif //MUTGOS_SECURITY_CHECKERHELPERS_H
