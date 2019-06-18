#ifndef MUTGOS_TEXT_EXTERNALFORMATTEDTEXT_H
#define MUTGOS_TEXT_EXTERNALFORMATTEDTEXT_H

#include "osinterface/osinterface_OsTypes.h"

#include "text/text_ExternalPlainText.h"

namespace mutgos
{
namespace text
{
    /**
     * Represents formatted text.  That is, text with color or other styling.
     */
    class ExternalFormattedText : public ExternalPlainText
    {
    public:
        /**
         * Indicates text color.
         * Update ExternalTextConverter and AnsiConverter if this enum changes.
         */
        enum Color
        {
            /** Use default color for client.  Must always be first enum entry. */
            COLOR_DEFAULT,
            /** Use custom RGB values for text color */
            COLOR_CUSTOM,
            /** Text color is black */
            COLOR_BLACK,
            /** Text color is red */
            COLOR_RED,
            /** Text color is yellow */
            COLOR_YELLOW,
            /** Text color is green */
            COLOR_GREEN,
            /** Text color is cyan */
            COLOR_CYAN,
            /** Text color is blue */
            COLOR_BLUE,
            /** Text color is magenta */
            COLOR_MAGENTA,
            /** Text color is white */
            COLOR_WHITE,
            /** Internal use only.  Insert new enums above this. */
            COLOR_END_INVALID
        };

        /**
         * Creates an external formatted text, suitable for deserialization.
         */
        ExternalFormattedText(void)
            : ExternalPlainText(ExternalText::TEXT_TYPE_FORMATTED_TEXT),
              red(0),
              green(0),
              blue(0),
              color_style(COLOR_DEFAULT),
              bold_style(false),
              italic_style(false),
              underline_style(false),
              inverse_style(false)
        { }

        /**
         * Creates an external formatted text.
         * @param text[in] The text.
         * @param bold[in] True if text is to be bold.
         * @param italic[in] True if text is to be italicized.
         * @param underline[in] True if text is to be underlined.
         * @param inverse[in] True if text is to be inversed.
         * @param color[in] Indicate if preset color, custom, or default.
         * @param red_intensity[in] If use_color true, the red intensity.
         * @param green_intensity[in] If use_color true, the green intensity.
         * @param blue_intensity[in] If use_color true, the blue intensity.
         */
        ExternalFormattedText(
            const std::string &text,
            const bool bold,
            const bool italic,
            const bool underline,
            const bool inverse,
            const Color color,
            const osinterface::OsTypes::UnsignedInt8 red_intensity = 0,
            const osinterface::OsTypes::UnsignedInt8 green_intensity = 0,
            const osinterface::OsTypes::UnsignedInt8 blue_intensity = 0)

          : ExternalPlainText(ExternalText::TEXT_TYPE_FORMATTED_TEXT, text),
            red(red_intensity),
            green(green_intensity),
            blue(blue_intensity),
            color_style(color),
            bold_style(bold),
            italic_style(italic),
            underline_style(underline),
            inverse_style(inverse)
        { }

        /**
         * Creates an external formatted text with empty text.
         * @param bold[in] True if text is to be bold.
         * @param italic[in] True if text is to be italicized.
         * @param underline[in] True if text is to be underlined.
         * @param inverse[in] True if text is to be inversed.
         * @param color[in] Indicate if preset color, custom, or default.
         * @param red_intensity[in] If use_color true, the red intensity.
         * @param green_intensity[in] If use_color true, the green intensity.
         * @param blue_intensity[in] If use_color true, the blue intensity.
         */
        ExternalFormattedText(
            const bool bold,
            const bool italic,
            const bool underline,
            const bool inverse,
            const Color color,
            const osinterface::OsTypes::UnsignedInt8 red_intensity = 0,
            const osinterface::OsTypes::UnsignedInt8 green_intensity = 0,
            const osinterface::OsTypes::UnsignedInt8 blue_intensity = 0)

            : ExternalPlainText(ExternalText::TEXT_TYPE_FORMATTED_TEXT, ""),
              red(red_intensity),
              green(green_intensity),
              blue(blue_intensity),
              color_style(color),
              bold_style(bold),
              italic_style(italic),
              underline_style(underline),
              inverse_style(inverse)
        { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ExternalFormattedText(const ExternalFormattedText &rhs)
            : ExternalPlainText(rhs),
              red(rhs.red),
              green(rhs.green),
              blue(rhs.blue),
              color_style(rhs.color_style),
              bold_style(rhs.bold_style),
              italic_style(rhs.italic_style),
              underline_style(rhs.underline_style),
              inverse_style(rhs.inverse_style)
        { }

        /**
         * Destructor.
         */
        virtual ~ExternalFormattedText()
         { }

        /**
         * @return How much memory this ExternalText instance uses.
         */
        virtual size_t mem_used(void) const
        {
            return ExternalPlainText::mem_used();
        }

        /**
         * Creates a copy of this ExternalText.
         * @return A cloned copy.  Caller must manage the pointer.
         */
        virtual ExternalText *clone(void) const
        {
            return new ExternalFormattedText(*this);
        }

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

        /**
         * @return Color to use (default, preset, or custom).
         */
        const Color get_color(void) const
          { return color_style; }

        /**
         * Confirm text is using color before calling.
         * @return The red intensity of the text.
         */
        const osinterface::OsTypes::UnsignedInt8 get_red_intensity(void) const
          { return red; }

        /**
         * Confirm text is using color before calling.
         * @return The green intensity of the text.
         */
        const osinterface::OsTypes::UnsignedInt8 get_green_intensity(void) const
          { return green; }

        /**
         * Confirm text is using color before calling.
         * @return The blue intensity of the text.
         */
        const osinterface::OsTypes::UnsignedInt8 get_blue_intensity(void) const
          { return blue; }

        /**
         * @return True if text is to be formatted bold.
         */
        const bool is_bold(void) const
          { return bold_style; }

        /**
         * @return True if text is to be formatted italic.
         */
        const bool is_italic(void) const
          { return italic_style; }

        /**
         * @return True if text is to be formatted underlined.
         */
        const bool is_underlined(void) const
          { return underline_style; }

        /**
         * @return True if background/foreground colors are inverted.
         */
        const bool is_inverse(void) const
          { return inverse_style; }

    private:
        /** Red color intensity 0-255 */
        osinterface::OsTypes::UnsignedInt8 red;
        /** Green color intensity 0-255 */
        osinterface::OsTypes::UnsignedInt8 green;
        /** Blue color intensity 0-255 */
        osinterface::OsTypes::UnsignedInt8 blue;
        /** Indicates if using preset color, custom, or default */
        Color color_style;
        /** True if text is to be bold */
        bool bold_style;
        /** True if text is to be italicized */
        bool italic_style;
        /** True if text is to be underlined */
        bool underline_style;
        /** Text background/foreground color inverted */
        bool inverse_style;
    };
}
}

#endif //MUTGOS_TEXT_EXTERNALFORMATTEDTEXT_H
