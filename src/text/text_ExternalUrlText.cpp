/*
 * text_ExternalUrlText.cpp
 */

#include <string>

#include "utilities/json_JsonUtilities.h"

#include "text/text_ExternalUrlText.h"
#include "text/text_ExternalText.h"
#include "text/text_ExternalTextConverter.h"

namespace
{
    const static std::string URL_TYPE_KEY = "type";
    const static std::string URL_TEXT_KEY = "url";
    const static std::string URL_NAME_KEY = "name";
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    bool ExternalUrlText::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ExternalText::save(root, node);

        if (success)
        {
            success = json::add_static_key_static_value(
                URL_TYPE_KEY,
                ExternalTextConverter::url_type_to_string(url_type),
                node,
                root) and success;

            success = json::add_static_key_value(
                URL_TEXT_KEY,
                url_text,
                node,
                root) and success;

            success = json::add_static_key_value(
                URL_NAME_KEY,
                url_name,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ExternalUrlText::restore(const json::JSONNode &node)
    {
        bool success = ExternalText::restore(node);

        if (success)
        {
            std::string url_type_string;

            success = json::get_key_value(URL_TYPE_KEY, node, url_type_string);

            if (not url_type_string.empty())
            {
                url_type = ExternalTextConverter::string_to_url_type(url_type_string);

                success = success and (url_type != URL_TYPE_END_INVALID);
            }

            success = json::get_key_value(URL_TEXT_KEY, node, url_text)
                and success;

            success = json::get_key_value(URL_NAME_KEY, node, url_name)
                      and success;
        }

        return success;
    }
}
}
