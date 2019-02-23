
#include <string>

#include "dbinterface_DbResultCode.h"

namespace
{
    const static std::string DB_RESULT_CODE_AS_STRING[] =
    {
        "invalid",
        "OK",
        "OK_DELAYED",
        "ERROR",
        "DATABASE_ERROR",
        "ERROR_NOT_FOUND",
        "ERROR_ENTITY_IN_USE",
        "BAD_SITE_ID",
        "BAD_ENTITY_ID",
        "BAD_ID",
        "BAD_OWNER",
        "BAD_NAME",
        "BAD_ENTITY_TYPE"
    };

    const static std::string DB_RESULT_CODE_AS_FRIENDLY_STRING[] =
    {
        "INVALID",
        "Operation completed successfully",
        "Operation will complete successfully soon",
        "Error",
        "The database backend reported an error",
        "The item requested was not found",
        "The Entity's instance in memory is still being referenced.",
        "The site ID is invalid or does not exist",
        "The Entity ID is invalid or does not exist",
        "The ID is invalid or does not exist",
        "The owner is invalid or does not exist",
        "The name is invalid",
        "The entity type is invalid",
    };
}

namespace mutgos
{
namespace dbinterface
{
    // -----------------------------------------------------------------------
    const std::string &db_result_code_to_string(const DbResultCode result)
    {
        if ((result >= DBRESULTCODE_END) or (result <= DBRESULTCODE_invalid))
        {
            return DB_RESULT_CODE_AS_STRING[0];
        }

        return DB_RESULT_CODE_AS_STRING[result];
    }

    // -----------------------------------------------------------------------
    const std::string &db_result_code_to_friendly_string(
        const DbResultCode result)
    {
        if ((result >= DBRESULTCODE_END) or (result <= DBRESULTCODE_invalid))
        {
            return DB_RESULT_CODE_AS_FRIENDLY_STRING[0];
        }

        return DB_RESULT_CODE_AS_FRIENDLY_STRING[result];
    }

    // -----------------------------------------------------------------------
    DbResultCode string_to_db_result_code(const std::string &str)
    {
        DbResultCode result = DBRESULTCODE_invalid;

        // Check each string for a match.
        for (int index = 1; index < DBRESULTCODE_END; ++index)
        {
            if (DB_RESULT_CODE_AS_STRING[index] == str)
            {
                result = (DbResultCode) index;
                break;
            }
        }

        return result;
    }
}
}