/*
 * message_ClientTextData.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTTEXTDATA_H
#define MUTGOS_MESSAGE_CLIENTTEXTDATA_H

#include "text/text_ExternalText.h"
#include "message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * A message to and from the client that contains text data.
     */
    class ClientTextData : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientTextData(void);

        /**
         * Constructor that sets all attributes.
         * @param line[in,out] The text line to send in this message.
         * The text data itself will be transferred to this class instance,
         * leaving this parameter empty when construction is complete.
         */
        ClientTextData(text::ExternalTextLine &line);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientTextData(const ClientTextData &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientTextData();

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
         * Sets the text line to send.  Any existing text data will be
         * cleaned up first.
         * @param line[in,out] The text line to send in this message.
         * The text data itself will be transferred to this class instance,
         * leaving this parameter empty when the method returns.
         */
        void set_text_line(text::ExternalTextLine &line);

        /**
         * Used to transfer ownership of the text line pointer to the caller.
         * @return Pointer to the text line data, or null if none.  Caller will
         * own the pointer.
         */
        text::ExternalTextLine *transfer_text_line(void);

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * Note this will call the message factory to instantiate
         * the message payload.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        text::ExternalTextLine *text_line_ptr; ///< The text data
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTTEXTDATA_H
