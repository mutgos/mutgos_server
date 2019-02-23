/*
 * security_FindCharacterByNameChecker.h
 */

#ifndef MUTGOS_SECURITY_FINDCHARACTERBYNAMECHECKER_H
#define MUTGOS_SECURITY_FINDCHARACTERBYNAMECHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks security for OPERATION_FIND_CHARACTER_BY_NAME.
     */
    class FindCharacterByNameChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        FindCharacterByNameChecker(void);

        /**
         * Required destructor.
         */
        virtual ~FindCharacterByNameChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context);
    };
}
}

#endif //MUTGOS_SECURITY_FINDCHARACTERBYNAMECHECKER_H
