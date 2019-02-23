/*
 * security_SecurityChecker.h
 */

#ifndef MUTGOS_SECURITY_SECURITYCHECKER_H
#define MUTGOS_SECURITY_SECURITYCHECKER_H

#include <string>

#include "security_OperationsCapabilities.h"
#include "security_Context.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace security
{
    /**
     * An interface class used by the Security subsystem to determine if an
     * operation is allowed based on the parameters.  The various implementers,
     * which are stateless, can accept one or more of the operations and
     * relevant checker methods and provide a determination if an operation
     * is allowed.  Note that most operations only accept a single method from
     * below to check with.  Refer to the Operation enum to determine which
     * method should be used.  The methods are intentionally generic, and most
     * are used for multiple Operations.
     *
     * Note that SecurityCheckers may be chained together and must be
     * thread safe (reentrant).
     *
     * The SecurityCheckers do NOT have to validate the operation makes sense
     * for the inputs presented (unless it is part of determining if
     * it passes security); other code will do that.
     *
     * This class implements all checker methods with a default return of
     * 'skip'; implementing classes only need to implement the applicable
     * method(s) to their operation(s).
     *
     * Do not assume implementors rigorously check the operation being
     * requested; it is assumed they are configured properly to only be called
     * for the operations that they support and that arguments provided are
     * not null (EntityRefs have valid entities, etc) except where allowed.
     *
     */
    class SecurityChecker
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~SecurityChecker();

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

    protected:
        /**
         * Protected constructor so only subclasses can be constructed.
         */
        SecurityChecker(void);
    };
}
}

#endif //MUTGOS_SECURITY_SECURITYCHECKER_H
