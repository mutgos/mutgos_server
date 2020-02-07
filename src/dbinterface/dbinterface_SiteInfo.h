/*
 * dbinterface_SiteInfo.h
 */

#ifndef MUTGOS_DBINTERFACE_SITEINFO_H
#define MUTGOS_DBINTERFACE_SITEINFO_H

#include <string>

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * Container class to hold cached info about a site.
     */
    class SiteInfo
    {
    public:
        /**
         * Default constructor used by STL only.
         */
        SiteInfo(void)
            : site_id(0)
        {
        }

        /**
         * Constructor.
         * @param id[in] The site ID this info is about.
         */
        SiteInfo(const dbtype::Id::SiteIdType id)
          : site_id(id)
        {
        }

        /**
         * Destructor.
         */
        ~SiteInfo()
        {
        }

        /**
         * @return The site ID this info is about.
         */
        dbtype::Id::SiteIdType get_site_id(void) const
        {
            return site_id;
        }

        /**
         * @return The name of the site.
         */
        const std::string &get_site_name(void) const
        {
            return site_name;
        }

        /**
         * Sets the site's name.
         * @param name[in] The name of the site.
         */
        void set_site_name(const std::string &name)
        {
            site_name = name;
        }

        /**
         * @return The description of the site.
         */
        const std::string &get_site_description(void) const
        {
            return site_description;
        }

        /**
         * Sets the site's description.
         * @param description[in] The description of the site.
         */
        void set_site_description(const std::string &description)
        {
            site_description = description;
        }

    private:
        dbtype::Id::SiteIdType site_id; ///< Site ID
        std::string site_name; ///< Name of the site
        std::string site_description; ///< Site description
    };
}
}

#endif //MUTGOS_DBINTERFACE_SITEINFO_H
