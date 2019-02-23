/*
 * dbtype_id.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "dbtype_Id.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    const static std::string SITE_ID_KEY = "siteId";
    const static std::string ENTITY_ID_KEY = "entityId";
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    std::string Id::to_string(const bool include_site) const
    {
        std::ostringstream strstream;

        strstream << "#";

        if (include_site)
        {
            strstream << site_id << "-";
        }

        strstream << entity_id;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Id::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = true;

        success =
            json::add_static_key_value(SITE_ID_KEY, site_id, node, root);
        success =
            json::add_static_key_value(ENTITY_ID_KEY, entity_id, node, root)
            and success;

        return success;
    }

    // ----------------------------------------------------------------------
    bool Id::restore(const json::JSONNode &node)
    {
        bool success = false;

        success = json::get_key_value(SITE_ID_KEY, node, site_id);
        success = json::get_key_value(ENTITY_ID_KEY, node, entity_id)
                  and success;

        return success;
    }

} /* namespace dbtype */
} /* namespace mutgos */
