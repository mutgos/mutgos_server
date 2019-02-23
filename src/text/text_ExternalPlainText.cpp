/*
 * text_ExternalPlainText.cpp
 */

#include <string>

#include "utilities/json_JsonUtilities.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"

namespace
{
    const static std::string PLAIN_TEXT_KEY = "plainText";
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    bool ExternalPlainText::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ExternalText::save(root, node);

        if (success)
        {
            success = json::add_static_key_value(
                PLAIN_TEXT_KEY,
                plain_text,
                node,
                root);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ExternalPlainText::restore(const json::JSONNode &node)
    {
        bool success = ExternalText::restore(node);

        if (success)
        {
            success = json::get_key_value(PLAIN_TEXT_KEY, node, plain_text);
        }

        return success;
    }
}
}