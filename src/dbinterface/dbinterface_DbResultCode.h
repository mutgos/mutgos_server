#ifndef MUTGOS_DBINTERFACE_DB_RESULT_CODE_H
#define MUTGOS_DBINTERFACE_DB_RESULT_CODE_H

#include <string>

namespace mutgos
{
namespace dbinterface
{
    /**
     * An enum which lists all the return/result codes for operations in
     * DatabaseAccess.
     * @see DatabaseAccess
     */
    enum DbResultCode
    {
        DBRESULTCODE_invalid = 0,
        DBRESULTCODE_OK,   ///< No issues; operation completed successfully.
        DBRESULTCODE_OK_DELAYED, ///< Operation will complete later.
        DBRESULTCODE_ERROR, ///< Generic error
        DBRESULTCODE_DATABASE_ERROR, ///< Database backend had an error
        DBRESULTCODE_ERROR_NOT_FOUND, ///< Item to retrieve was not found
        DBRESULTCODE_ERROR_ENTITY_IN_USE, ///< Entity is still referenced in mem
        DBRESULTCODE_BAD_SITE_ID, ///< The site ID does not exist or invalid.
        DBRESULTCODE_BAD_ENTITY_ID,  ///< The entity ID does not exist or invalid.
        DBRESULTCODE_BAD_ID, ///< The ID does not exist or is invalid.
        DBRESULTCODE_BAD_OWNER,  ///< The owner does not exist or is invalid.
        DBRESULTCODE_BAD_NAME, ///< The name is not valid (empty string, etc).
        DBRESULTCODE_BAD_ENTITY_TYPE, ///< The Entity type is not valid.
        DBRESULTCODE_END
    };

    /**
     * Given a DbResultCode, return it as a string equivalent.
     * @param[in] result The DB result to convert.
     * @return result as a string.
     */
    const std::string &db_result_code_to_string(const DbResultCode result);

    /**
     * Given a DbResultCode, return it as a string that could be shown to a
     * user or translated.
     * @param[in] result The result to convert.
     * @return result as a 'friendly' string suitable for display or translation.
     */
    const std::string &db_result_code_to_friendly_string(
        const DbResultCode result);

    /**
     * Given a string representing a DbResultCode, return the representative
     * enum.
     * @param[in] str The string to convert.  Must not have excess whitespace
     * and be an exact match.
     * @return The equivalent DbResultCode enum or invalid.
     * @see DbResultCode
     */
    DbResultCode string_to_db_result_code(const std::string &str);
}
}

#endif //MUTGOS_DBINTERFACE_RESULT_CODE_H
