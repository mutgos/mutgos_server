/*
 * events_ChannelClientDataMessage.h
 */

#ifndef MUTGOS_EVENTS_CHANNELCLIENTDATAMESSAGE_H
#define MUTGOS_EVENTS_CHANNELCLIENTDATAMESSAGE_H

#include "channels/events_ChannelMessage.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents a client data item sent from a ClientDataChannel.
     */
    class ChannelClientDataMessage : public ChannelMessage
    {
    public:
        /**
         * Creates a client data message.
         * @param message_ptr[in] The client data item to send.  Ownership of
         * the pointer is transferred to this class.
         */
        ChannelClientDataMessage(const message::ClientMessage *message_ptr)
          : ChannelMessage(
            executor::ProcessMessage::MESSAGE_CLIENT_DATA_CHANNEL),
            client_message_ptr(message_ptr)
          { }

        /**
         * Required virtual destructor, and performs cleanup.
         */
        virtual ~ChannelClientDataMessage()
          { delete client_message_ptr; }

        /**
         * @return The client message.
         */
        const message::ClientMessage &get_item(void) const
          { return *client_message_ptr; }

        /**
         * Transfers control of the client message pointer to the caller.
         * @return The pointer to the client message.  The caller owns
         * the pointer.
         */
        const message::ClientMessage *transfer(void)
        {
            const message::ClientMessage * const temp_ptr = client_message_ptr;
            client_message_ptr = 0;
            return temp_ptr;
        }

    private:
        const message::ClientMessage *client_message_ptr; ///< The client data message to send to a process.
    };
}
}

#endif //MUTGOS_EVENTS_CHANNELCLIENTDATAMESSAGE_H
