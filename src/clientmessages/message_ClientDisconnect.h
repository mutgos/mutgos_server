/*
 * message_ClientDisconnect.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTDISCONNECT_H
#define MUTGOS_MESSAGE_CLIENTDISCONNECT_H

#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to indicate a proper disconnection is
     * requested.  This can go in either direction.
     */
    class ClientDisconnect : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientDisconnect(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientDisconnect(const ClientDisconnect &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientDisconnect();

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

#endif //MUTGOS_MESSAGE_CLIENTDISCONNECT_H
