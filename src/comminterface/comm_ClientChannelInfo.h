/*
 * comm_ClientChannelInfo.h
 */

#ifndef MUTGOS_COMM_CLIENTCHANNELINFO_H
#define MUTGOS_COMM_CLIENTCHANNELINFO_H

#include <string>
#include <vector>

#include "channels/events_Channel.h"
#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace comm
{
    /**
     * Container class that has selected data about a channel.
     *
     * Primarily used by certain comm interfaces to get more details on a
     * channel after it's already been declared, typically because the
     * client is unable to keep track of that information between reconnects.
     */
    class ClientChannelInfo
    {
    public:
        /**
         * Constructor that sets everything.
         * @param id[in] The channel ID for this session.
         * @param name[in] The channel name.
         * @param type[in] The channel type.
         * @param subtype[in] The channel subtype (optional).
         * @param entity_id[in] The ID of the Entity associated with the
         * channel (optional).
         * @param outgoing[in] True if channel outputs towards the client,
         * false if inputs from client.
         * @param blocked[in] True if channel is currently blocked.
         */
        ClientChannelInfo(
            const ChannelId id,
            const std::string &name,
            const events::Channel::ChannelType type,
            const std::string &subtype,
            const dbtype::Id &entity_id,
            const bool outgoing,
            const bool blocked)
          : channel_id(id),
            channel_name(name),
            channel_type(type),
            channel_subtype(subtype),
            channel_entity_id(entity_id),
            channel_outgoing(outgoing),
            channel_blocked(blocked)
        { }

        /**
         * Destructor.
         * Nothing to clean up here.
         */
        ~ClientChannelInfo()
        { }

        /**
         * @return The channel ID.
         */
        ChannelId get_channel_id(void) const
        { return channel_id; }

        /**
         * @return The channel name.
         */
        const std::string &get_channel_name(void) const
        { return channel_name; }

        /**
         * @return The channel type.
         */
        const events::Channel::ChannelType get_channel_type(void) const
        { return channel_type; }

        /**
         * @return The channel subtype.
         */
        const std::string &get_channel_subtype(void) const
        { return channel_subtype; }

        /**
         * @return The channel entity ID.
         */
        const dbtype::Id &get_channel_entity_id(void) const
        { return channel_entity_id; }

        /**
         * @return True if channel outputs towards the client, false if inputs
         * from client.
         */
        bool channel_is_outgoing(void) const
        { return channel_outgoing; }

        /**
         * @return True if channel is blocked.
         */
        bool channel_is_blocked(void) const
        { return channel_blocked; }

    private:
        ChannelId channel_id; ///< The channel ID.
        std::string channel_name; ///< The channel name.
        events::Channel::ChannelType channel_type; ///< The channel type.
        std::string channel_subtype; ///< The channel subtype
        dbtype::Id channel_entity_id; // The Entity ID associated with the channel

        bool channel_outgoing; ///< True if channel outputs towards the client, false if inputs from client
        bool channel_blocked; ///< True if channel is currently blocked.
    };


    typedef std::vector<ClientChannelInfo> ClientChannelInfoVector;
}
}

#endif //MUTGOS_COMM_CLIENTCHANNELINFO_H
