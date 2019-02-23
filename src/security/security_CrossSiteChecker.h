/*
 * security_CrossSiteChecker.h
 */

#ifndef MUTGOS_SECURITY_CROSSSITECHECKER_H
#define MUTGOS_SECURITY_CROSSSITECHECKER_H

#include "dbtypes/dbtype_Id.h"

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * This security checker is inserted in front of the more specific security
     * checkers.  Its purpose is to disallow sites from accessing each other's
     * stuff, except the root/shared site (1).
     */
    class CrossSiteChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        CrossSiteChecker();

        /**
         * Required virtual destructor.
         */
        virtual ~CrossSiteChecker();

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
         * Performs a check to confirm the requester can access the given
         * target, based on site membership.
         * @param requester_id[in] The requester's ID.
         * @param target_id[in] The entity to be acted upon.
         * @return A Result to indicate if the requested is accepted or
         * always denied.
         */
        Result check_site(
            const dbtype::Id &requester_id,
            const dbtype::Id &target_id);
    };
}
}

#endif //MUTGOS_SECURITY_CROSSSITECHECKER_H
