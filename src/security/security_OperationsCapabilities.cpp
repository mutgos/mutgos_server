/*
 * security_OperationsCapabilities.cpp
 */

#include <string>
#include "security_OperationsCapabilities.h"

namespace
{
    const static std::string OPERATION_AS_STRING[] =
    {
        "GET_FORMATTED_PROCESSES",
        "FIND_BY_NAME_RELATIVE",
        "FIND_CHARACTER_BY_NAME",
        "CHARACTER_ONLINE",
        "GET_CONTAINS",
        "GET_ACTIONS",
        "CREATE_ENTITY",
        "DELETE_ENTITY",
        "GET_ENTITY_FIELD",
        "SET_ENTITY_FIELD",
        "GET_APPLICATION_SECURITY",
        "SET_APPLICATION_SECURITY",
        "GET_APPLICATION_PROPERTY",
        "SET_APPLICATION_PROPERTY",
        "DELETE_APPLICATION_PROPERTY",
        "ENTITY_TOSTRING",
        "TRANSFER_ENTITY",
        "SEND_TEXT_ROOM_UNRESTRICTED",
        "SEND_TEXT_ENTITY",
        "USE_ACTION",
        "invalid"
    };

    const static std::string RESULT_AS_STRING[] =
    {
        "ACCEPT",
        "ACCEPT_ALWAYS",
        "DENY_ALWAYS",
        "DENY",
        "SKIP",
        "invalid"
    };

    const static std::string CAPABILITY_AS_STRING[] =
    {
        "ADMIN",
        "CREATE_PLAYER",
        "BUILDER",
        "SEND_TEXT_ROOM_UNRESTRICTED",
        "SEND_TEXT_ENTITY",
        "FIND_BY_NAME_AFAR",
        "ANY_ID_TO_NAME",
        "CONNECTION_CHECK",
        "RUN_AS_USER",
        "invalid"
    };
}

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    const std::string &operation_to_string(const Operation operation)
    {
        if ((operation >= OPERATION_END_INVALID) or
            (operation < OPERATION_GET_FORMATTED_PROCESSES))
        {
            return OPERATION_AS_STRING[OPERATION_END_INVALID];
        }

        return OPERATION_AS_STRING[operation];
    }

    // -----------------------------------------------------------------------
    const std::string &result_to_string(const Result result)
    {
        if ((result >= RESULT_END_INVALID) or
            (result < RESULT_ACCEPT))
        {
            return RESULT_AS_STRING[RESULT_END_INVALID];
        }

        return RESULT_AS_STRING[result];
    }

    // -----------------------------------------------------------------------
    const std::string &capability_to_string(const Capability capability)
    {
        if ((capability >= CAPABILITY_END_INVALID) or
            (capability < CAPABILITY_ADMIN))
        {
            return CAPABILITY_AS_STRING[CAPABILITY_END_INVALID];
        }

        return CAPABILITY_AS_STRING[capability];
    }
}
}
