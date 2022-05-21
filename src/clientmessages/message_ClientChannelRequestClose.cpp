/*
 * message_ClientChannelRequestClose.cpp
 */

#include "message_ClientChannelRequestClose.h"
#include "clientmessages/message_ClientMessageType.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_CHANNEL_REQUEST_CLOSE_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_CLOSE_CHANNEL_REQUEST,
            mutgos::message::ClientChannelRequestClose::make_instance);

    const static std::string CHANNELS_TO_CLOSE_KEY = "channelsToClose";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientChannelRequestClose::make_instance(void)
    {
        return new ClientChannelRequestClose();
    }

    // ----------------------------------------------------------------------
    ClientChannelRequestClose::ClientChannelRequestClose(void)
        : ClientMessage(CLIENTMESSAGE_CLOSE_CHANNEL_REQUEST)
    {
    }

    // ----------------------------------------------------------------------
    ClientChannelRequestClose::ClientChannelRequestClose(
        const ClientChannelRequestClose &rhs)
        : ClientMessage(rhs),
          channels_to_close(rhs.channels_to_close)
    {
    }

    // ----------------------------------------------------------------------
    ClientChannelRequestClose::~ClientChannelRequestClose()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientChannelRequestClose::clone() const
    {
        return new ClientChannelRequestClose(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientChannelRequestClose::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        JSON_MAKE_ARRAY_NODE(channel_ids_node);

        for (ChannelIds::const_iterator channel_iter = channels_to_close.begin();
            channel_iter != channels_to_close.end();
            ++channel_iter)
        {
            success = json::array_add_value(
                *channel_iter,
                channel_ids_node,
                root) and success;
        }

        if (success)
        {
            success = json::add_static_key_value(
                CHANNELS_TO_CLOSE_KEY,
                channel_ids_node,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientChannelRequestClose::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore ID (required).
        //
        const json::JSONNode *channel_ids_node = 0;

        success = json::get_key_value(
            CHANNELS_TO_CLOSE_KEY,
            node,
            channel_ids_node) and success;

        if (success)
        {
            ssize_t index = 0;
            comm::ChannelId value = 0;

            while (json::array_get_value(*channel_ids_node, index++, value))
            {
                channels_to_close.push_back(value);
            }
        }

        return success;
    }
}
}
