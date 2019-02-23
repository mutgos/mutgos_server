/*
 * security_SendTextEntityChecker.h
 */

#ifndef MUTGOS_SECURITY_SENDTEXTENTITYCHECKER_H
#define MUTGOS_SECURITY_SENDTEXTENTITYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks security for CAPABILITY_SEND_TEXT_ENTITY.
     */
    class SendTextEntityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        SendTextEntityChecker(void);

        /**
         * Required destructor.
         */
        virtual ~SendTextEntityChecker();

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

#endif //MUTGOS_SECURITY_SENDTEXTENTITYCHECKER_H
