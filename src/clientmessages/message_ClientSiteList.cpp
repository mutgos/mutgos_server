/*
 * message_ClientSiteList.cpp
 */

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "clientmessages/message_ClientSiteList.h"
#include "utilities/json_JsonUtilities.h"

namespace
{
    const static std::string SITES_KEY = "sites";
    const static std::string SITE_ID_KEY = "id";
    const static std::string SITE_NAME_KEY = "name";
    const static std::string SITE_DESCRIPTION_KEY = "description";
    const static std::string SITE_ONLINE_COUNT_KEY = "onlineCount";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientSiteList::ClientSiteList(void)
      : ClientMessage(CLIENTMESSAGE_SITE_LIST)
    {
    }

    // ----------------------------------------------------------------------
    ClientSiteList::ClientSiteList(const ClientSiteList &rhs)
      : ClientMessage(rhs),
        sites(rhs.sites)
    {
    }

    // ----------------------------------------------------------------------
    ClientSiteList::~ClientSiteList()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientSiteList::make_instance(void)
    {
        return new ClientSiteList();
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientSiteList::clone(void) const
    {
        return new ClientSiteList(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientSiteList::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        JSON_MAKE_ARRAY_NODE(sites_array);

        // Add each site info to array
        //
        for (Sites::const_iterator sites_iter = sites.begin();
            sites_iter != sites.end();
            ++sites_iter)
        {
            JSON_MAKE_MAP_NODE(site_info);

            success = json::add_static_key_value(
                SITE_ID_KEY,
                sites_iter->site_id,
                site_info,
                root) and success;

            success = json::add_static_key_value(
                SITE_NAME_KEY,
                sites_iter->site_name,
                site_info,
                root) and success;

            success = json::add_static_key_value(
                SITE_DESCRIPTION_KEY,
                sites_iter->site_description,
                site_info,
                root) and success;

            success = json::add_static_key_value(
                SITE_ONLINE_COUNT_KEY,
                sites_iter->site_online_count,
                site_info,
                root) and success;

            success = json::array_add_value(site_info, sites_array, root)
                      and success;
        }

        success = json::add_static_key_value(
            SITES_KEY,
            sites_array,
            node,
            root) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientSiteList::restore(const json::JSONNode &node)
    {
        return false;
    }

    // ----------------------------------------------------------------------
    void ClientSiteList::add_site(
        const dbtype::Id::SiteIdType id,
        const std::string &name,
        const std::string &description,
        const MG_UnsignedInt online_count)
    {
        sites.push_back(SiteInfo());

        SiteInfo &info = sites.back();

        info.site_id = id;
        info.site_name = name;
        info.site_description = description;
        info.site_online_count = online_count;
    }
}
}
