/*
 * text_ExternalIdText.cpp
 */

#include <string>

#include "utilities/json_JsonUtilities.h"

#include "text/text_ExternalIdText.h"
#include "text/text_ExternalText.h"
#include "text/text_ExternalTextConverter.h"

namespace
{
    const static std::string DB_ID_KEY = "id";
    const static std::string DB_ID_NAME_KEY = "name";
    const static std::string DB_ID_TYPE_KEY = "type";
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    bool ExternalIdText::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = ExternalText::save(root, node);

        if (success)
        {
            JSON_MAKE_MAP_NODE(id_node);

            success = db_id.save(root, id_node);

            if (success)
            {
                success = json::add_static_key_value(
                    DB_ID_KEY,
                    id_node,
                    node,
                    root) and success;

                success = json::add_static_key_value(
                    DB_ID_NAME_KEY,
                    db_id_name,
                    node,
                    root) and success;

                success = json::add_static_key_static_value(
                    DB_ID_TYPE_KEY,
                    ExternalTextConverter::id_type_to_string(db_id_type),
                    node,
                    root) and success;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ExternalIdText::restore(const json::JSONNode &node)
    {
        bool success = ExternalText::restore(node);

        if (success)
        {
            const json::JSONNode *id_node = 0;
            std::string id_type_string;

            success = json::get_key_value(DB_ID_KEY, node, id_node) and success;

            if (success)
            {
                success = db_id.restore(*id_node) and success;
            }

            success = json::get_key_value(DB_ID_NAME_KEY, node, db_id_name)
                and success;

            success = json::get_key_value(DB_ID_TYPE_KEY, node, id_type_string)
                      and success;

            if (not id_type_string.empty())
            {
                db_id_type = ExternalTextConverter::string_to_id_type(id_type_string);

                success = success and (db_id_type != ID_TYPE_END_INVALID);
            }
        }

        return success;
    }
}
}
