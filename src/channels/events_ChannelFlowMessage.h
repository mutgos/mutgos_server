/*
 * events_ChannelFlowMessage.h
 */

#ifndef MUTGOS_EVENTS_CHANNELFLOWMESSAGE_H
#define MUTGOS_EVENTS_CHANNELFLOWMESSAGE_H

#include <string>

#include "events_ChannelMessage.h"

namespace mutgos
{
namespace events
{
    /**
     * A message representing a change in the channel flow status.  It can
     * be sent to both the sender and receiver on a channel, depending on the
     * status.
     */
    class ChannelFlowMessage : public ChannelMessage
    {
    public:
        enum ChannelFlowStatus
        {
            /** Channel is ready to accept new items */
            CHANNEL_FLOW_OPEN,
            /** Channel is unable to accept new items for now */
            CHANNEL_FLOW_BLOCKED,
            /** Channel is permanently closed and will never accept new items */
            CHANNEL_FLOW_CLOSED,
            /** Channel has been destructed.  The pointer is invalid. */
            CHANNEL_DESTRUCTED
        };

        /**
         * Creates a channel status message.
         * @param status[in] The channel status.
         */
        ChannelFlowMessage(const ChannelFlowStatus status)
          : ChannelMessage(
                executor::ProcessMessage::MESSAGE_CHANNEL_FLOW),
            channel_status(status)
          { }

        /**
         * Required virtual destructor.
         */
        virtual ~ChannelFlowMessage()
          { }

        /**
         * @return The channel status.
         */
        const ChannelFlowStatus get_channel_status(void) const
          { return channel_status; }

    private:
        const ChannelFlowStatus channel_status; ///< The channel status.
    };
}
}

#endif //MUTGOS_EVENTS_CHANNELFLOWMESSAGE_H
