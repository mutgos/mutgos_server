/*
 * message_ClientSiteList.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTSITELIST_H
#define MUTGOS_MESSAGE_CLIENTSITELIST_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Lists all the sites known to this server and basic information.
     * This is an outgoing message only; for now no getters will be added
     * since they wouldn't be used.
     */
    class ClientSiteList : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientSiteList(void);

        /**
         * Copy constructor.
         * @param ser_id[in] The source to copy from.
         */
        ClientSiteList(const ClientSiteList &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientSiteList();

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Currently this message is only sent to the client, never received;
         * restore is not currently implemented.
         * Restores this message from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return False since restore is not currently permitted.
         */
        virtual bool restore(const json::JSONNode &node);

        /**
         * Adds a site's info to the list.
         * @param id[in] Site ID of the site.
         * @param name[in] Name of the site.
         * @param description[in] Description of the site.
         * @param online_count[in] How many are online at the site.
         */
        void add_site(
            const dbtype::Id::SiteIdType id,
            const std::string &name,
            const std::string &description,
            const MG_UnsignedInt online_count);

    private:
        /**
         * Container class holding info about a site
         */
        class SiteInfo
        {
        public:
            /**
             * Constructor to default attributes
             */
            SiteInfo(void)
                : site_id(0),
                  site_online_count(0)
            { }

            dbtype::Id::SiteIdType site_id;
            std::string site_name;
            std::string site_description;
            MG_UnsignedInt site_online_count;
        };

        typedef std::vector<SiteInfo> Sites;

        Sites sites; ///< List of all known sites and their info.
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTSITELIST_H
