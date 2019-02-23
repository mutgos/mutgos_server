/*
 * events_TextChannelReceiver.h
 */

#ifndef MUTGOS_EVENTS_TEXTCHANNELRECEIVER_H
#define MUTGOS_EVENTS_TEXTCHANNELRECEIVER_H

#include <string>

#include "text/text_ExternalText.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class TextChannel;

    /**
     * Classes that want to get a direct callback (instead of a message in the
     * Executor) when they get data on a TextChannel must extend and implement
     * the methods in this class.  This is basically an interface class.
     */
    class TextChannelReceiver
    {
    public:
        /**
         * Interface class constructor.
         */
        TextChannelReceiver(void)
         { }

        /**
         * Interface class required destructor.
         */
        virtual ~TextChannelReceiver()
         { }

        /**
         * Called when a TextChannel has data for the listener.
         * This must be thread safe.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the channel.
         * @param text_line[in,out] The text data for a line of text.  If
         * an element is removed from from the list, pointer control for it
         * has passed to the caller, who is responsible for deleting it.  When
         * the call returns, any items still in text_line will be automatically
         * cleaned up.
         */
        virtual void text_channel_data(
            const std::string &channel_name,
            TextChannel *channel_ptr,
            text::ExternalTextLine &text_line) =0;
    };
}
}

#endif //MUTGOS_EVENTS_TEXTCHANNELRECEIVER_H
