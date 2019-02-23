/*
 * message_ClientRequestSiteList.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTREQUESTSITELIST_H
#define MUTGOS_MESSAGE_CLIENTREQUESTSITELIST_H

#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    // Used by the client to request a list of available sites to login to.
    // This is sent by the client to the server only.  The server will never
    // send this out.
    //
    class ClientRequestSiteList : public ClientMessage
    {
    public:
        /**
         * Standard constructor.
         */
        ClientRequestSiteList(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientRequestSiteList(const ClientRequestSiteList &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientRequestSiteList();

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

#endif //MUTGOS_MESSAGE_CLIENTREQUESTSITELIST_H
