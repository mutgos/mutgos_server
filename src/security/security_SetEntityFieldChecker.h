/*
 * security_SetEntityFieldChecker.h
 */

#ifndef MUTGOS_SECURITY_SETENTITYFIELDCHECKER_H
#define MUTGOS_SECURITY_SETENTITYFIELDCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks security for setting fields on an entity.  Fields can be set
     * if the requester is the owner, admin, or has write permissions.
     *
     * The only exception are group 'disabled IDs'.  An Entity can add or
     * remove itself from the disabled list only if it is in the group list,
     * regardless of security settings.
     */
    class SetEntityFieldChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        SetEntityFieldChecker(void);

        /**
         * Required virtual destructor.
         */
        virtual ~SetEntityFieldChecker();

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
    };
}
}

#endif //MUTGOS_SECURITY_SETENTITYFIELDCHECKER_H
