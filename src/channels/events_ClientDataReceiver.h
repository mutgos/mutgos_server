/*
 * events_ClientDataReceiver.h
 */

#ifndef MUTGOS_EVENTS_CLIENTDATARECEIVER_H
#define MUTGOS_EVENTS_CLIENTDATARECEIVER_H

#include <string>
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class ClientDataChannel;

    /**
     * Classes that want to get a direct callback (instead of a message in the
     * Executor) when they get data on a ClientDataChannel must extend and
     * implement the methods in this class.  This is basically an interface
     * class.
     */
    class ClientDataReceiver
    {
    public:
        /**
         * Interface class constructor.
         */
        ClientDataReceiver(void)
         { }

        /**
         * Interface class required destructor.
         */
        virtual ~ClientDataReceiver()
         { }

        /**
         * Called when a ClientDataChannel has data for the listener.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the channel.
         * @param client_message_ptr[in] The client data.  Ownership of the
         * pointer is transferred to this method.
         */
        virtual void client_channel_data(
            const std::string &channel_name,
            ClientDataChannel *channel_ptr,
            message::ClientMessage *client_message_ptr) =0;
    };
}
}

#endif //MUTGOS_EVENTS_CLIENTDATARECEIVER_H
