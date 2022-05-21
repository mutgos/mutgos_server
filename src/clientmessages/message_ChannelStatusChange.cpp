/*
 * comm_ChannelStatusChange.cpp
 */

#include <string>

#include "utilities/json_JsonUtilities.h"

#include "message_ChannelStatusChange.h"

#include "message_ChannelStatus.h"


namespace
{
    const static std::string CHANNEL_STATUS_KEY = "channelStatus";
    const static std::string CHANNEL_OUT_KEY = "channelOut";
    const static std::string CHANNEL_ID_KEY = "channelId";
    const static std::string CHANNEL_NAME_KEY = "channelName";
    const static std::string CHANNEL_TYPE_KEY = "channelType";
    const static std::string CHANNEL_SUBTYPE_KEY = "channelSubtype";
    const static std::string CHANNEL_ENTITY_ID_KEY = "channelEntityId";

    const static std::string CHANNEL_TYPE_TEXT = "text";
    const static std::string CHANNEL_TYPE_DATA = "data";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ChannelStatusChange::ChannelStatusChange(
        const ChannelStatus status,
        const bool out,
        const comm::ChannelId id,
        const std::string &name,
        const events::Channel::ChannelType type,
        const std::string &subtype,
        const dbtype::Id &entity_id)
        : ClientMessage(CLIENTMESSAGE_CHANNEL_STATUS_CHANGE),
          channel_status(status),
          channel_out(out),
          channel_id(id),
          channel_name(name),
          channel_type(type),
          channel_subtype(subtype),
          channel_entity_id(entity_id)
    {
    }

    // ----------------------------------------------------------------------
    ChannelStatusChange::ChannelStatusChange(const ChannelStatusChange &rhs)
      : ClientMessage(rhs),
        channel_status(rhs.channel_status),
        channel_out(rhs.channel_out),
        channel_id(rhs.channel_id),
        channel_name(rhs.channel_name),
        channel_type(rhs.channel_type),
        channel_subtype(rhs.channel_subtype),
        channel_entity_id(rhs.channel_entity_id)
    {
    }

    // ----------------------------------------------------------------------
    ChannelStatusChange::~ChannelStatusChange()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ChannelStatusChange::clone() const
    {
        return new ChannelStatusChange(*this);
    }

    // ----------------------------------------------------------------------
    bool ChannelStatusChange::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        success = json::add_static_key_static_value(
            CHANNEL_STATUS_KEY,
            channel_status_to_string(channel_status),
            node,
            root) and success;

        success = json::add_static_key_value(
            CHANNEL_OUT_KEY,
            channel_out,
            node,
            root) and success;

        success = json::add_static_key_value(
            CHANNEL_ID_KEY,
            channel_id,
            node,
            root) and success;

        success = json::add_static_key_value(
            CHANNEL_NAME_KEY,
            channel_name,
            node,
            root) and success;

        switch (channel_type)
        {
            case events::Channel::CHANNEL_TYPE_TEXT:
            {
                success = json::add_static_key_static_value(
                    CHANNEL_TYPE_KEY,
                    CHANNEL_TYPE_TEXT,
                    node,
                    root) and success;

                break;
            }

            case events::Channel::CHANNEL_TYPE_CLIENT_DATA:
            {
                success = json::add_static_key_static_value(
                    CHANNEL_TYPE_KEY,
                    CHANNEL_TYPE_DATA,
                    node,
                    root) and success;

                break;
            }

            default:
            {
                success = false;
            }
        }

        if (not channel_subtype.empty())
        {
            success = json::add_static_key_value(
                CHANNEL_SUBTYPE_KEY,
                channel_subtype,
                node,
                root) and success;
        }

        if (not channel_entity_id.is_default())
        {
            JSON_MAKE_MAP_NODE(id_node);
            success = channel_entity_id.save(root, id_node) and success;

            if (success)
            {
                success = json::add_static_key_value(
                    CHANNEL_ENTITY_ID_KEY,
                    id_node,
                    node,
                    root) and success;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ChannelStatusChange::restore(const json::JSONNode &node)
    {
        return false;
    }
}
}
