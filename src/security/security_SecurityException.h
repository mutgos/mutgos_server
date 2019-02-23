/*
 * security_SecurityException.h
 */

#ifndef MUTGOS_SECURITY_SECURITYEXCEPTION_H
#define MUTGOS_SECURITY_SECURITYEXCEPTION_H

#include <exception>

#include "security_OperationsCapabilities.h"
#include "security_Context.h"

#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace security
{
    class SecurityException : public std::exception
    {
    public:
        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         */
        SecurityException(
            const Operation operation,
            Context &context);

        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         * @param entity_type[in] The type of entity being checked.
         */
        SecurityException(
            const Operation operation,
            Context &context,
            const dbtype::EntityType entity_type);

        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         */
        SecurityException(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target);

        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param application[in] The application or property on the
         * entity_target being checked.
         */
        SecurityException(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const std::string &application);

        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param entity_field[in] The field on the entity_target being
         * checked.
         * @return The result of the security check.
         */
        SecurityException(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const dbtype::EntityField entity_field);

        /**
         * Constructs security exception, creating an error message based on
         * the inputs.
         * @param operation[in] The operation being checked.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The destination or target Entity being
         * checked.
         * @param entity_source[in] The source of the entity_target, or the
         * source Entity being moved to entity_target.
         * @return The result of the security check.
         */
        SecurityException(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            dbinterface::EntityRef &entity_source);

        /**
         * Required destructor.
         */
        virtual ~SecurityException();

        /**
         * @return The reason for the security exception.
         */
        virtual const char* what() const throw()
        { return message.c_str(); }

        /**
         * @return The reason for the security exception.
         */
        virtual const std::string &get_error(void) const
        { return message; }

    private:
        std::string message; ///< The exception message.
    };
}
}

#endif //MUTGOS_SECURITY_SECURITYEXCEPTION_H
