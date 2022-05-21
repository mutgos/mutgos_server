/*
 * message_ClientMessageType.cpp
 */

#include <string>

#include "clientmessages/message_ClientMessageType.h"

namespace
{
    /** Must be kept in sync with header */
    const static std::string CLIENT_MESSAGE_TYPE_AS_STRING[] =
    {
        "DataAcknowledge",
        "DataAcknowledgeReconnect",
        "ChannelStatusChange",
        "RequestSiteList",
        "SiteList",
        "AuthenticateRequest",
        "AuthenticateResult",
        "ChannelRequestClose",
        "Disconnect",
        "ChannelData",
        "TextData",
        "ConnectPuppetRequest",
        "ExecuteEntity",
        "FindEntityRequest",
        "FindEntityResult",
        "LocationInfoChange",
        "INVALID"
    };
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    const std::string &client_message_type_to_string(
        const ClientMessageType type)
    {
        if ((type >= 0) and (type <= CLIENTMESSAGE_END_INVALID))
        {
            return CLIENT_MESSAGE_TYPE_AS_STRING[type];
        }

        return CLIENT_MESSAGE_TYPE_AS_STRING[CLIENTMESSAGE_END_INVALID];
    }

    // ----------------------------------------------------------------------
    ClientMessageType string_to_client_message_type(const std::string &str)
    {
        ClientMessageType type = CLIENTMESSAGE_END_INVALID;

        // Check each string for a match.
        for (int index = 0; index < CLIENTMESSAGE_END_INVALID; ++index)
        {
            if (CLIENT_MESSAGE_TYPE_AS_STRING[index] == str)
            {
                type = (ClientMessageType) index;
                break;
            }
        }

        return type;
    }
}
}
