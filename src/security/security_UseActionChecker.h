/*
 * security_UseActionChecker.h
 */

#ifndef MUTGOS_SECURITY_USEACTIONCHECKER_H
#define MUTGOS_SECURITY_USEACTIONCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the security for using (executing) an ActionEntity or subclasses.
     *
     * An action can be used if:
     *   The Entity trying to use it has base permissions to it.
     *   The action lock accepts the Entity trying to use it.
     *
     * Unlike other security checkers, it does not matter if a program is
     * initiating the move; only the Entity trying to use the action is
     * considered.
     */
    class UseActionChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        UseActionChecker(void);

        /**
         * Required destructor.
         */
        virtual ~UseActionChecker();

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

#endif //MUTGOS_SECURITY_USEACTIONCHECKER_H
