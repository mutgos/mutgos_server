/*
 * message_ClientMessage.cpp
 */

#include <string>
#include <rapidjson/document.h>

#include "utilities/json_JsonUtilities.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_ClientMessageType.h"

namespace
{
    const static std::string MESSAGE_TYPE_KEY = "messageType";
    const static std::string REQUEST_ID_KEY = "requestMessageId";
    const static std::string RESPONSE_FLAG_KEY = "isMessageResponse";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessageType ClientMessage::get_message_type(
        const json::JSONNode &json)
    {
        std::string type_string;

        json::get_key_value(MESSAGE_TYPE_KEY, json, type_string);

        return string_to_client_message_type(type_string);
    }

    // ----------------------------------------------------------------------
    ClientMessage::~ClientMessage()
    {
    }

    // ----------------------------------------------------------------------
    bool ClientMessage::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = true;

        success = json::add_static_key_static_value(
            MESSAGE_TYPE_KEY,
            client_message_type_to_string(message_type),
            node,
            root);

        if (request_id)
        {
            success = json::add_static_key_value(
                REQUEST_ID_KEY,
                request_id,
                node,
                root) and success;

            success = json::add_static_key_value(
                RESPONSE_FLAG_KEY,
                response_flag,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientMessage::restore(const json::JSONNode &node)
    {
        bool success = true;

        // message_type is never restored, since the caller already
        // instantiated the correct message type.

        if (json::get_key_value(REQUEST_ID_KEY, node, request_id))
        {
            // If we have a request ID, then we must have a response flag.
            success =
                json::get_key_value(RESPONSE_FLAG_KEY, node, response_flag);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    ClientMessage::ClientMessage(const ClientMessageType type)
      : message_type(type),
        request_id(0),
        response_flag(false)
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage::ClientMessage(const ClientMessage &rhs)
      : message_type(rhs.message_type),
        request_id(rhs.request_id),
        response_flag(rhs.response_flag)
    {
    }
}
}
