/*
 * security_CharacterOnlineChecker.h
 */

#ifndef MUTGOS_SECURITY_CHARACTERONLINECHECKER_H
#define MUTGOS_SECURITY_CHARACTERONLINECHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks security for OPERATION_CHARACTER_ONLINE.
     * Works on puppets or players.
     */
    class CharacterOnlineChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        CharacterOnlineChecker(void);

        /**
         * Required virtual destructor.
         */
        virtual ~CharacterOnlineChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context);

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

#endif //MUTGOS_SECURITY_CHARACTERONLINECHECKER_H
