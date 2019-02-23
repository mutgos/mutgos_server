/*
 * angelscript_MovementOps.h
 */

#ifndef MUTGOS_ANGELSCRIPT_MOVEMENTOPS_H
#define MUTGOS_ANGELSCRIPT_MOVEMENTOPS_H

#include <angelscript.h>

namespace mutgos
{
namespace angelscript
{
    /**
     * Provides operations that have to do with moving Entities around.
     */
    class MovementOps
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
         * void move_entity(
         *     AEntity *entity_to_move,
         *     AEntity *entity_destination);
         *
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @see primitives::MovementPrims::move_entity() for documentation.
         */
        static void move_entity(asIScriptGeneric *gen_ptr);

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

#endif //MUTGOS_ANGELSCRIPT_MOVEMENTOPS_H
