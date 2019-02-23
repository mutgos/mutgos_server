/*
 * text_ExternalTextConverter.h
 */

#ifndef MUTGOS_TEXT_EXTERNALTEXTCONVERTER_H
#define MUTGOS_TEXT_EXTERNALTEXTCONVERTER_H

#include <string>

#include "text/text_ExternalText.h"
#include "text/text_ExternalIdText.h"
#include "text/text_ExternalUrlText.h"
#include "text/text_ExternalFormattedText.h"

namespace mutgos
{
namespace text
{
    /**
     * Utility class that can convert between internal unparsed plain text
     * (with notations to indicate formatting, etc) to the structured parsed
     * format.
     */
    class ExternalTextConverter
    {
    public:
        /**
         * Converts internal (unparsed) plain text to external parsed form.
         * Caller must manage pointers in returned ExternalTextLine.
         * @param text[in] The line of text to convert.
         * @return The parsed form or empty if error.
         */
        static ExternalTextLine to_external(const std::string &text);

        /**
         * Converts external parsed text to internal plain text form.
         * @param external_text[in] The line of text to convert.
         * @return The internal (plain) notated text form.
         */
        static std::string from_external(
            const ExternalTextLine &external_text);

        /**
         * Converts a single ExternalText element to internal plain text
         * form.
         * @param text_ptr[in] The line of text to convert.
         * @return The internal (plain) notated text form.
         */
        static std::string from_external(const ExternalText *text_ptr);

        /**
         * Converts the ExternalIdText::IdType enum to a string.
         * @param id_type[in] The enum to convert.
         * @return The enum as a string, or invalid if enum is out of range.
         */
        static const std::string &id_type_to_string(
            const ExternalIdText::IdType id_type);

        /**
         * Converts a string to an ExternalIdText::IdType.
         * @param type[in] The string to convert.
         * @return The string as an enum, or the END_INVALID enum if error
         * converting.
         */
        static const ExternalIdText::IdType string_to_id_type(
            const std::string &type);

        /**
         * Converts the ExternalUrlText::UrlType enum to a string.
         * @param url_type[in] The enum to convert.
         * @return The enum as a string, or invalid if enum is out of range.
         */
        static const std::string &url_type_to_string(
            const ExternalUrlText::UrlType url_type);

        /**
         * Converts a string to an ExternalUrlText::UrlType.
         * @param type[in] The string to convert.
         * @return The string as an enum, or the END_INVALID enum if error
         * converting.
         */
        static const ExternalUrlText::UrlType string_to_url_type(
            const std::string &type);

        /**
         * Converts the ExternalFormattedText::Color enum to a string.
         * @param color[in] The enum to convert.
         * @return The enum as a string, or invalid if enum is out of range.
         */
        static const std::string &color_to_string(
            const ExternalFormattedText::Color color);

        /**
         * Converts a string to an ExternalFormattedText::Color.
         * @param color[in] The string to convert.
         * @return The string as an enum, or the END_INVALID enum if error
         * converting.
         */
        static const ExternalFormattedText::Color string_to_color(
            const std::string &color);


    private:
        /**
         * State machine for parsing from internal to external.  Putting it
         * in a simple container class makes it easy to pass around.
         */
        class ParserStateMachine
        {
        public:
            /**
             * Container class constructor.  Defaults everything.
             */
            ParserStateMachine(void)
              : text_ptr(0),
                pos(0),
                in_style(false),
                color(ExternalFormattedText::COLOR_DEFAULT),
                red(0),
                green(0),
                blue(0),
                bold(false),
                italic(false),
                underline(false),
                inverse(false),
                url_type(ExternalUrlText::URL_TYPE_PAGE)
             { }

            /**
             * Container class destructor.
             */
            ~ParserStateMachine()
             { }

            ExternalText *text_ptr; ///< External text being worked on

            std::string::size_type pos; ///< Current position within string
            bool in_style; ///< True if in styled text

            // Style info
            //
            ExternalFormattedText::Color color; ///< What color to use
            osinterface::OsTypes::UnsignedInt8 red; ///< Custom red intensity
            osinterface::OsTypes::UnsignedInt8 green; ///< Custom green intensity
            osinterface::OsTypes::UnsignedInt8 blue; ///< Custom blue intensity
            bool bold; ///< Text is bold
            bool italic; ///< Text is italic
            bool underline; ///< Text is underlined
            bool inverse; ///< Text is inversed

            // URL info
            //
            ExternalUrlText::UrlType url_type; ///< Type of URL
            std::string url_text; ///< The actual URL.  Clear when not in use.
            std::string url_name; ///< The url name/title
        };


        /**
         * Checks for valid style syntax and converts it to an
         * ExternalFormattedText if valid, adding the current ExternalText and
         * the new ExternalFormattedText to output.  If style syntax indicates
         * it wants to display as-is, it will instead append to current
         * ExternalText.
         * It is assumed state.pos is set to the opening URL notation
         * (currently '~`').
         * @param text[in] The entirety of the text line being converted.
         * @param state[in,out] The current state of the conversion.  Text
         * may be added to the current external text pointer.  THe pointer
         * to the external text may have been changed.
         * @param output[out] The external text line being built.
         */
        static void convert_style(
            const std::string &text,
            ParserStateMachine &state,
            ExternalTextLine &output);

        /**
         * Checks for valid URL syntax and converts it to a ExternalUrlText
         * if valid, adding the current ExternalText and the new
         * ExternalUrlText to output.  If URL syntax indicates it wants to
         * display as-is, it will instead append to current ExternalText.
         * It is assumed state.pos is set to the opening URL notation
         * (currently '[[').
         * @param text[in] The entirety of the text line being converted.
         * @param state[in,out] The current state of the conversion.  Text
         * may be added to the current external text pointer.  THe pointer
         * to the external text may have been changed.
         * @param output[out] The external text line being built.
         */
        static void convert_url(
            const std::string &text,
            ParserStateMachine &state,
            ExternalTextLine &output);

        /**
         * Appends a substring of text to the current ExternalText in state
         * starting from the current position in state.  The state position
         * will also advance accordingly when done.
         * @param text[in] The entirety of the text line currently being
         * processed.  This must be the line of text currently being
         * parsed.  It cannot be an arbritrary line of text.
         * @param state[in,out] The current state of the conversion.
         * @param len[in] The number of characters from text to copy into
         * the state ExternalText, starting from the state's position.
         * @see output_string() To output arbritrary lines of text.
         */
        static void output_substring(
            const std::string &text,
            ParserStateMachine &state,
            const std::string::size_type len);

        /**
         * Appends entirety of text to the current ExternalText in state.  The
         * state position is not modified.
         * @param text[in] The text to append.
         * @param state[in,out] The current state of the conversion.
         */
        static void output_string(
            const std::string &text,
            ParserStateMachine &state);

        /**
         * If the current external text pointed to by 'state' is not empty,
         * then append it to the output and null the pointer.
         * @param state[in,out] The current parsing state.
         * @param output[out] The output line being formed.
         */
        static void add_text_segment(
            ParserStateMachine &state,
            ExternalTextLine &output);

        /**
         * If the current state has a null text pointer, make a new one
         * based on if we're in a style or not.
         * @param state[in,out] The current parsing state.
         */
        static void new_text_segment(ParserStateMachine &state);
    };
}
}

#endif //MUTGOS_TEXT_EXTERNALTEXTCONVERTER_H
