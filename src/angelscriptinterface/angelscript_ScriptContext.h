/*
 * angelscript_ScriptContext.h
 */

#ifndef MUTGOS_ANGELSCRIPT_SCRIPTCONTEXT_H
#define MUTGOS_ANGELSCRIPT_SCRIPTCONTEXT_H

#include <string>
#include <exception>

#include "dbtypes/dbtype_Id.h"

#include "security/security_Context.h"

#include "channels/events_TextChannel.h"

namespace mutgos
{
namespace angelscript
{
    /**
     * This maintains important state data about a script that's currently
     * running, such as the security context, how many lines have been
     * executed, other security settings, I/O, etc.
     *
     * While this class can be copied as needed, it is intended to be
     * created on the heap and used by one thread at a time.
     * It is not thread safe.
     */
    class ScriptContext
    {
    public:
        /**
         * The types of exceptions that can be thrown in the script context.
         */
        enum ExceptionType
        {
            /** No exception currently raised */
            EXCEPTION_NONE,
            /** Out of virtual heap memory */
            EXCEPTION_MEMORY,
            /** Exception is an unknown type that inherits from std::exception */
            EXCEPTION_STD,
            /** Security violation exception */
            EXCEPTION_SECURITY,
            /** Exception originating within the angelscript namespace */
            EXCEPTION_ANGEL,
            /** An unknown/other exception type */
            EXCEPTION_OTHER
        };

        /**
         * Constructor that sets the security context.
         * @param security_context[in] The security context pointer.  Control
         * of the pointer will pass to this class instance.
         */
        ScriptContext(security::Context * const security_context);

        /**
         * Standard destructor.
         */
        ~ScriptContext();

        /**
         * Sets the outgoing Channel.
         * @param channel_ptr[in] The outgoing/output Channel.  The pointer
         * will NOT be owned or managed by this class.
         */
        void set_output_channel(events::TextChannel * const channel_ptr)
        { output_channel_ptr = channel_ptr; }

        /**
         * @return The output Channel (may be null).
         */
        events::TextChannel *get_output_channel(void) const
        { return output_channel_ptr; }

        /**
         * @return The security context.
         */
        security::Context &get_security_context()
        { return *security_context_ptr; }

        /**
         * @return True if there is an exception set on this context.
         */
        bool has_exception(void) const
        { return (exception_type != EXCEPTION_NONE); }

        /**
         * Sets the exception type manually.
         * @param type[in] The type of exception.
         */
        void set_exception_type(const ExceptionType type)
        { exception_type = type; }

        /**
         * @return The type of exception thrown.
         */
        ExceptionType get_exception_type(void) const
        { return exception_type; }

        /**
         * Sets the exception reason manually.
         * @param reason[in] The exception reason.
         */
        void set_exception_reason(const std::string reason)
        { exception_what = reason; }

        const std::string &get_exception_reason(void) const
        { return exception_what; }

        /**
         * Sets exception type and reason based on the provided exception.
         * @param exception[in] The exception to
         */
        void set_exception_info(const std::exception &exception);

        /**
         * Clears out the exception info in this context.
         */
        void clear_exception(void);

    private:
        security::Context * const security_context_ptr; ///< The security context

        events::TextChannel *output_channel_ptr; ///< Output Channel

        ExceptionType exception_type; ///< Type of exception, or none if none raised
        std::string exception_what; ///< Holds the reason for the script-stopping exception
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_SCRIPTCONTEXT_H
