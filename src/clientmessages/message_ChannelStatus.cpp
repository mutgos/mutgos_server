/*
 * comm_ChannelStatus.cpp
 */

#include <string>

#include "message_ChannelStatus.h"

namespace
{
    const static std::string CHANNEL_STATUS_AS_STRING[] =
    {
        "INVALID",
        "open",
        "close",
        "block",
        "unblock"
    };
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    const std::string &channel_status_to_string(const ChannelStatus status)
    {
        if ((status >= CHANNEL_STATUS_END) or (status <= CHANNEL_STATUS_invalid))
        {
            return CHANNEL_STATUS_AS_STRING[0];
        }

        return CHANNEL_STATUS_AS_STRING[status];
    }

    ChannelStatus string_to_channel_status(const std::string &str)
    {
        ChannelStatus status = CHANNEL_STATUS_invalid;

        for (int index = 1; index < CHANNEL_STATUS_END; ++index)
        {
            if (CHANNEL_STATUS_AS_STRING[index] == str)
            {
                status = (ChannelStatus) index;
                break;
            }
        }

        return status;
    }
}
}
