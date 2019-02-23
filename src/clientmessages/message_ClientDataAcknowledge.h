#ifndef MUTGOS_MESSAGE_DATAACKNOWLEDGE_H
#define MUTGOS_MESSAGE_DATAACKNOWLEDGE_H

#include "comminterface/comm_CommonTypes.h"
#include "message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to indicate receipt of a message(s).
     */
    class ClientDataAcknowledge : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientDataAcknowledge(void);

        /**
         * Constructor that sets all attributes.
         * @param ser_id[in] The message serial ID being ACKed.
         */
        ClientDataAcknowledge(const comm::MessageSerialId ser_id);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientDataAcknowledge(const ClientDataAcknowledge &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientDataAcknowledge();

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
         * Sets the ACKed message serial ID.
         * @param ser_id[in] The message serial ID being ACKed.
         */
        void set_serial_id(const comm::MessageSerialId ser_id)
          { message_ser_id = ser_id; }

        /**
         * @return The message serial ID being ACKed.
         */
        comm::MessageSerialId get_serial_id(void) const
          { return message_ser_id; }

        /**
         * Saves this message to the provided document.
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

    protected:
        /**
         * Constructor that allows a message type to be specified.  Used by
         * subclasses.
         * @param type[in] The type of the message.
         */
        ClientDataAcknowledge(const ClientMessageType type);

        /**
         * Constructor that allows a message type to be specified and all
         * attributes.  Used by subclasses.
         * @param type The type of the message.
         * @param ser_id The message serial ID being ACKed.
         */
        ClientDataAcknowledge(
            const ClientMessageType type,
            const comm::MessageSerialId ser_id);

    private:
        comm::MessageSerialId message_ser_id; ///< The message serial ID being ACKed
    };
}
}

#endif //MUTGOS_MESSAGE_DATAACKNOWLEDGE_H
