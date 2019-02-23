/*
 * security_AdminSecurityChecker.h
 */

#ifndef MUTGOS_SECURITY_ADMINSECURITYCHECKER_H
#define MUTGOS_SECURITY_ADMINSECURITYCHECKER_H

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Simple security checker designed to be put in the front; it will
     * always approve every operation if the user is an admin,
     * otherwise return SKIP.
     */
    class AdminSecurityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        AdminSecurityChecker(void);

        /**
         * Required virtual destructor.
         */
        virtual ~AdminSecurityChecker();

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
         * @param entity_type[in] The type of entity being checked.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            const dbtype::EntityType entity_type);

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

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The destination or target Entity being
         * checked.
         * @param entity_source[in] The source of the entity_target, or the
         * source Entity being moved to entity_target.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            dbinterface::EntityRef &entity_source);

    private:
        /**
         * Always approve every operation if the user is an admin or #*-2,
         * #*-3, otherwise return SKIP.
         * @param context[in] The context the check is made in.
         * @return APPROVE or SKIP based on if the requester is an admin.
         */
        Result admin_check(Context &context);

        /**
         * Do not allow the admin flag to mean player is admin of global
         * site unless they really are.  This will also call admin_check(),
         * so a separate call to that is not nessecary.
         * @param context[in] The context the check is made in.
         * @param entity_ref[in]  The entity to check.
         * @return APPROVE or SKIP based on if the requester is an admin.
         */
        Result cross_site_admin_check(
            Context &context,
            dbinterface::EntityRef &entity_ref);
    };
}
}

#endif //MUTGOS_SECURITY_ADMINSECURITYCHECKER_H
