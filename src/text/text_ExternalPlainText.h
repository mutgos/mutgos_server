#ifndef MUTGOS_TEXT_EXTERNALPLAINTEXT_H
#define MUTGOS_TEXT_EXTERNALPLAINTEXT_H

#include <string>

#include "text/text_ExternalText.h"

namespace mutgos
{
namespace text
{
    /**
     * Represents unformatted text.  In other words, no special
     * formatting; default colors and styles should be used.
     */
    class ExternalPlainText : public ExternalText
    {
    public:
        /**
         * Creates an external plain text, suitable for deserialization.
         */
        ExternalPlainText(void)
            : ExternalText(ExternalText::TEXT_TYPE_PLAIN_TEXT)
        { }

        /**
         * Creates an external plain text.
         * @param text[in] The text.
         */
        ExternalPlainText(const std::string &text)
            : ExternalText(ExternalText::TEXT_TYPE_PLAIN_TEXT),
              plain_text(text)
        { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ExternalPlainText(const ExternalPlainText &rhs)
            : ExternalText(rhs),
              plain_text(rhs.plain_text)
          { }

        /**
         * Destructor.
         */
        virtual ~ExternalPlainText()
          { }

        /**
         * Creates a copy of this ExternalText.
         * @return A cloned copy.  Caller must manage the pointer.
         */
        virtual ExternalText *clone(void) const
        {
            return new ExternalPlainText(*this);
        }

        /**
         * Appends the given text.
         * @param text[in] The text to append.
         */
        virtual void append_text(const std::string &text)
         { plain_text.append(text); }

        /**
         * Appends a substring from text.
         * @param text[in] The text whose substring is to be appended.
         * @param index[in] The starting position within text to import.
         * @param length[in] The length of the substring to import.
         */
        virtual void append_text(
            const std::string &text,
            const std::string::size_type index,
            const std::string::size_type length)
         { plain_text.append(text, index, length); }

        /**
         * Clears the text content.
         */
        virtual void clear_text(void)
          { plain_text.clear(); }

        /**
         * @return True if the unformatted text is empty.
         */
        virtual bool is_text_empty(void) const
          { return plain_text.empty(); }

        /**
         * @return The plain, unformatted text.
         */
        const std::string &get_text(void) const
          { return plain_text; }

       /**
        * @return The plain, unformatted text so it can be modified.
        */
        std::string &get_text_update(void)
          { return plain_text; }

        /**
         * @return The ExternalText component as a plain string, stripping
         * all metadata or other formatting.
         */
        virtual std::string to_string(void) const
          { return get_text(); }

        /**
         * Saves this to the provided JSON node.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    protected:
        /**
         * Creates an external plain text, constructor used by subclasses.
         * @param type[in] The text type.
         * @param text[in] The text.
         */
        ExternalPlainText(
            const ExternalText::TextType type,
            const std::string &text)
            : ExternalText(type),
              plain_text(text)
        { }

        /**
         * Creates an external plain text, constructor used by subclasses for
         * deserialization.
         * @param type[in] The text type.
         */
        ExternalPlainText(
            const ExternalText::TextType type)
            : ExternalText(type)
        { }

        std::string plain_text; ///< The plain, unformatted text
    };
}
}

#endif //MUTGOS_TEXT_EXTERNALPLAINTEXT_H
