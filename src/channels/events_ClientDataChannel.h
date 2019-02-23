/*
 * events_ClientDataChannel.h
 */

#ifndef MUTGOS_EVENTS_CLIENTDATACHANNEL_H
#define MUTGOS_EVENTS_CLIENTDATACHANNEL_H

#include <string>

#include "channels/events_Channel.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class ClientDataReceiver;

    /**
     * A Client Data Channel is a one way stream of data to or from an
     * 'enhanced' client (such as web).  It can be used both as output to the
     * communication subsystem (out to the user), or as input from the user
     * (commands, UI elements clicked, etc).
     */
    class ClientDataChannel : public Channel
    {
    public:
        /**
         * Creates a new ClientDataChannel.
         * @param name[in] The name of the ClientDataChannel.  It need not be
         * unique, but should be used to uniquely identify the channel in the
         * context of whatever it is being used for.
         * @param subtype[in] An optional subtype of the ClientDataChannel.
         * The meaning is specific to how it's being used.
         */
        ClientDataChannel(
            const std::string &name,
            const std::string &subtype = "");

        /**
         * Destructor.
         */
        virtual ~ClientDataChannel();

        /**
         * Sends the provided client data item on the channel.
         * @param item_ptr[in] The item to send.  Ownership of the pointer
         * will only transfer if true is returned.
         * @return True if successfully sent (pointer ownership transferred),
         * or false if not (channel blocked, closed, etc).
         */
        bool send_item(message::ClientMessage *item_ptr);

        /**
         * Registers the given pointer to receive sent items as callbacks.
         * There can only be one receiver (either a callback or a Process via
         * being added as a resource).
         * @param callback_ptr[in] The callback pointer to register.
         * @return True if successfully registered, false if null or another
         * listener already registered.
         */
        bool register_receiver_callback(ClientDataReceiver *callback_ptr);

        /**
         * Unregisters the given pointer as the receiver and closes the channel
         * if successful.
         * @param callback_ptr[in] The callback pointer to unregister.
         */
        void unregister_receiver_callback(ClientDataReceiver *callback_ptr);

    protected:
        /**
         * @return True if a callback for the listener has been
         * registered.  Used to make sure both a process and callback aren't
         * both registered as a receiver listener.
         */
        virtual bool receiver_callback_registered(void);

    private:
        ClientDataReceiver *recv_callback_ptr; ///< Pointer to receiver, if any
    };
}
}

#endif //MUTGOS_EVENTS_CLIENTDATACHANNEL_H
