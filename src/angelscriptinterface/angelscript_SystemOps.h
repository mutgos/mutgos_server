/*
 * angelscript_SystemOps.h
 */

#ifndef MUTGOS_ANGELSCRIPT_SYSTEMOPS_H
#define MUTGOS_ANGELSCRIPT_SYSTEMOPS_H

#include <angelscript.h>

namespace mutgos
{
namespace angelscript
{
    /**
     * Provides static methods related to System level operations, such as
     * who is online, running processes, etc.
     */
    class SystemOps
    {
    public:
        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * AEntity *get_me(void);
         * @param engine[in] The script engine to register with.
         * @return The requester as an AEntity.
         */
        static void get_me(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * AString *get_formatted_processes(void);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return All the currently running processes on the system,
         * formatted as a large, multiline string.
         * @see primitives::SystemPrims::get_formatted_processes() for
         * documentation.
         */
        static void get_formatted_processes(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * CScriptArray *get_online_players(void);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return An array of OnlineStatEntry instances, one for each online
         * player for the context's site.
         * @see primitives::SystemPrims::get_online_players() for documentation.
         */
        static void get_online_players(asIScriptGeneric *gen_ptr);

    private:
        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_SYSTEMOPS_H
