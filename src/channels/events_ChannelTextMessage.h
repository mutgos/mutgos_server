/*
 * events_ChannelTextMessage.h
 */

#ifndef MUTGOS_EVENTS_CHANNELTEXTMESSAGE_H
#define MUTGOS_EVENTS_CHANNELTEXTMESSAGE_H

#include "events_ChannelMessage.h"
#include "text/text_ExternalText.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents a text item sent from a TextChannel.
     */
    class ChannelTextMessage : public ChannelMessage
    {
    public:

        /**
         * Creates a channel text message.
         * @param item[in] The text item.  Ownership of the contents
         * transfers to this class.
         */
        ChannelTextMessage(const text::ExternalTextLine &line)
        : ChannelMessage(
            executor::ProcessMessage::MESSAGE_TEXT_CHANNEL),
          text_line(line)
        { }

        /**
         * Required virtual destructor.
         * This will also clean up the text line to avoid memory leaks.
         */
        virtual ~ChannelTextMessage()
          { text::ExternalText::clear_text_line(text_line); }

        /**
         * If you want to save a pointer from the text line, just erase
         * it from the returned ExternalTextLine, otherwise it will be
         * automatically cleaned up when this class instance is destructed.
         * @return The text item.  It is the actual item, not a copy.
         */
        text::ExternalTextLine &get_item(void)
          { return text_line; }

    private:
        text::ExternalTextLine text_line; ///< The text item being sent.
    };
}
}

#endif //MUTGOS_EVENTS_CHANNELTEXTMESSAGE_H
