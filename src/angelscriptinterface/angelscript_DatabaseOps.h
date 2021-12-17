/*
 * angelscript_DatabaseOps.h
 */

#ifndef MUTGOS_ANGELSCRIPT_DATABASEOPS_H
#define MUTGOS_ANGELSCRIPT_DATABASEOPS_H

#include <angelscript.h>

namespace mutgos
{
namespace angelscript
{
    /**
     * Provides static methods to search or otherwise manipulate or access the
     * database at a high level.  For Entity-level operations, see the AEntity
     * class.
     */
    class DatabaseOps
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
         * AEntity *match_name_to_id(
         *    const AString &search_string,
         *    const bool exact_match,
         *    const dbtype::EntityType entity_type,
         *    bool &ambiguous);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return The found Entity, or an invalid Entity if ambiguous or not
         * found.
         * @see primitives::DatabasePrims::match_name_to_id() for documentation.
         */
        static void match_name_to_entity(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * AEntity *match_online_name_to_entity(
         *    const AString &search_string,
         *    const bool exact_match,
         *    const dbtype::EntityType entity_type,
         *    bool &ambiguous);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return The found Entity, or an invalid Entity if ambiguous or not
         * found.
         * @see primitives::DatabasePrims::match_online_name_to_id() for
         * documentation.
         */
        static void match_online_name_to_entity(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * AEntity *convert_string_to_entity(const AString &id_as_string);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return The ID as string converted to an Entity, or an invalid
         * Entity if the string was invalid.
         * @see primitives::DatabasePrims::convert_string_to_id() for
         * documentation.
         */
        static void convert_id_to_entity(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         *
         * Actual method signature:
         * AEntity *create_entity(
         *     const dbtype::EntityType entity_type,
         *     const AString &name)
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @return The newly created Entity if no errors or security
         * violations.  Otherwise, it will throw an exception or return an
         * invalid Entity.
         * @see primitives::DatabasePrims::create_entity() for documentation.
         */
        static void create_entity(asIScriptGeneric *gen_ptr);

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

#endif //MUTGOS_ANGELSCRIPT_DATABASEOPS_H
