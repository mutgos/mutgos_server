/*
 * message_ClientChannelRequestClose.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTCHANNELREQUESTCLOSE_H
#define MUTGOS_MESSAGE_CLIENTCHANNELREQUESTCLOSE_H

#include <vector>

#include "comminterface/comm_CommonTypes.h"
#include "utilities/json_JsonUtilities.h"
#include "comminterface/comm_CommonTypes.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Contains a request by the client to close one or more Channels.
     * This is never sent from the server to the client.
     */
    class ClientChannelRequestClose : public ClientMessage
    {
    public:
        typedef std::vector<comm::ChannelId> ChannelIds;

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        ClientChannelRequestClose(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientChannelRequestClose(const ClientChannelRequestClose &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientChannelRequestClose();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The IDs of the channels to close.
         */
        const ChannelIds &get_channels_to_close(void) const
          { return channels_to_close; }

        /**
         * Sets the channels to close.
         * @param channels[in] The IDs of the channels to close.
         */
        const void set_channels_to_close(const ChannelIds &channels)
          { channels_to_close = channels; }

        /**
         * Saves this message to the provided document.
         * This is normally not used, but is available for debugging/testing.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        ChannelIds channels_to_close; ///< Vector of channels the client wants closed
    };
}
}


#endif //MUTGOS_SERVER_CLIENTCHANNELREQUESTCLOSE_H
