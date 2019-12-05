/*
 * angelscript_ScriptUtilities.h
 */

#ifndef MUTGOS_ANGELSCRIPT_SCRIPTUTILITIES_H
#define MUTGOS_ANGELSCRIPT_SCRIPTUTILITIES_H

#include <string>
#include <angelscript.h>

#include "security/security_Context.h"

#include "angelscript_ScriptContext.h"

#include "add_on/scriptarray.h"

// TODO Need to fix and report scriptarray as not having a check for out of mem in Precache(), and memory leaks during construction if max size fail

namespace mutgos
{
namespace angelscript
{
    /**
     * Static methods used for interacting with AngelScript's engine and other
     * common functions.
     */
    class ScriptUtilities
    {
    public:

        /**
         * Used to set the script context on the engine running this script.
         * Only the code that manages AngelScript's execution should call this.
         * @param engine[in,out] The engine running this script.
         * @param context[in] The context to set on the engine.
         */
        static void set_my_script_context(
            asIScriptEngine * const engine,
            ScriptContext *context);

        /**
         * Used to delete the script context on the engine running this
         * script.  Only the code that manages AngelScript's execution should
         * call this.
         * @param engine[in,out] The engine running this script.
         */
        static void cleanup_my_script_context(asIScriptEngine * const engine);

        /**
         * @param engine[in] The engine running this script.
         * @return The current ScriptContext.
         */
        static ScriptContext *get_my_script_context(
            asIScriptEngine * const engine);

        /**
         * @param engine[in] The engine running this script.
         * @return The security context, or null if error.  Do not delete this
         * pointer.
         */
        static security::Context *get_my_security_context(
            asIScriptEngine * const engine);

        /**
         * Sets the exception info on the context based on a provided exception.
         * @param engine[in] The engine running this script.
         * @param exception[in] The exception to read.
         */
        static void set_exception_info(
            asIScriptEngine * const engine,
            const std::exception &exception);

        /**
         * Sets the exception info manually on the context.
         * @param engine[in] The engine running this script.
         * @param type[in] The type of exception.
         * @param reason[in] The reason for the exception.
         */
        static void set_exception_info(
            asIScriptEngine * const engine,
            const ScriptContext::ExceptionType type,
            const std::string &reason);

        /**
         * Sets the exception info manually on the context for an unknown
         * exception.
         * @param engine[in] The engine running this script.
         */
        static void set_exception_info(asIScriptEngine * const engine);

        /**
         * Creates an AngelScript array with the given template type and
         * initial size.
         * @param engine[in] The engine running this script.
         * @param template_type[in] The type of array to create.  This is
         * only the part contained within the angle brackets.
         * @param initial_size[in] How many reserved elements the array
         * initially has.
         * @param exception_on_too_big[in] If initial_size is bigger than the
         * array supports, throw an exception if true, otherwise make it as
         * big as possible and return.
         * @return The created array.  This should never return null, as
         * an exception will be thrown instead.
         * @throws AngelException If there are errors constructing the
         * array (including out of memory).
         */
        static CScriptArray *create_array(
            asIScriptEngine * const engine,
            const std::string &template_type,
            const size_t initial_size,
            const bool exception_on_too_big);

        /**
         * Converts a string with embedded newlines into an array, with
         * each newline indicating a new element.
         * @param engine[in] The engine running this script.
         * @param str[in] The string to convert.
         * @param exception_on_too_big[in] If initial_size is bigger than the
         * array supports, throw an exception if true, otherwise make it as
         * big as possible and return.
         * @return str as an array of strings.
         */
        static CScriptArray *multiline_string_to_array(
            asIScriptEngine * const engine,
            const std::string &str,
            const bool exception_on_too_big);
    private:
        // Static-only class
        ScriptUtilities(void);
        ~ScriptUtilities();
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_SCRIPTUTILITIES_H
