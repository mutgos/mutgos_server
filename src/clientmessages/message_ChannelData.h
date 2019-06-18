/*
 * message_ChannelData.h
 */

#ifndef MUTGOS_MESSAGE_CHANNELDATA_H
#define MUTGOS_MESSAGE_CHANNELDATA_H

#include "comminterface/comm_CommonTypes.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * An unusual message in that it is basically a transport for other
     * ClientMessages being sent through a channel.
     *
     * This also contains the message serial number, since all data that must
     * be ACKed has to come through a channel (non-channel control messages
     * aren't ACKed, such as ClientDataAcknowledge messages).
     *
     * Only the minimal methods needed are implemented, but more can be added
     * later.
     *
     * This can come to and from clients.
     */
    class ChannelData : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ChannelData(void);

        /**
         * Constructor that sets all attributes.
         * @param channel[in] The channel ID the message is being
         * sent/received on.
         * @param serial[in] The serial number of the message.
         * @param client_message_ptr[in] The message contents.  Control of
         * the pointer passes to this class.
         */
        ChannelData(
            const comm::ChannelId channel,
            const comm::MessageSerialId serial,
            ClientMessage *client_message_ptr);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ChannelData(const ChannelData &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ChannelData();

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The channel ID the message is being sent on.
         */
        comm::ChannelId get_channel_id(void) const
          { return channel_id; }

        /**
         * @return The serial number of the message.
         */
        comm::MessageSerialId get_serial_id(void) const
          { return serial_id; }

        /**
         * @return True if there is a message to transfer.
         */
        bool has_message(void) const
          { return message_ptr; }

        /**
         * Transfers control of the message payload pointer to the caller.
         * @return The message payload.  Caller controls the pointer.
         */
        ClientMessage *transfer_message(void);

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * Note this will call the message factory to instantiate
         * the message payload.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        comm::ChannelId channel_id; ///< The channel ID the message is being sent on
        comm::MessageSerialId serial_id; ///< The serial number of the message

        ClientMessage *message_ptr; ///< The client message payload.
    };
}
}

#endif //MUTGOS_MESSAGE_CHANNELDATA_H
