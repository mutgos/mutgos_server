
#include <vector>
#include <string>

#include "text/text_ExternalText.h"

#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalFormattedText.h"
#include "text/text_ExternalIdText.h"
#include "text/text_ExternalUrlText.h"

namespace
{
    const static std::string TEXT_LINE_KEY = "textLine";
    const static std::string TEXT_TYPE_KEY = "textType";

    // If this changes, various methods in this class are impacted.  Please
    // check.
    const static std::string TEXT_TYPE_AS_STRING[] =
    {
        "plain",
        "formatted",
        "url",
        "id"
    };
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    void ExternalText::clear_text_line(ExternalTextLine &line)
    {
        for (ExternalTextLine::iterator iter = line.begin();
            iter != line.end();
            ++iter)
        {
            delete *iter;
        }

        line.clear();
    }

    // ----------------------------------------------------------------------
    void ExternalText::clear_text_lines(ExternalTextMultiline &lines)
    {
        for (ExternalTextMultiline::iterator line_iter = lines.begin();
            line_iter != lines.end();
            ++line_iter)
        {
            clear_text_line(*line_iter);
        }

        lines.clear();
    }

    // ----------------------------------------------------------------------
    ExternalTextLine ExternalText::clone_text_line(
        const ExternalTextLine &line)
    {
        ExternalTextLine result;

        for (ExternalTextLine::const_iterator iter = line.begin();
            iter != line.end();
            ++iter)
        {
            if (*iter)
            {
                result.push_back((*iter)->clone());
            }
        }

        return result;
    }


    // ----------------------------------------------------------------------
    std::string ExternalText::to_string(const ExternalTextLine &line)
    {
        std::string result;

        for (ExternalTextLine::const_iterator iter = line.begin();
             iter != line.end();
             ++iter)
        {
            result.append((*iter)->to_string());
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // Note: This is used directly by the comm subsystem and parses data
    // provided by a client.
    //
    // Keep updated with TextType enum.
    bool ExternalText::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = false;

        if ((text_type <= TEXT_TYPE_ID) and (text_type >= TEXT_TYPE_PLAIN_TEXT))
        {
            success = json::add_static_key_static_value(
                TEXT_TYPE_KEY,
                TEXT_TYPE_AS_STRING[text_type],
                node,
                root);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    // Note: This is used directly by the comm subsystem and parses data
    // provided by a client.
    //
    // Keep updated with TextType enum.
    bool ExternalText::restore(const json::JSONNode &node)
    {
        // Nothing to restore right now because the type is set during
        // construction.

        return true;
    }

    // ----------------------------------------------------------------------
    // Note: This is used directly by the comm subsystem and parses data
    // provided by a client.
    bool ExternalText::save_line(
        const ExternalTextLine &line,
        json::JSONRoot &root,
        json::JSONNode &node)
    {
        bool success = true;

        JSON_MAKE_ARRAY_NODE(text_line_array);

        for (ExternalTextLine::const_iterator line_iter = line.begin();
            line_iter != line.end();
            ++line_iter)
        {
            JSON_MAKE_MAP_NODE(text_node);
            success = (*line_iter)->save(root, text_node) and success;
            success = json::array_add_node(text_node, text_line_array, root)
                      and success;
        }

        success = json::add_static_key_value(
            TEXT_LINE_KEY,
            text_line_array,
            node,
            root) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    // Note: This is used directly by the comm subsystem and parses data
    // provided by a client.
    //
    // Keep updated with TextType enum.
    bool ExternalText::restore_line(
        const json::JSONNode &node,
        ExternalTextLine &line)
    {
        bool success = true;

        clear_text_line(line);

        const json::JSONNode *text_array_ptr = 0;
        success = json::get_key_value(TEXT_LINE_KEY, node, text_array_ptr);

        if (success and text_array_ptr)
        {
            const MG_UnsignedInt line_size = json::array_size(*text_array_ptr);
            std::string type_str;
            ExternalText *text_ptr = 0;

            for (MG_UnsignedInt index = 0; index < line_size; ++index)
            {
                // Get the node for part of the text.  Determine the type,
                // construct the subclass, restore it, and add it to the line.
                //
                const json::JSONNode *text_node_ptr = 0;
                success = json::array_get_node(
                    *text_array_ptr,
                    index,
                    text_node_ptr) and success;

                if (success and text_node_ptr)
                {
                    text_ptr = 0;

                    success = json::get_key_value(
                        TEXT_TYPE_KEY,
                        *text_node_ptr,
                        type_str) and success;

                    if (type_str == TEXT_TYPE_AS_STRING[TEXT_TYPE_PLAIN_TEXT])
                    {
                        text_ptr = new ExternalPlainText();
                    }
                    else if (type_str == TEXT_TYPE_AS_STRING[TEXT_TYPE_FORMATTED_TEXT])
                    {
                        text_ptr = new ExternalFormattedText();
                    }
                    else if (type_str == TEXT_TYPE_AS_STRING[TEXT_TYPE_URL])
                    {
                        text_ptr = new ExternalUrlText();
                    }
                    else if (type_str == TEXT_TYPE_AS_STRING[TEXT_TYPE_ID])
                    {
                        text_ptr = new ExternalIdText();
                    }
                    else
                    {
                        // Unknown type.
                        success = false;
                    }

                    success = (text_ptr and text_ptr->restore(*text_node_ptr))
                              and success;

                    if (text_ptr)
                    {
                        line.push_back(text_ptr);
                    }
                }
            }

            if (not success)
            {
                clear_text_line(line);
            }
        }

        return success;
    }
}
}
