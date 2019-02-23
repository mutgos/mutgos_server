/*
 * text_AnsiConverter.cpp
 */

#include <string>

#include "text_AnsiConverter.h"

#include "text_ExternalText.h"
#include "text_ExternalFormattedText.h"
#include "text_StringConversion.h"

namespace
{
    /** ESC[ sequence, common to all ANSI we use */
    const static char ANSI_ESC_CSI[] = {27, '[', 0};
    /** ESC[m sequence, used to reset everything. */
    const static char ANSI_RESET[] = {27, '[', 'm', 0};
    /** SGR command */
    const static char ANSI_SGR = 'm';

    // These are used as part of SGR

    static const char *const ANSI_FOREGROUND_COLOR = "38";
    static const char ANSI_BOLD = '1';
    static const char ANSI_ITALIC = '3';
    static const char ANSI_UNDERLINE = '4';
    static const char ANSI_INVERSE = '7';
    static const char ANSI_PREDEFINED_COLOR = '5';
    static const char *const ANSI_BACKGROUND_COLOR = "48";
    static const char ANSI_CUSTOM_COLOR = '2';

    /** Separator character */
    const static char ANSI_SEPARATOR = ';';

    /** Maps ExternalFormattedText::Color to ANSI value.  Done in order of the
        enum */
    const static char COLOR_TO_VALUE[] =
        {
            '9',  // Default
            '0',  // Custom (not used)
            '0',  // Black
            '1',  // Red
            '3',  // Yellow
            '2',  // Green
            '6',  // Cyan
            '4',  // Blue
            '5',  // Magenta
            '7',  // White
            '9'   // Invalid
        };
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    // Refer to https://en.wikipedia.org/wiki/ANSI_escape_code
    //          http://wiki.bash-hackers.org/scripting/terminalcodes
    // For info on ANSI escape commands
    std::string to_ansi(const ExternalTextLine &line)
    {
        std::string result;

        for (ExternalTextLine::const_iterator line_iter = line.begin();
            line_iter != line.end();
            ++line_iter)
        {
            if ((*line_iter)->get_text_type() !=
                ExternalText::TEXT_TYPE_FORMATTED_TEXT)
            {
                // Format normally (no color or ANSI effects)
                //
                result += (*line_iter)->to_string();
            }
            else
            {
                // This is a TEXT_TYPE_FORMATTED_TEXT.  Format using ANSI
                //
                const ExternalFormattedText &text =
                    *static_cast<ExternalFormattedText *>(*line_iter);
                const ExternalFormattedText::Color text_color =
                    text.get_color();

                result += ANSI_ESC_CSI;

                if (text.is_bold())
                {
                    result += ANSI_BOLD;
                    result += ANSI_SEPARATOR;
                }

                if (text.is_italic())
                {
                    result += ANSI_ITALIC;
                    result += ANSI_SEPARATOR;
                }

                if (text.is_underlined())
                {
                    result += ANSI_UNDERLINE;
                    result += ANSI_SEPARATOR;
                }

                if (text.is_inverse())
                {
                    result += ANSI_INVERSE;
                    result += ANSI_SEPARATOR;
                }

                if (text_color != ExternalFormattedText::COLOR_DEFAULT)
                {
                    // Add foreground code so we're ready with the colors.
                    //
                    result += ANSI_FOREGROUND_COLOR;
                    result += ANSI_SEPARATOR;

                    // Determine if this is a custom or standard color.
                    //
                    if (text_color != ExternalFormattedText::COLOR_CUSTOM)
                    {
                        // Standard colors
                        //
                        result += ANSI_PREDEFINED_COLOR;
                        result += ANSI_SEPARATOR;
                        result += COLOR_TO_VALUE[text_color];
                        result += ANSI_SEPARATOR;
                    }
                    else
                    {
                        // Custom color
                        //
                        result += ANSI_CUSTOM_COLOR;
                        result += ANSI_SEPARATOR;
                        result += to_string(text.get_red_intensity());
                        result += ANSI_SEPARATOR;
                        result += to_string(text.get_green_intensity());
                        result += ANSI_SEPARATOR;
                        result += to_string(text.get_blue_intensity());
                        result += ANSI_SEPARATOR;
                    }
                }

                if ((not result.empty()) and
                    (result[result.size() - 1] == ANSI_SEPARATOR))
                {
                    // Remove excess separator
                    result.pop_back();
                }

                result += ANSI_SGR;

                result += (*line_iter)->to_string();

                // Reset back to normal when done
                result += ANSI_RESET;
            }
        }

        return result;
    }
}
}
