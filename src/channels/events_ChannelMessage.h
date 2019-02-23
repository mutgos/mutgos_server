/*
 * events_ChannelMessage.h
 */

#ifndef MUTGOS_EVENTS_CHANNELMESSAGE_H
#define MUTGOS_EVENTS_CHANNELMESSAGE_H

#include <string>

#include "executor/executor_ProcessMessage.h"

namespace mutgos
{
namespace events
{
    /**
     * Base abstract class from which all channel messages inherit from.
     *
     * NOTE: The channel pointer is not provided because messages could
     * potentially arrive after the channel has been removed.  To avoid
     * a coredump, the pointer is therefore not provided and must be looked
     * up using the RID and/or name.
     */
    class ChannelMessage : public executor::ProcessMessage
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~ChannelMessage()
          { }

        /**
         * Sets the name of the channel that sent this message.
         * @param name[in] The name of the channel.
         */
        void set_channel_name(const std::string &name)
          { channel_name = name; }

        /**
         * @return The name of the channel that sent this message.
         */
        const std::string &get_channel_name(void) const
          { return channel_name; }

    protected:
        /**
         * Create a ChannelMessage of a specific message.  Only
         * channel-related types should be used.
         * @param type[in] The type of message.
         */
        ChannelMessage(const executor::ProcessMessage::ProcessMessageType type)
            : executor::ProcessMessage(type)
          { }

    private:

        // No copying
        ChannelMessage(const ChannelMessage &rhs);
        ChannelMessage &operator=(const ChannelMessage &rhs);

        std::string channel_name; ///< The name of the channel sending the msg
    };
}
}

#endif //MUTGOS_EVENTS_CHANNELMESSAGE_H
