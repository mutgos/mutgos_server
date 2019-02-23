#ifndef MUTGOS_DBINTERFACE_SITE_CACHE_H
#define MUTGOS_DBINTERFACE_SITE_CACHE_H

#include <map>

#include <boost/thread/mutex.hpp>

#include "dbtypes/dbtype_Id.h"

#include "dbinterface_CachedEntity.h"
#include "dbinterface_DbBackend.h"
#include "dbinterface_DbResultCode.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * This class manages the cache for a specific site.  Right now the cache
     * is very simple - it doesn't evict old Entities.  This is an area
     * that will need significant work post-demo.
     */
    class SiteCache
    {
    public:
        /**
         * Constructs a SiteCache.  Provide the database and site ID.
         * @param db_backend[in] The database to get or set data from.  Must not
         * be null.
         * @param site[in] The site ID this SiteCache is for.
         */
        SiteCache(DbBackend *db_backend, const dbtype::Id::SiteIdType site);

        /**
         * Destructor.
         */
        ~SiteCache();

        /**
         * @return The site ID this SiteCache is handling.
         */
        dbtype::Id::SiteIdType get_site_id(void) const;

        /**
         * Sets the delete pending flag, which indicates this site will be
         * deleted from the database when it is no longer being referenced in
         * memory.
         */
        void set_delete_pending(void);

        /**
         * @return True if site is to be deleted.
         * @see set_delete_pending(void);
         */
        bool is_delete_pending(void);

        /**
         * Gets a reference to an Entity, loading it from the database if
         * needed.  If cached, return the cached version.
         * @param id[in] The ID of the Entity to get.
         * @param ref[out] Sets the EntityRef to reference the desired Entity,
         * if found.  If not, makes it invalid.
         * @return The status code.  Can return:
         * DBRESULTCODE_OK
         * DBRESULTCODE_BAD_SITE_ID
         * DBRESULTCODE_BAD_ENTITY_ID
         * DBRESULTCODE_DATABASE_ERROR
         */
        DbResultCode get_entity_ref(const dbtype::Id &id, EntityRef &ref);

        /**
         * Removes the given Entity from the cache.
         * @param id[in] The ID of the Entity to remove from the cache.
         * @return True if removed or not found, false if cannot be removed
         * because it is dirty or in use.
         */
        bool delete_entity_cache(const dbtype::Id &id);

        /**
         * This is expensive.
         * @return True if any Entity in this site is being referenced in
         * memory.
         */
        bool is_anything_referenced(void);

    private:
        typedef std::map<dbtype::Id::EntityIdType, CachedEntity *> EntityCacheMap;

        DbBackend *db_backend_ptr; ///< Database backend so we can load Entities
        const dbtype::Id::SiteIdType site_id; ///< Site ID this cache manages
        boost::mutex mutex; ///< Enforces single access at a time.
        bool delete_pending; ///< True if Site scheduled to be deleted from the database
        EntityCacheMap cached_entities; ///< The entity cache, lookup by Entity ID.
    };
}
}

#endif //MUTGOS_DBINTERFACE_SITE_CACHE_H
