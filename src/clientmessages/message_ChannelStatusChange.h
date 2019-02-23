/*
 * comm_ChannelStatusChange.h
 */

#ifndef MUTGOS_MESSAGE_CHANNELSTATUSCHANGE_H
#define MUTGOS_MESSAGE_CHANNELSTATUSCHANGE_H

#include <string>

#include "comminterface/comm_CommonTypes.h"
#include "utilities/json_JsonUtilities.h"
#include "osinterface/osinterface_OsTypes.h"

#include "channels/events_Channel.h"

#include "message_ChannelStatus.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Contains status information for a specific channel
     */
    class ChannelStatusChange : public ClientMessage
    {
    public:
        /**
         * Constructs the channel status information.  Most of this data
         * is provided by the channel itself.
         * @param status[in] The current channel status.
         * @param out[in] True if this channel goes out towards the client,
         * false if in from the client.
         * @param id[in] The ID of the channel, as given by the comm subsystem.
         * @param name[in] The name of the channel.
         * @param subtype[in] The subtype of the channel.
         */
        ChannelStatusChange(
            const ChannelStatus status,
            const bool out,
            const comm::ChannelId id,
            const std::string &name,
            const events::Channel::ChannelType type,
            const std::string &subtype);

        /**
         * Copy constructor
         * @param rhs[in] Source to copy from.
         */
        ChannelStatusChange(const ChannelStatusChange &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ChannelStatusChange();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The current channel status.
         */
        ChannelStatus get_channel_status(void) const
          { return channel_status; }

        /**
         * @return True if this channel goes out towards the client,
         * false if in from the client.
         */
        bool get_channel_out(void) const
          { return channel_out; }

        /**
         * @return The ID of the channel, as given by the comm subsystem.
         */
        comm::ChannelId get_channel_id(void) const
          { return channel_id; }

        /**
         * @return The name of the channel.
         */
        const std::string &get_channel_name(void) const
          { return channel_name; }

        /**
         * @return The type of the channel.
         */
        const events::Channel::ChannelType get_channel_type(void) const
          { return channel_type; }

        /**
         * @return The subtype of the channel.
         */
        const std::string &get_channel_subtype(void) const
          { return channel_subtype; }

        /**
         * Saves this channel status to the provided JSON node.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * CURRENTLY DISABLED, because this goes towards the client only,
         * never from the client.
         *
         *
         * Restores this channel status from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return False..
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        ChannelStatus channel_status; ///< Overall channel status
        bool channel_out; ///< True if heading towards client, false for towards server
        comm::ChannelId channel_id; ///< ID number associated with the channel
        std::string channel_name;  ///< The channel name
        events::Channel::ChannelType channel_type; ///< The channel type
        std::string channel_subtype; ///< The channel subtype
    };
}
}

#endif //MUTGOS_MESSAGE_CHANNELSTATUSCHANGE_H
