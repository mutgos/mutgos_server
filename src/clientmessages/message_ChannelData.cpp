/*
 * message_ChannelData.cpp
 */

#include "logging/log_Logger.h"

#include "comminterface/comm_CommonTypes.h"
#include "clientmessages/message_ClientMessage.h"
#include "utilities/json_JsonUtilities.h"

#include "clientmessages/message_MessageFactory.h"

#include "message_ChannelData.h"

namespace
{
    // Static registration
    const bool CHANNEL_DATA_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_CHANNEL_DATA,
            mutgos::message::ChannelData::make_instance);

    const static std::string CHANNEL_ID_KEY = "channelId";
    const static std::string SERIAL_ID_KEY = "serialId";
    const static std::string MESSAGE_PTR_KEY = "content";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ChannelData::ChannelData(void)
      : ClientMessage(CLIENTMESSAGE_CHANNEL_DATA),
        channel_id(0),
        serial_id(0),
        message_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    ChannelData::ChannelData(
        const comm::ChannelId channel,
        const comm::MessageSerialId serial,
        ClientMessage *client_message_ptr)
      : ClientMessage(CLIENTMESSAGE_CHANNEL_DATA),
        channel_id(channel),
        serial_id(serial),
        message_ptr(client_message_ptr)
    {
        if (not client_message_ptr)
        {
            LOG(warning, "message", "ChannelData",
                "client_message_ptr is null!");
        }
    }

    // ----------------------------------------------------------------------
    ChannelData::ChannelData(const ChannelData &rhs)
        : ClientMessage(rhs),
          channel_id(rhs.channel_id),
          serial_id(rhs.serial_id),
          message_ptr(0)
    {
        if (rhs.message_ptr)
        {
            message_ptr = rhs.message_ptr->clone();
        }
    }

    // ----------------------------------------------------------------------
    ChannelData::~ChannelData()
    {
        delete message_ptr;
        message_ptr = 0;
    }

    // ----------------------------------------------------------------------
    ClientMessage* ChannelData::make_instance(void)
    {
        return new ChannelData();
    }

    // ----------------------------------------------------------------------
    ClientMessage* ChannelData::clone(void) const
    {
        return new ChannelData(*this);
    }

    // ----------------------------------------------------------------------
    ClientMessage *ChannelData::transfer_message(void)
    {
        ClientMessage * const temp_ptr = message_ptr;
        message_ptr = 0;
        return temp_ptr;
    }

    // ----------------------------------------------------------------------
    bool ChannelData::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node) and message_ptr;

        success = json::add_static_key_value(
            CHANNEL_ID_KEY,
            channel_id,
            node,
            root) and success;

        success = json::add_static_key_value(
            SERIAL_ID_KEY,
            serial_id,
            node,
            root) and success;

        // Save the message contents
        //
        if (message_ptr)
        {
            JSON_MAKE_MAP_NODE(contents);

            success = message_ptr->save(root, contents);

            success = json::add_static_key_value(
                MESSAGE_PTR_KEY,
                contents,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ChannelData::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        success = json::get_key_value(
            CHANNEL_ID_KEY,
            node,
            channel_id) and success;

        success = json::get_key_value(
            SERIAL_ID_KEY,
            node,
            serial_id) and success;

        const json::JSONNode *message_node = 0;

        json::get_key_value(MESSAGE_PTR_KEY, node, message_node);

        if (not message_node)
        {
            LOG(error, "message", "restore", "No message contents found!");
            success = false;
        }
        else
        {
            const ClientMessageType message_type =
                ClientMessage::get_message_type(*message_node);

            if (message_type == CLIENTMESSAGE_CHANNEL_DATA)
            {
                LOG(error, "message", "restore",
                    "Message contents is a ChannelData!  Nesting of "
                        "ChannelDatas is not allowed!");
                success = false;
            }
            else
            {
                LOG(debug, "message", "restore",
                    "Restoring message of type " +
                    message::client_message_type_to_string(message_type));

                message_ptr =
                    message::MessageFactory::create_message(message_type);

                if (not message_ptr)
                {
                    LOG(error, "message", "restore",
                        "Message type is not registered: " +
                        message::client_message_type_to_string(message_type));

                    success = false;
                }
                else if (not message_ptr->restore(*message_node))
                {
                    LOG(error, "message", "restore",
                        "Failed to restore message of type: " +
                        message::client_message_type_to_string(message_type));

                    delete message_ptr;
                    message_ptr = 0;
                }
            }
        }

        return success;
    }
}
}
