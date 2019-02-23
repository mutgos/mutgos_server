/*
 * message_ClientDataAcknowledgeReconnect.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTDATAACKNOWLEDGERECONNECT_H
#define MUTGOS_MESSAGE_CLIENTDATAACKNOWLEDGERECONNECT_H

#include "clientmessages/message_ClientDataAcknowledge.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to indicate receipt of a message(s) during
     * a reconnect.
     */
    class ClientDataAcknowledgeReconnect : public ClientDataAcknowledge
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientDataAcknowledgeReconnect(void);

        /**
         * Constructor that sets all attributes.
         * @param ser_id[in] The message serial ID being ACKed.
         */
        ClientDataAcknowledgeReconnect(const comm::MessageSerialId ser_id);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientDataAcknowledgeReconnect(
            const ClientDataAcknowledgeReconnect &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientDataAcknowledgeReconnect();

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
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTDATAACKNOWLEDGERECONNECT_H
