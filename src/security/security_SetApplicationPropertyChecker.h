/*
 * security_SetApplicationPropertyChecker.h
 */

#ifndef MUTGOS_SECURITY_SETAPPLICATIONPROPERTYCHECKER_H
#define MUTGOS_SECURITY_SETAPPLICATIONPROPERTYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the ability to set an application property.
     * Only those with write permissions on the application can change
     * application properties.
     */
    class SetApplicationPropertyChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        SetApplicationPropertyChecker(void);

        /**
         * Required destructor.
         */
        virtual ~SetApplicationPropertyChecker();

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


#endif //MUTGOS_SECURITY_SETAPPLICATIONPROPERTYCHECKER_H
