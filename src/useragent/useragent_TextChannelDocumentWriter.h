/*
 * useragent_TextChannelDocumentWriter.h
 */

#ifndef MUTGOS_USERAGENT_TEXTCHANNELDOCUMENTWRITER_H
#define MUTGOS_USERAGENT_TEXTCHANNELDOCUMENTWRITER_H

#include <string>

#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "text/text_ExternalText.h"

#include "channels/events_TextChannelReceiver.h"
#include "channels/events_ChannelControlListener.h"

namespace mutgos
{
namespace useragent
{
    /**
     * A simple TextChannel receiver that writes whatever it gets into
     * a Document.  When the channel is closed, the document is saved
     * to the specified PropertyEntity and path.
     *
     * This class is only designed to be created on the heap, never the
     * stack.  It is also self managing; the class will automatically delete
     * itself when done, or if the channel closes or has any other error.
     *
     * The class assumes security has been properly checked before
     * construction.  It will not check security before saving the output.  The
     * Entityref will be held onto to prevent deletion of the Entity; this
     * might be worth improving in the future.
     *
     * TODO This will need to be enhanced after the demo, but for now serves
     * as a good example of how to use certain features.
     */
    class TextChannelDocumentWriter :
        public events::TextChannelReceiver,
        public events::ChannelControlListener
    {
    public:
        /**
         * Creates a TextChannelDocumentWriter, registers it with the
         * channel, and listens for text data.
         * @param entity[in] The PropertyEntity to save the Document
         * in when complete.  The entity MUST be valid.
         * @param property[in] The property path to save the Document in
         * when complete.  Any existing documents will be overwritten.  The
         * application is assumed to exist.
         * @param text_channel_ptr[in] The TextChannel to get data from.
         * While ownership of the pointer does not pass to this class, the
         * pointer must not be deleted until after this class is itself
         * deleted.  Under normal usage of the channel there will be no
         * problems.
         */
        TextChannelDocumentWriter(
            dbinterface::EntityRef &entity,
            const std::string &property,
            events::TextChannel * const text_channel_ptr);

        /**
         * Required virtual destructor.
         */
        virtual ~TextChannelDocumentWriter();

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
            events::TextChannel *channel_ptr,
            text::ExternalTextLine &text_line);

        /**
         * Called when the channel's flow has been blocked, prohibiting items
         * from being placed on it.  Items can be placed on the channel
         * once it has been unblocked.
         * While it is not required, it is highly recommended (to avoid
         * potential stack overflow) to not send a message on the channel
         * when in this callback, if the channel will have flow turned on
         * and off repeatedly.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_blocked(
            const std::string &channel_name,
            events::Channel * const channel_ptr);

        /**
         * Called when the channel's flow has been opened (unblocked), allowing
         * for one or more items to be placed on it.
         * While it is not required, it is highly recommended (to avoid
         * potential stack overflow) to not send a message on the channel
         * when in this callback, if the channel will have flow turned on
         * and off repeatedly.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_open(
            const std::string &channel_name,
            events::Channel *const channel_ptr);

        /**
         * Called when the channel has been permanently closed (no new
         * items can be sent) and will not be reopened.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_closed(
            const std::string &channel_name,
            events::Channel * const channel_ptr);

        /**
         * Called when the channel instance has been destructed.  The pointer
         * will not be valid after this method returns.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_destructed(
            const std::string &channel_name,
            events::Channel * const channel_ptr);
    private:

        /**
         * If currently registered as a listener to the channel, unregister.
         */
        void unregister(void);

        // No copying
        //
        TextChannelDocumentWriter(const TextChannelDocumentWriter &rhs);
        TextChannelDocumentWriter &operator=(
            const TextChannelDocumentWriter &rhs);

        bool registered_listener; ///< True if we've registered as a listener
        events::TextChannel * const channel_ptr; ///< Pointer to the text channel we are receiving from.

        const dbinterface::EntityRef entity_ref; ///< Entity where document will be stored.
        const std::string property_path; ///< Property path where document will be stored.
        dbtype::DocumentProperty document; ///< The document being populated.
    };
}
}

#endif //MUTGOS_USERAGENT_TEXTCHANNELDOCUMENTWRITER_H
