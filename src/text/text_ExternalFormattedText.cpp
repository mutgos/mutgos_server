/*
 * text_ExternalFormattedText.cpp
 */

#include <string>

#include "utilities/json_JsonUtilities.h"

#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalFormattedText.h"
#include "text/text_ExternalTextConverter.h"

namespace
{
    const static std::string RED_KEY = "red";
    const static std::string GREEN_KEY = "green";
    const static std::string BLUE_KEY = "blue";
    const static std::string COLOR_STYLE_KEY = "color";
    const static std::string BOLD_STYLE_KEY = "bold";
    const static std::string ITALIC_STYLE_KEY = "italic";
    const static std::string UNDERLINE_STYLE_KEY = "underline";
    const static std::string INVERSE_STYLE_KEY = "inverse";
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    bool ExternalFormattedText::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ExternalPlainText::save(root, node);

        if (success)
        {
            success = json::add_static_key_static_value(
                COLOR_STYLE_KEY,
                ExternalTextConverter::color_to_string(color_style),
                node,
                root);

            if (color_style == COLOR_CUSTOM)
            {
                // Need to also serialize RGB values
                //
                success = json::add_static_key_value(RED_KEY, red, node, root)
                    and success;
                success = json::add_static_key_value(GREEN_KEY, green, node, root)
                    and success;
                success = json::add_static_key_value(BLUE_KEY, blue, node, root)
                    and success;
            }

            // Only serialize the boolean styles if they are true, to conserve
            // space and CPU.
            //

            if (bold_style)
            {
                success = json::add_static_key_value(
                    BOLD_STYLE_KEY,
                    bold_style,
                    node,
                    root) and success;
            }

            if (italic_style)
            {
                success = json::add_static_key_value(
                    ITALIC_STYLE_KEY,
                    italic_style,
                    node,
                    root) and success;
            }

            if (underline_style)
            {
                success = json::add_static_key_value(
                    UNDERLINE_STYLE_KEY,
                    underline_style,
                    node,
                    root) and success;
            }

            if (underline_style)
            {
                success = json::add_static_key_value(
                    INVERSE_STYLE_KEY,
                    inverse_style,
                    node,
                    root) and success;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ExternalFormattedText::restore(const json::JSONNode &node)
    {
        bool success = ExternalPlainText::restore(node);
        std::string color_style_string;

        if (success)
        {
            success =
                json::get_key_value(COLOR_STYLE_KEY, node, color_style_string);

            if (success)
            {
                color_style = ExternalTextConverter::string_to_color(
                    color_style_string);

                success = (color_style != COLOR_END_INVALID);
            }

            if (not success)
            {
                color_style = COLOR_DEFAULT;
            }
            else
            {
                if (color_style == COLOR_CUSTOM)
                {
                    MG_UnsignedInt int_value = 0;

                    success = json::get_key_value(RED_KEY, node, int_value)
                        and success;
                    red = (osinterface::OsTypes::UnsignedInt8) int_value;

                    success = json::get_key_value(GREEN_KEY, node, int_value)
                              and success;
                    green = (osinterface::OsTypes::UnsignedInt8) int_value;

                    success = json::get_key_value(BLUE_KEY, node, int_value)
                              and success;
                    blue = (osinterface::OsTypes::UnsignedInt8) int_value;
                }

                // Defaults to false, and these are all optional.
                //
                json::get_key_value(BOLD_STYLE_KEY, node, bold_style);
                json::get_key_value(ITALIC_STYLE_KEY, node, italic_style);
                json::get_key_value(UNDERLINE_STYLE_KEY, node, underline_style);
                json::get_key_value(INVERSE_STYLE_KEY, node, inverse_style);
            }
        }

        return success;
    }
}
}
