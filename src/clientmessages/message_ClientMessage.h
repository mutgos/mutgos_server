/*
 * message_ClientMessage.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTMESSAGE_H
#define MUTGOS_MESSAGE_CLIENTMESSAGE_H

#include "utilities/json_JsonUtilities.h"

#include "osinterface/osinterface_OsTypes.h"
#include "clientmessages/message_ClientMessageType.h"

namespace mutgos
{
namespace message
{
    /**
     * Base class for all messages to/from enhanced clients.
     */
    class ClientMessage
    {
    public:
        /**
         * Used to peek at the message type for deserialization purposes.
         * @param json[in] The JSON to examine.
         * @return The message type, or invalid if JSON is not a message or
         * some other error.
         */
        static ClientMessageType get_message_type(
            const json::JSONNode &json);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientMessage();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const =0;

        /**
         * Saves this message to the provided JSON node.
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

        /**
         * @return The message type.
         */
        ClientMessageType get_message_type(void) const
          { return message_type; }

        /**
         * Sets the message request ID.  If unset, or set to 0, neither
         * this nor the response flag will be sent.
         * @param id[in] The ID to set.
         */
        void set_message_request_id(const MG_UnsignedInt id)
          { request_id = id; }

        /**
         * @return The message request ID.
         */
        MG_UnsignedInt get_message_request_id(void) const
          { return request_id; }

        /**
         * Sets the response flag.  When true, it indicates the request ID
         * is associated to a request with the same ID.
         * @param flag[in] The response flag to set.
         */
        void set_message_response_flag(const bool flag)
          { response_flag = flag; }

        /**
         * @return The response flag.
         */
        bool get_message_response_flag(void) const
          { return response_flag; }

    protected:
        /**
         * Constructor used by subclasses.
         * @param type[in] The type of the message.
         */
        ClientMessage(const ClientMessageType type);

        /**
         * Copy constructor used by subclasses.
         * @param rhs[in] The instance to copy from.
         */
        ClientMessage(const ClientMessage &rhs);

    private:
        // No assignment
        ClientMessage &operator=(const ClientMessage &rhs);

        const ClientMessageType message_type; ///< The type of the subclass
        MG_UnsignedInt request_id; ///< Request ID, when a response is needed
        bool response_flag; ///< True if this message is actually a response to a request
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTMESSAGE_H
