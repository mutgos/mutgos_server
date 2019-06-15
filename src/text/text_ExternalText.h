#ifndef MUTGOS_TEXT_EXTERNALTEXT_H
#define MUTGOS_TEXT_EXTERNALTEXT_H

#include <vector>
#include <string>

#include "utilities/json_JsonUtilities.h"

namespace mutgos
{
namespace text
{
    // TODO Maybe add background color?

    class ExternalText; // Needed for typedef
    // TODO This is a memory leak waiting to happen.  Perhaps make it inherit from vector and add auto-cleanup?
    /** A line of formatted text, destined to or from a client.  Use static
        methods in ExternalText to manage. */
    typedef std::vector<ExternalText *> ExternalTextLine;
    /** Multiple ExternalTextLines */
    typedef std::vector<ExternalTextLine> ExternalTextMultiline;

    /**
     * A class that represents formatted textual data either going out to the
     * user or coming from the user.  The data is not just raw text; it may
     * be a URL, color attributes, a DB ID, etc.  It is a client-independent
     * way of representing data the client must format and display.
     *
     * While not all subclasses are pure text, they represent the sort of
     * formatted data that might appear in text (says, poses, descriptions,
     * etc).
     *
     * Most lines of text to and from the user are likely composed of more
     * than one instance of an ExternalText subclass.  Use a ExternalTextLine
     * to represent one complete line.
     */
    class ExternalText
    {
    public:
        /** The type of formatted text contained by this ExternalText.
            Generally, this is used to determine the actual subclass and cast
            to it */
        enum TextType
        {
            /** String is completely unformatted - ExternalPlainText class */
            TEXT_TYPE_PLAIN_TEXT,   // Keep first for bounds check
            /** String has formatting (color, style, etc) -
                ExternalFormattedText class */
            TEXT_TYPE_FORMATTED_TEXT,
            /** String is a URL - ExternalUrlText class */
            TEXT_TYPE_URL,
            /** String is an Entity ID - ExternalIdText class */
            TEXT_TYPE_ID // Keep last for bounds check
        };

        /**
         * Given an ExternalTextLine, delete (clean up memory) all the
         * ExternalText components and clear the container.
         * @param line[in,out] The line of ExternalText data to clear.
         */
        static void clear_text_line(ExternalTextLine &line);

        /**
         * Given multiple ExternalTextLines, delete (clean up memory) all the
         * ExternalText components and clear the container.
         * @param lines[in,out] The line of ExternalText data to clear.
         */
        static void clear_text_lines(ExternalTextMultiline &lines);

        /**
         * Given an ExternalTextLine, return a deep copy of it.  Caller
         * must manage the returned pointers.
         * @param line[in] The line to clone.
         * @return A clone of the line.  Caller must manage the pointers.
         */
        static ExternalTextLine clone_text_line(const ExternalTextLine &line);

        /**
         * Performs to_string() on all components of the line and concatonates
         * them together.
         * @param line[in] The line of ExternalText data to convert to a
         * string.
         * @return The ExternalText data as an unformatted string.
         * @see to_string(void)
         */
        static std::string to_string(const ExternalTextLine &line);

        /**
         * Determines an estimate of how much memory an ExternalTextLine
         * is using.  Extra capacity in the ExternalTextLine is ignored, but
         * the active element pointers are considered.
         * @param line[in] The ExternalTextLine to get the memory usage for.
         * @return The total estimated size that the ExternalTextLine is
         * using.
         */
        static size_t total_mem_used(const ExternalTextLine &line);

        /**
         * Destructor.
         */
        virtual ~ExternalText()
        { }

        /**
         * @return How much memory this ExternalText instance uses.
         */
        virtual size_t mem_used(void) const
        {
            return sizeof(*this);
        }

        /**
         * Creates a copy of this ExternalText.
         * @return A cloned copy.  Caller must manage the pointer.
         */
        virtual ExternalText *clone(void) const =0;

        /**
         * @return The ExternalText component as a plain string, stripping
         * all metadata or other formatting.
         * @see to_string(ExternalTextLine &)
         */
        virtual std::string to_string(void) const =0;

        /**
         * @return The type of the subclass for easy/efficient casting.
         */
        const TextType get_text_type(void) const
          { return text_type; }

        /**
         * Saves this ExternalText (and subclass) to the provided JSON node.
         * Typically used by the save for ExternalTextLine.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this ExternalText (and subclass) from the provided JSON
         * node.
         * Typically used by the restore for ExternalTextLine.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

        /**
         * Saves the line of external text to a JSON node.
         * @param line[in] THe line of external text to save.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        static bool save_line(
            const ExternalTextLine &line,
            json::JSONRoot &root,
            json::JSONNode &node);

        /**
         * Restores a line of external text from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @param line[out] The restored line of external text.  The line will
         * be cleared and cleaned up first, success or fail.
         * @return True if success, false if did not restore correctly.  If
         * false, line will be empty.
         */
        static bool restore_line(
            const json::JSONNode &node,
            ExternalTextLine &line);

    protected:
        /**
         * Copy constructor.
         * @param rhs[in] Source.
         */
        ExternalText(const ExternalText &rhs)
          : text_type(rhs.text_type)
        { }

        /**
         * Constructor used by subclasses to set type.
         * @param type[in] Type of text.
         */
        ExternalText(const TextType type)
            : text_type(type)
        { }

    private:
        const TextType text_type; ///< The type of the subclass
    };
}
}

#endif //MUTGOS_TEXT_EXTERNALTEXT_H
