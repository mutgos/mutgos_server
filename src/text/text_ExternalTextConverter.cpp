/*
 * text_ExternalTextConverter.cpp
 */

#include <ctype.h>
#include <string>
#include "text/text_StringConversion.h"

#include "osinterface/osinterface_OsTypes.h"
#include "text/text_StringParsing.h"
#include "logging/log_Logger.h"

#include "text/text_ExternalTextConverter.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalIdText.h"
#include "text/text_ExternalUrlText.h"
#include "text/text_ExternalFormattedText.h"

namespace
{
    /** @see ExternalIdText::IdType */
    const static std::string ID_TYPE_AS_STRING[] =
    {
        "entity",
        "action",
        "exit",
        "INVALID"
    };

    /** @see ExternalUrlText::UrlType */
    const static std::string URL_TYPE_AS_STRING[] =
    {
        "page",
        "image",
        "audio",
        "INVALID"
    };

    /** @see ExternalFormattedText::Color */
    const static std::string COLOR_AS_STRING[] =
    {
        "default",
        "custom",
        "black",
        "red",
        "yellow",
        "green",
        "cyan",
        "blue",
        "magenta",
        "white",
        "INVALID"
    };

    /** Opening token for URLs */
    const static std::string OPEN_URL = "[[";
    /** Extra opening token for taking URL literally */
    const static char OPEN_URL_LIT = OPEN_URL[0];
    /** Closing token for URLs */
    const static std::string CLOSE_URL = "]]";
    /** Extra closing token for taking URL literally */
    const static char CLOSE_URL_LIT = CLOSE_URL[0];
    /** Separator token for URLs */
    const static std::string URL_SEP = "||";
    /** How many parameters a URL tag can have */
    const static MG_UnsignedInt MAX_URL_PARAMS = 3;

    /** Opening token for color */
    const static std::string OPEN_STYLE = "~`";
    /** Closing token for color */
    const static std::string CLOSE_STYLE = "`";
    /** Separator token for color */
    const static std::string STYLE_SEP = ",";
    /** Max characters in a color tag */
    const static MG_UnsignedInt MAX_STYLE_SIZE = 64;

    /** Show format token, to indicate style should be shown */
    const static std::string SHOW_STYLE = "showfmt";
    const static std::string BOLD_STYLE = "bold";
    const static std::string ITALIC_STYLE = "italic";
    const static std::string UNDERLINE_STYLE = "underline";
    const static std::string UNDERLINE_SHORT_STYLE = "uline";
    const static std::string INVERSE_STYLE = "inverse";
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    ExternalTextLine ExternalTextConverter::to_external(
        const std::string &text)
    {
        ExternalTextLine output;
        ParserStateMachine state;

        new_text_segment(state);

        std::string::size_type style_index = 0;
        std::string::size_type url_index = 0;
        bool style_found = true;
        bool url_found = true;

        while (style_found or url_found)
        {
            // This loop will append text to the current ExternalText instance
            // held in the 'state' up to what might be a potentially
            // convertible token, and then perform the conversion.  The
            // conversion method may output additional text to 'state' or
            // change the ExternalText pointer completely.
            // The loop does not exit until all potential tokens and text
            // have been processed for the line of text.

            // Find next style and URL, and process the nearest one first.
            //
            style_index = text.find(OPEN_STYLE, state.pos);
            style_found = (style_index != std::string::npos);
            url_index = text.find(OPEN_URL, state.pos);
            url_found = (url_index != std::string::npos);

            if ((not style_found) and (not url_found))
            {
                // Only plain text is left.  Output remainder of text as-is.
                output_substring(text, state, text.size() - state.pos);
            }
            else if (style_found and (not url_found))
            {
                // Only a style has been found further on.  Output whatever
                // text is between here and the token, and then process the
                // potential style.
                //
                output_substring(text, state, style_index - state.pos);
                convert_style(text, state, output);
                new_text_segment(state);
            }
            else if ((not style_found) and url_found)
            {
                // Only a URL has been found further on.  Output whatever
                // text is between here and the token, and then process the
                // potential URL.
                //
                output_substring(text, state, url_index - state.pos);
                convert_url(text, state, output);
                new_text_segment(state);
            }
            else if (style_index <= url_index)
            {
                // Both style and URL found, but style occurs sooner.
                // Output whatever text is between here and the token and then
                // process the potential style.
                //
                output_substring(text, state, style_index - state.pos);
                convert_style(text, state, output);
                new_text_segment(state);
            }
            else
            {
                // Both style and URL found, but URL occurs sooner.
                // Output whatever text is between here and the token and then
                // process the potential URL.
                //
                output_substring(text, state, url_index - state.pos);
                convert_url(text, state, output);
                new_text_segment(state);
            }
        }

        // Add the last text segment from 'state' to the output, if any, since
        // it is not automatically added.
        add_text_segment(state, output);

        return output;
    }

    // ----------------------------------------------------------------------
    std::string ExternalTextConverter::from_external(
        const ExternalTextLine &external_text)
    {
        std::string output;

        for (ExternalTextLine::const_iterator item = external_text.begin();
            item != external_text.end();
            ++item)
        {
            output.append(from_external(*item));
        }

        return output;
    }

    // ----------------------------------------------------------------------
    std::string ExternalTextConverter::from_external(
        const ExternalText *text_ptr)
    {
        std::string output;

        if (text_ptr)
        {
            switch (text_ptr->get_text_type())
            {
                case ExternalText::TEXT_TYPE_PLAIN_TEXT:
                {
                    // Unformatted text.  Just append as-is to our output.
                    //
                    output.append(text_ptr->to_string());
                    break;
                }

                case ExternalText::TEXT_TYPE_URL:
                {
                    // Output a URL.
                    //
                    const ExternalUrlText *url_ptr =
                        dynamic_cast<const ExternalUrlText *>(text_ptr);

                    if (url_ptr)
                    {
                        output.append(OPEN_URL);
                        output.append(url_ptr->get_url());

                        if (not url_ptr->get_url_name().empty())
                        {
                            output.append(" ");
                            output.append(URL_SEP);
                            output.append(" ");
                            output.append(url_ptr->get_url_name());
                            output.append(" ");
                            output.append(URL_SEP);
                            output.append(" ");
                            output.append(
                                url_type_to_string(url_ptr->get_url_type()));
                        }

                        output.append(CLOSE_URL);
                    }
                    else
                    {
                        LOG(fatal, "text", "from_external(item)",
                            "Failed to cast to ExternalUrlText!");

                        output.append(text_ptr->to_string());
                    }

                    break;
                }

                case ExternalText::TEXT_TYPE_ID:
                {
                    // Output an ID.  Currently ID conversion is a WIP,
                    // so this is just something temporary.
                    // TODO Implement permanent ID conversion solution
                    //
                    const ExternalIdText *id_ptr =
                        dynamic_cast<const ExternalIdText *>(text_ptr);

                    if (id_ptr)
                    {
                        output.append(id_ptr->get_name());
                        output.append("(");
                        output.append(id_ptr->get_id().to_string(false));
                        output.append(")");
                    }
                    else
                    {
                        LOG(fatal, "text", "from_external(item)",
                            "Failed to cast to ExternalIdText!");

                        output.append(text_ptr->to_string());
                    }

                    break;
                }

                case ExternalText::TEXT_TYPE_FORMATTED_TEXT:
                {
                    // Output formatted (styled) text.
                    //
                    const ExternalFormattedText *style_ptr =
                        dynamic_cast<const ExternalFormattedText *>(text_ptr);
                    bool entry_added = false;

                    if (style_ptr)
                    {
                        output.append(OPEN_STYLE);

                        const ExternalFormattedText::Color color =
                            style_ptr->get_color();

                        if (color != ExternalFormattedText::COLOR_DEFAULT)
                        {
                            if (color == ExternalFormattedText::COLOR_CUSTOM)
                            {
                                // Uses custom color, so output RGB
                                //
                                output.append(text::to_string(
                                    style_ptr->get_red_intensity()));
                                output.append(STYLE_SEP);
                                output.append(text::to_string(
                                    style_ptr->get_green_intensity()));
                                output.append(STYLE_SEP);
                                output.append(text::to_string(
                                    style_ptr->get_blue_intensity()));

                                entry_added = true;
                            }
                            else
                            {
                                // If it's not default or custom, it's a normal
                                // color.
                                //
                                output.append(color_to_string(color));
                                output.append(STYLE_SEP);

                                entry_added = true;
                            }
                        }

                        if (style_ptr->is_bold())
                        {
                            if (entry_added)
                            {
                                output.append(STYLE_SEP);
                            }

                            output.append("bold");
                            entry_added = true;
                        }

                        if (style_ptr->is_italic())
                        {
                            if (entry_added)
                            {
                                output.append(STYLE_SEP);
                            }

                            output.append("italic");
                            entry_added = true;
                        }

                        if (style_ptr->is_underlined())
                        {
                            if (entry_added)
                            {
                                output.append(STYLE_SEP);
                            }

                            output.append("underline");
                            entry_added = true;
                        }

                        output.append(CLOSE_STYLE);

                        output.append(style_ptr->to_string());

                        output.append(OPEN_STYLE);
                        output.append(CLOSE_STYLE);
                    }
                    else
                    {
                        LOG(fatal, "text", "from_external(item)",
                            "Failed to cast to ExternalFormattedText!");

                        output.append(text_ptr->to_string());
                    }

                    break;
                }

                default:
                {
                    LOG(fatal, "text", "from_external(item)",
                        "Unknown text type!");

                    output.append(text_ptr->to_string());
                }
            }
        }

        return output;
    }

    // ----------------------------------------------------------------------
    const std::string &ExternalTextConverter::id_type_to_string(
        const ExternalIdText::IdType id_type)
    {
        if ((id_type >= ExternalIdText::ID_TYPE_END_INVALID) or
            (id_type < ExternalIdText::ID_TYPE_ENTITY))
        {
            return ID_TYPE_AS_STRING[ExternalIdText::ID_TYPE_END_INVALID];
        }

        return ID_TYPE_AS_STRING[id_type];
    }

    // ----------------------------------------------------------------------
    const ExternalIdText::IdType ExternalTextConverter::string_to_id_type(
        const std::string &type)
    {
        ExternalIdText::IdType enum_type = ExternalIdText::ID_TYPE_END_INVALID;

        // Check each string for a match.
        for (int index = 0;
             index < ExternalIdText::ID_TYPE_END_INVALID;
             ++index)
        {
            if (ID_TYPE_AS_STRING[index] == type)
            {
                enum_type = (ExternalIdText::IdType) index;
                break;
            }
        }

        return enum_type;
    }

    // ----------------------------------------------------------------------
    const std::string &ExternalTextConverter::url_type_to_string(
        const ExternalUrlText::UrlType url_type)
    {
        if ((url_type >= ExternalUrlText::URL_TYPE_END_INVALID) or
            (url_type < ExternalUrlText::URL_TYPE_PAGE))
        {
            return URL_TYPE_AS_STRING[ExternalUrlText::URL_TYPE_END_INVALID];
        }

        return URL_TYPE_AS_STRING[url_type];
    }

    // ----------------------------------------------------------------------
    const ExternalUrlText::UrlType ExternalTextConverter::string_to_url_type(
        const std::string &type)
    {
        ExternalUrlText::UrlType enum_type =
            ExternalUrlText::URL_TYPE_END_INVALID;

        // Check each string for a match.
        for (int index = 0;
             index < ExternalUrlText::URL_TYPE_END_INVALID;
             ++index)
        {
            if (URL_TYPE_AS_STRING[index] == type)
            {
                enum_type = (ExternalUrlText::UrlType) index;
                break;
            }
        }

        return enum_type;
    }

    // ----------------------------------------------------------------------
    const std::string &ExternalTextConverter::color_to_string(
        const ExternalFormattedText::Color color)
    {
        if ((color >= ExternalFormattedText::COLOR_END_INVALID) or
            (color < ExternalFormattedText::COLOR_DEFAULT))
        {
            return COLOR_AS_STRING[ExternalFormattedText::COLOR_END_INVALID];
        }

        return COLOR_AS_STRING[color];
    }

    // ----------------------------------------------------------------------
    const ExternalFormattedText::Color ExternalTextConverter::string_to_color(
        const std::string &color)
    {
        ExternalFormattedText::Color enum_type =
            ExternalFormattedText::COLOR_END_INVALID;

        // Check each string for a match.
        for (int index = 0;
             index < ExternalFormattedText::COLOR_END_INVALID;
             ++index)
        {
            if (COLOR_AS_STRING[index] == color)
            {
                enum_type = (ExternalFormattedText::Color) index;
                break;
            }
        }

        return enum_type;
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::convert_style(
        const std::string &text,
        ParserStateMachine &state,
        ExternalTextLine &output)
    {
        // Find ending mark
        //
        const std::string::size_type end_style_index =
            text.find(CLOSE_STYLE, state.pos + OPEN_STYLE.size());

        // Shortcut check for reset
        //
        if ((state.pos + OPEN_STYLE.size()) == end_style_index)
        {
            // Reset found
            //
            ExternalFormattedText * const output_ptr =
                dynamic_cast<ExternalFormattedText *>(state.text_ptr);

            if (output_ptr)
            {
                // Finished formatting text
                if (state.text_ptr)
                {
                    add_text_segment(state, output);
                }
                else
                {
                    LOG(fatal, "text", "convert_style",
                        "text_ptr was null when handling reset!");
                }

                state.pos = end_style_index + CLOSE_STYLE.size();
                state.in_style = false;
                return;
            }
            else
            {
                // Text wasn't being formatted.  Output as-is.
                output_substring(text, state, OPEN_STYLE.size());
                state.in_style = false;
                return;
            }
        }

        // See if format string is too long -
        //
        if ((end_style_index == std::string::npos) or
            ((end_style_index - state.pos) > MAX_STYLE_SIZE))
        {
            // Too long.  Don't parse.
            output_substring(text, state, OPEN_STYLE.size());
            return;
        }

        // Parse to see what options are present
        //
        const std::string::size_type begin_style_index =
            state.pos + OPEN_STYLE.size();
        text::StringParsing::SplitStrings style_params =
            text::StringParsing::split_string(
                text,
                begin_style_index,
                end_style_index - begin_style_index,
                STYLE_SEP);

        ExternalFormattedText::Color color =
            ExternalFormattedText::COLOR_END_INVALID;
        osinterface::OsTypes::UnsignedInt8 red = 0;
        osinterface::OsTypes::UnsignedInt8 green = 0;
        osinterface::OsTypes::UnsignedInt8 blue = 0;
        // Used when parsing the colors, to know which one we're at
        osinterface::OsTypes::UnsignedInt8 color_index = 0;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool inverse = false;
        bool bad_format = false;
        text::StringParsing::SplitStrings::iterator showfmt_iter =
            style_params.end();


        // Go through each entry to try and set the style options
        //
        for (text::StringParsing::SplitStrings::iterator iter =
                style_params.begin();
             (iter != style_params.end()) and (not bad_format);
            ++iter)
        {
            if (not iter->empty())
            {
                if (isdigit((*iter)[0]))
                {
                    // Take three sets of numbers and make them red, green, blue
                    // For simplicity, don't enforce they must be sequential.
                    //
                    if (color == ExternalFormattedText::COLOR_END_INVALID)
                    {
                        // First number
                        if (not text::from_string<
                            osinterface::OsTypes::UnsignedInt8>(*iter, red))
                        {
                            bad_format = true;
                        }
                        else
                        {
                            color = ExternalFormattedText::COLOR_CUSTOM;
                            ++color_index;
                        }
                    }
                    else if (color == ExternalFormattedText::COLOR_CUSTOM)
                    {
                        osinterface::OsTypes::UnsignedInt8 parsed_color = 0;

                        if (not text::from_string<
                                osinterface::OsTypes::UnsignedInt8>(
                                  *iter,
                                  parsed_color))
                        {
                            bad_format = true;
                        }
                        else
                        {
                            if (color_index == 1)
                            {
                                green = parsed_color;
                                ++color_index;
                            }
                            else if (color_index == 2)
                            {
                                blue = parsed_color;
                                ++color_index;
                            }
                            else
                            {
                                bad_format = true;
                            }
                        }
                    }
                    else
                    {
                        bad_format = true;
                    }
                }
                else
                {
                    // See if it's one of the styles.
                    //
                    if (*iter == BOLD_STYLE)
                    {
                        bold = true;
                    }
                    else if (*iter == ITALIC_STYLE)
                    {
                        italic = true;
                    }
                    else if ((*iter == UNDERLINE_STYLE) or
                        (*iter == UNDERLINE_SHORT_STYLE))
                    {
                        underline = true;
                    }
                    else if (*iter == INVERSE_STYLE)
                    {
                        inverse = true;
                    }
                    else if (*iter == SHOW_STYLE)
                    {
                        // It's showformat.  Only allowed once.
                        //
                        if (showfmt_iter != style_params.end())
                        {
                            showfmt_iter = iter;
                        }
                        else
                        {
                            bad_format = true;
                        }
                    }
                    else
                    {
                        // See if it's a color.
                        //
                        const ExternalFormattedText::Color parsed_color =
                            string_to_color(*iter);

                        if (parsed_color !=
                            ExternalFormattedText::COLOR_END_INVALID)
                        {
                            if (color ==
                                ExternalFormattedText::COLOR_END_INVALID)
                            {
                                color = parsed_color;
                            }
                            else
                            {
                                bad_format = true;
                            }
                        }
                    }
                }
            }
        }

        // Went through all the tokens, now decide what to output.
        //
        if (bad_format)
        {
            output_substring(text, state, OPEN_STYLE.size());
        }
        else if (showfmt_iter != style_params.end())
        {
            // Show format desired.  Erase the 'showformat' style so it won't
            // be included in the reconversion.
            //
            bool first_entry = true;
            style_params.erase(showfmt_iter);
            showfmt_iter = style_params.end();

            // Reconvert the parsed style back to a series of comma separated
            // entries with the open and close constants.
            //
            state.pos = end_style_index + 1;
            output_string(OPEN_STYLE, state);

            for (text::StringParsing::SplitStrings::iterator iter =
                style_params.begin();
                 iter != style_params.end();
                 ++iter)
            {
                output_string(*iter, state);

                if (first_entry)
                {
                    first_entry = false;
                }
                else
                {
                    output_string(STYLE_SEP, state);
                }
            }

            output_string(CLOSE_STYLE, state);
        }
        else if ((color == ExternalFormattedText::COLOR_END_INVALID) and
            (not bold) and (not italic) and (not underline) and (not inverse))
        {
            // This is a reset
            //
            add_text_segment(state, output);
            state.in_style = false;
            state.pos = end_style_index + 1;
        }
        else
        {
            // Valid.  Set to the current text type and save off formatting.
            //
            add_text_segment(state, output);

            state.in_style = true;
            state.bold = bold;
            state.italic = italic;
            state.underline = underline;
            state.inverse = inverse;
            state.color = color;
            state.red = red;
            state.green = green;
            state.blue = blue;

            state.text_ptr = new ExternalFormattedText(
                bold, italic, underline, inverse,
                color,
                red, green, blue);

            state.pos = end_style_index + 1;
        }
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::convert_url(
        const std::string &text,
        ParserStateMachine &state,
        ExternalTextLine &output)
    {
        bool converted_url = false;

        // Shortcut check in case of something like [[[[[[[[[[]]]]]]]]]]]]
        // Or even [[[<eot>
        //
        if ((text.size() <= (state.pos + OPEN_URL.size() + CLOSE_URL.size())) or
            ((text.size() > (state.pos + OPEN_URL.size() + 1)) and
            text[state.pos + OPEN_URL.size() + 1] == OPEN_URL_LIT))
        {
            output_substring(text, state, OPEN_URL.size());
            return;
        }

        // Find ending brackets
        //
        const std::string::size_type end_bracket_index =
            text.find(CLOSE_URL, state.pos);

        if (end_bracket_index == std::string::npos)
        {
            output_substring(text, state, OPEN_URL.size());
            return;
        }

        // Determine if tripled
        //
        if ((text[state.pos + OPEN_URL.size()] == OPEN_URL_LIT) and
            (text.size() > (end_bracket_index + CLOSE_URL.size())) and
            (text[state.pos + OPEN_URL.size() + 1] != OPEN_URL_LIT) and
            (text[end_bracket_index + CLOSE_URL.size()] == CLOSE_URL_LIT))
        {
            // Tripled, meaning we need to strip off the extra token character
            // and append it to the current text as-is, since it's supposed
            // to be a literal.
            //
            ExternalPlainText * const output_ptr =
                dynamic_cast<ExternalPlainText *>(state.text_ptr);

            if (not output_ptr)
            {
                LOG(fatal, "text", "convert_url",
                    "Expected plain text pointer but was some other type!");
            }
            else
            {
                output_ptr->append_text(OPEN_URL);
                state.pos += OPEN_URL.size() + 1;
                output_ptr->append_text(
                    text,
                    state.pos,
                    end_bracket_index - state.pos);
                output_ptr->append_text(CLOSE_URL);
                state.pos = end_bracket_index + CLOSE_URL.size() + 1;
            }

            return;
        }

        // 'Normal' URL syntax.  Split it into 1-3 sections and set all the
        // options.  If any option doesn't parse, the string will be output
        // as-is.
        //
        state.pos += OPEN_URL.size();

        // Run through parser and attempt to extract 1-3 strings
        //
        const text::StringParsing::SplitStrings url_params =
            text::StringParsing::split_string(
                text,
                state.pos,
                end_bracket_index - state.pos,
                URL_SEP,
                MAX_URL_PARAMS,
                true);

        state.url_type = ExternalUrlText::URL_TYPE_PAGE;
        state.url_text.clear();
        state.url_name.clear();

        // Get the URL itself...
        //
        if (not url_params.empty())
        {
            state.url_text = url_params[0];
            converted_url = true;
        }

        // ... And the name if it has one ...
        if (converted_url and (url_params.size() > 1))
        {
            state.url_name = url_params[1];
        }

        // ... And finally the URL type if it has one.
        //
        if (converted_url and (url_params.size() > 2))
        {
            state.url_type = string_to_url_type(url_params[2]);

            if (state.url_type == ExternalUrlText::URL_TYPE_END_INVALID)
            {
                // Not a valid enum
                converted_url = false;
            }
        }

        if (converted_url)
        {
            // URL converted.  Add previous external text, then create a URL
            // text and add it to the end.
            //
            add_text_segment(state, output);

            ExternalUrlText * const url_ptr = new ExternalUrlText(
                state.url_type,
                state.url_text,
                state.url_name.empty() ? state.url_text : state.url_name);

            output.push_back(url_ptr);
            state.pos = end_bracket_index + CLOSE_URL.size();
        }
        else
        {
            // URL was not successfully converted.  Get past the opening
            // token and let the caller take care of the rest.
            output_substring(text, state, OPEN_URL.size());
        }

        return;
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::output_substring(
        const std::string &text,
        ParserStateMachine &state,
        const std::string::size_type len)
    {
        if (len)
        {
            ExternalPlainText * const output_ptr =
                dynamic_cast<ExternalPlainText *>(state.text_ptr);

            if (output_ptr)
            {
                output_ptr->append_text(text, state.pos, len);
            }
            else
            {
                LOG(fatal, "text", "output_substring",
                    "Expected plain text pointer but was some other type!");
            }

            state.pos += len;
        }
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::output_string(
        const std::string &text,
        ParserStateMachine &state)
    {
        ExternalPlainText * const output_ptr =
            dynamic_cast<ExternalPlainText *>(state.text_ptr);

        if (output_ptr)
        {
            output_ptr->append_text(text);
        }
        else
        {
            LOG(fatal, "text", "output_string",
                "Expected plain text pointer but was some other type!");
        }
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::add_text_segment(
        ExternalTextConverter::ParserStateMachine &state,
        ExternalTextLine &output)
    {
        if (state.text_ptr)
        {
            ExternalPlainText * const plain_text_ptr =
                dynamic_cast<ExternalPlainText *>(state.text_ptr);

            if (plain_text_ptr)
            {
                if (not plain_text_ptr->is_text_empty())
                {
                    // Text has something in it.  Append and reset.
                    output.push_back(state.text_ptr);
                }
                else
                {
                    // Text has nothing in it.  Don't add and delete.
                    delete state.text_ptr;
                }
            }
            else
            {
                // Something that is not pure text.  Always add those.
                output.push_back(state.text_ptr);
            }

            state.text_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void ExternalTextConverter::new_text_segment(
        ExternalTextConverter::ParserStateMachine &state)
    {
        if (not state.text_ptr)
        {
            // Need to populate text pointer based on if we're in a style
            // or not.
            //
            if (state.in_style)
            {
                // In a style, so make a styled text.
                //
                state.text_ptr = new ExternalFormattedText(
                    state.bold,
                    state.italic,
                    state.underline,
                    state.inverse,
                    state.color,
                    state.red,
                    state.green,
                    state.blue);
            }
            else
            {
                // Not in a style, so make it plain.
                state.text_ptr = new ExternalPlainText();
            }
        }
    }
}
}
