/*
 * security_GetEntityFieldChecker.h
 */

#ifndef MUTGOS_SECURITY_ENTITYIDTONAMECHECKER_H
#define MUTGOS_SECURITY_ENTITYIDTONAMECHECKER_H

#include "security_SecurityChecker.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Security.h"

#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks security for getting Entity fields.
     *
     * These are the 'basic' attribute enums:
     *   ENTITYFIELD_type  (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_id    (always allowed)
     *   ENTITYFIELD_name  (special rules)
     *   ENTITYFIELD_owner (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_flags (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_deleted_flag  (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_contained_by (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_action_contained_by (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_action_commands (always allowed if nearby and basic flag set)
     *   ENTITYFIELD_player_display_name (special rules)
     *   ENTITYFIELD_puppet_display_name (special rules)
     *
     * This will also check:
     *   List of contained items / inventory (always allowed if nearby and basic flag set)
     *   List of actions  (always allowed if nearby and basic flag set)
     *   [Temp] If a toString() can be done (owner, admin, read perms only)
     *
     * Anything not listed above requires the 'read' attribute explicitly and
     * is not location dependent.  The 'read' flag overrides 'basic' and would
     * allow accessing 'basic' attributes no matter where Entities are located.
     */
    class GetEntityFieldChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        GetEntityFieldChecker(void);

        /**
         * Required virtual destructor.
         */
        virtual ~GetEntityFieldChecker();

        /**
         * Performs a security check with the given parameters.
         * Used for checking 'get contains' and 'get actions'
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param entity_field[in] The field on the entity_target being
         * checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const dbtype::EntityField entity_field);

    private:
        /**
         * Performs a security check to see if the name can be retrieved.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity whose name we want to get.
         * @return The result of the security check.
         */
        Result name_security_check(
            Context &context,
            dbinterface::EntityRef &entity_target);

        /**
         * Performs a security check to see if the field can be retrieved,
         * first checking to see if the Entities are 'local' to each other and
         * has the 'other' basic flag set, and then checking straight read
         * permissions if the previous checks fail.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity whose field we want to get.
         * @return The result of the security check.
         */
        Result locality_security_check(
            Context &context,
            dbinterface::EntityRef &entity_target);
    };
}
}

#endif //MUTGOS_SECURITY_ENTITYIDTONAMECHECKER_H
