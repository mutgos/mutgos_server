/*
 * security_DeleteEntityChecker.h
 */

#ifndef MUTGOS_SECURITY_DELETEENTITYCHECKER_H
#define MUTGOS_SECURITY_DELETEENTITYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the security for deleting an Entity.
     * An entity can be deleted if the context is:
     *   The owner of the Entity.
     *   In the Entity's admin list.
     *   A site admin.
     */
    class DeleteEntityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        DeleteEntityChecker(void);

        /**
         * Required destructor.
         */
        virtual ~DeleteEntityChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target);
    };
}
}

#endif //MUTGOS_SECURITY_DELETEENTITYCHECKER_H
