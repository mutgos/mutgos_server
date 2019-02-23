/*
 * security_GetSetApplicationSecurityChecker.h
 */

#ifndef MUTGOS_SECURITY_GETSETAPPLICATIONSECURITYCHECKER_H
#define MUTGOS_SECURITY_GETSETAPPLICATIONSECURITYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the ability to retrieve or set the application security
     * settings.  Only the owner and admins can look at and set security
     * settings details.
     *
     * Getting a list of properties will show a shorthand version of the
     * security settings and is covered under the get_entity_field operation.
     */
    class GetSetApplicationSecurityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        GetSetApplicationSecurityChecker(void);

        /**
         * Required destructor.
         */
        virtual ~GetSetApplicationSecurityChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param application[in] The application or property on the
         * entity_target being checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const std::string &application);
    };
}
}

#endif //MUTGOS_SECURITY_GETSETAPPLICATIONSECURITYCHECKER_H
