/*
 * message_ClientMessageType.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTMESSAGETYPE_H
#define MUTGOS_MESSAGE_CLIENTMESSAGETYPE_H

#include <string>

namespace mutgos
{
namespace message
{
    // Must be kept in sync with CPP
    /**
     * Indicates what type of client message is being sent.
     *
     */
    enum ClientMessageType
    {
        /** ClientDataAcknowledge class */
        CLIENTMESSAGE_DATA_ACKNOWLEDGE = 0,
        /** ClientDataAcknowledgeReconnect class */
        CLIENTMESSAGE_DATA_ACKNOWLEDGE_RECONNECT,
        /** ChannelStatusChange class */
        CLIENTMESSAGE_CHANNEL_STATUS_CHANGE,
        /** ClientRequestSiteList class */
        CLIENTMESSAGE_REQUEST_SITE_LIST,
        /** ClientSiteList class */
        CLIENTMESSAGE_SITE_LIST,
        /** ClientAuthenticateRequest class */
        CLIENTMESSAGE_AUTHENTICATION_REQUEST,
        /** ClientAuthenticationResult class */
        CLIENTMESSAGE_AUTHENTICATION_RESULT,
        /** ClientDisconnect class */
        CLIENTMESSAGE_DISCONNECT,
        /** ChannelData class */
        CLIENTMESSAGE_CHANNEL_DATA,
        /** ClientTextData class */
        CLIENTMESSAGE_TEXT_DATA,
        /** ClientExecuteEntity class */
        CLIENTMESSAGE_EXECUTE_ENTITY,
        /** ClientMatchNameRequest class */
        CLIENTMESSAGE_MATCH_NAME_REQUEST,
        /** ClientMatchNameResult class */
        CLIENTMESSAGE_MATCH_NAME_RESULT,
        /** LocationInfoChange class */
        CLIENTMESSAGE_LOCATION_INFO_CHANGE,
        /** Invalid type, do not directly use */
        CLIENTMESSAGE_END_INVALID
    };

    /**
     * Given a ClientMessageType, return it as a string.
     * @param type[in] The type to convert.
     * @return type as a string.
     */
    const std::string &client_message_type_to_string(
        const ClientMessageType type);

    /**
     * Given a string representing a ClientMessageType, return the
     * representative enum.
     * @param str[in] The string to convert.  Must not have excess whitespace
     * and be an exact match.
     * @return The equivalent ClientMessageType enum or invalid.
     */
    ClientMessageType string_to_client_message_type(const std::string &str);
}
}

#endif //MUTGOS_MESSAGE_CLIENTMESSAGETYPE_H
