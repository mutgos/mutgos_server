/*
 * angelscript_AngelException.h
 */

#ifndef MUTGOS_ANGELSCRIPT_ANGELEXCEPTION_H
#define MUTGOS_ANGELSCRIPT_ANGELEXCEPTION_H

#include <string>
#include <exception>

#include "primitives/primitives_Result.h"

namespace mutgos
{
namespace angelscript
{
    class AngelException : public std::exception
    {
    public:
        /**
         * Constructor that sets an 'unknown reason' type exception.
         */
        AngelException(void);

        /**
         * Constructor that sets the reason for the exception being raised.
         * @param reason[in] The reason for the exception.
         */
        AngelException(const std::string &reason);

        /**
         * Constructor that sets the reason and result code for the exception
         * being raised.
         * @param reason[in] The reason for the exception.
         * @param result[in] The result status to add further detail to
         * the error.
         */
        AngelException(
            const std::string &reason,
            const primitives::Result &result);

        /**
         * Constructor that sets the reason, AngelScript class name, and
         * method name.
         * @param reason[in] The reason for the exception.
         * @param originating_class[in] The AngelScript class name where
         * the exception originated from.
         * @param originating_method[in] The AngelScript method name where
         * the exception originated from.
         */
        AngelException(
            const std::string &reason,
            const std::string &originating_class,
            const std::string &originating_method);

        /**
         * Constructor that sets the reason, result code, AngelScript class
         * name, and method name.
         * @param reason[in] The reason for the exception.
         * @param result[in] The result status to add further detail to
         * the error.
         * @param originating_class[in] The AngelScript class name where
         * the exception originated from.
         * @param originating_method[in] The AngelScript method name where
         * the exception originated from.
         */
        AngelException(
            const std::string &reason,
            const primitives::Result &result,
            const std::string &originating_class,
            const std::string &originating_method);

        /**
         * Required destructor.
         */
        virtual ~AngelException();

        /**
         * @return The reason for the exception.
         */
        virtual const char* what() const throw()
        { return message.c_str(); }

        /**
         * @return The reason for the exception.
         */
        virtual const std::string &get_error(void) const
        { return message; }

    private:
        std::string message; ///< The exception message.
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_ANGELEXCEPTION_H
