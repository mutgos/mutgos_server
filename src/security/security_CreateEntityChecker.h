/*
 * security_CreateEntityChecker.h
 */

#ifndef MUTGOS_SECURITY_CREATEENTITYCHECKER_H
#define MUTGOS_SECURITY_CREATEENTITYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    class CreateEntityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        CreateEntityChecker(void);

        /**
         * Required destructor.
         */
        virtual ~CreateEntityChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_type[in] The type of entity being checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            const dbtype::EntityType entity_type);
    };
}
}

#endif //MUTGOS_SECURITY_CREATEENTITYCHECKER_H
