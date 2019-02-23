#ifndef MUTGOS_EVENTS_TEXTCHANNEL_H
#define MUTGOS_EVENTS_TEXTCHANNEL_H

#include <string>

#include "channels/events_Channel.h"
#include "text/text_ExternalText.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class TextChannelReceiver;

    /**
     * A TextChannel is a one way stream of formatted textual data.  It can
     * be used both as output to the communication subsystem (out to the user),
     * or as input from the user (the commands or words they type).
     *
     * It can also be used in other ways, such as with redirected output
     * or input.
     *
     * This is thread safe.
     */
    class TextChannel : public Channel
    {
    public:
        /**
         * Creates a new TextChannel.
         * @param name[in] The name of the TextChannel.  It need not be unique,
         * but should be used to uniquely identify the channel in the context
         * of whatever it is being used for.
         * @param subtype[in] An optional subtype of the TextChannel.  The
         * meaning is specific to how it's being used.
         */
        TextChannel(const std::string &name, const std::string &subtype = "");

        /**
         * Destructor.
         */
        virtual ~TextChannel();

        // TODO Need parameters for item to identify it (pages, etc)

        /**
         * Sends the provided text item on the channel.
         * The control of the pointers for each part of the text item will be
         * transferred to this class.  item will be cleared when this method
         * returns to reflect this.
         * Note that if the item fails to send, pointer ownership will NOT
         * be transferred and item will not be cleared.  This will allow for
         * a resend of data later.
         * @param item[in,out] The item to send.  If successful, it will be
         * returned empty.
         * @return True if successfully sent, or false if not (channel blocked,
         * closed, etc).
         */
        bool send_item(text::ExternalTextLine &item);

        /**
         * Registers the given pointer to receive sent items as callbacks.
         * There can only be one receiver (either a callback or a Process via
         * being added as a resource).
         * @param callback_ptr[in] The callback pointer to register.
         * @return True if successfully registered, false if null or another
         * listener already registered.
         */
        bool register_receiver_callback(TextChannelReceiver *callback_ptr);

        /**
         * Unregisters the given pointer as the receiver and closes the channel
         * if successful.
         * @param callback_ptr[in] The callback pointer to unregister.
         */
        void unregister_receiver_callback(TextChannelReceiver *callback_ptr);

    protected:
        /**
         * @return True if a callback for the listener has been
         * registered.  Used to make sure both a process and callback aren't
         * both registered as a receiver listener.
         */
        virtual bool receiver_callback_registered(void);

    private:
        TextChannelReceiver *recv_callback_ptr; ///< Pointer to receiver, if any
    };
}
}

#endif //MUTGOS_EVENTS_TEXTCHANNEL_H
