/*
 * security_GetApplicationPropertyChecker.h
 */

#ifndef MUTGOS_SECURITY_GETAPPLICATIONPROPERTYCHECKER_H
#define MUTGOS_SECURITY_GETAPPLICATIONPROPERTYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the ability to retrieve an application property.
     * Only those with read permissions on the application can see
     * application properties.
     */
    class GetApplicationPropertyChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        GetApplicationPropertyChecker(void);

        /**
         * Required destructor.
         */
        virtual ~GetApplicationPropertyChecker();

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

#endif //MUTGOS_SECURITY_GETAPPLICATIONPROPERTYCHECKER_H
