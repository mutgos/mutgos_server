/*
 * comm_ChannelStatus.h
 */

#ifndef MUTGOS_MESSAGE_CHANNELSTATUS_H
#define MUTGOS_MESSAGE_CHANNELSTATUS_H

#include <string>

namespace mutgos
{
namespace message
{
    /**
     * An enum which has all the different states a Channel can be in.
     */
    enum ChannelStatus
    {
        /** Indicates conversion of string to enum failed */
        CHANNEL_STATUS_invalid,
        /** Channel has been opened */
        CHANNEL_STATUS_open,
        /** Channel has been closed */
        CHANNEL_STATUS_close,
        /** Channel is now blocked */
        CHANNEL_STATUS_block,
        /** Channel is now unblocked */
        CHANNEL_STATUS_unblock,
        /** Always at the end, unused by clients */
        CHANNEL_STATUS_END
    };

    /**
     * Given a ChannelStatus, return it as a string.
     * @param status[in] The status to convert.
     * @return Channel status as a string.
     */
    const std::string &channel_status_to_string(const ChannelStatus status);

    /**
     * Given a string representing a channel status, return the representative
     * enum.
     * @param str[in] The string to convert.  Must not have excess whitespace
     * and be an exact match.
     * @return The equivalent ChannelStatus enum or invalid.
     */
    ChannelStatus string_to_channel_status(const std::string &str);
}
}

#endif //MUTGOS_MESSAGE_CHANNELSTATUS_H
