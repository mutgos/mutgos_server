#include "dbinterface_SiteCache.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "dbtypes/dbtype_Id.h"

#include "dbinterface_CachedEntity.h"
#include "dbinterface_DbBackend.h"
#include "dbinterface_DbResultCode.h"

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace dbinterface
{
    // ----------------------------------------------------------------------
    SiteCache::SiteCache(
        DbBackend *db_backend,
        const dbtype::Id::SiteIdType site)
      : db_backend_ptr(db_backend),
        site_id(site),
        delete_pending(false)
    {
        LOG(debug, "dbinterface", "SiteCache()",
            "Constructing site cache for site ID "
            + text::to_string(site_id));

        if (not db_backend)
        {
            LOG(fatal, "dbinterface", "SiteCache()",
                "Database backend is null!");
        }
    }

    // ----------------------------------------------------------------------
    SiteCache::~SiteCache()
    {
        LOG(debug, "dbinterface", "~SiteCache()",
            "Started destructing site cache for site ID "
            + text::to_string(site_id));

        // Scope for mutex
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            for (EntityCacheMap::iterator iter = cached_entities.begin();
                iter != cached_entities.end();
                ++iter)
            {
                dbtype::Entity * const entity_temp_ptr =
                    iter->second->get_entity();
                // Delete first because it references the Entity pointer
                delete iter->second;
                // All references are now gone.  Delete actual Entity
                db_backend_ptr->delete_entity_mem(entity_temp_ptr);
            }

            cached_entities.clear();
        }

        LOG(debug, "dbinterface", "~SiteCache()",
            "Finished destructing site cache for site ID "
            + text::to_string(site_id));
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdType SiteCache::get_site_id(void) const
    {
        return site_id;
    }

    // ----------------------------------------------------------------------
    void SiteCache::set_delete_pending(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        delete_pending = true;
    }

    // ----------------------------------------------------------------------
    bool SiteCache::is_delete_pending(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        return delete_pending;
    }

    // ----------------------------------------------------------------------
    DbResultCode SiteCache::get_entity_ref(const dbtype::Id &id, EntityRef &ref)
    {
        DbResultCode return_code = DBRESULTCODE_OK;

        ref.clear();

        if (id.get_site_id() != site_id)
        {
            return_code = DBRESULTCODE_BAD_SITE_ID;
        }
        else
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            // See if it's already cached
            //
            EntityCacheMap::iterator find_iter =
                cached_entities.find(id.get_entity_id());

            if (find_iter != cached_entities.end())
            {
                // Found it in the cache
                //
                find_iter->second->get_reference(ref);
            }
            else
            {
                // Not cached, so load it
                //
                dbtype::Entity *entity_ptr = db_backend_ptr->get_entity_db(id);

                if (not entity_ptr)
                {
                    return_code = DBRESULTCODE_BAD_ENTITY_ID;
                }
                else
                {
                    // Make a new cache entry and get a reference
                    //
                    CachedEntity *cached_ptr = new CachedEntity(entity_ptr);
                    cached_ptr->get_reference(ref);
                    cached_entities[id.get_entity_id()] = cached_ptr;

                    entity_ptr->set_entity_accessed_timestamp();
                }
            }
        }

        return return_code;
    }

    // ----------------------------------------------------------------------
    bool SiteCache::delete_entity_cache(const dbtype::Id &id)
    {
        bool deleted = true;

        boost::lock_guard<boost::mutex> guard(mutex);

        EntityCacheMap::iterator find_iter =
            cached_entities.find(id.get_entity_id());

        if (find_iter != cached_entities.end())
        {
            // Found it in the cache.  Check if dirty or in use.
            //
            deleted = (not find_iter->second->is_referenced()) and
                (not find_iter->second->get_entity()->is_dirty());

            if (deleted)
            {
                // Can be deleted.  No one is using it and it's not dirty.
                db_backend_ptr->delete_entity_mem(
                    find_iter->second->get_entity());
                delete find_iter->second;
                cached_entities.erase(find_iter);
            }
        }

        return deleted;
    }

    // ----------------------------------------------------------------------
    bool SiteCache::is_anything_referenced(void)
    {
        bool referenced = false;

        boost::lock_guard<boost::mutex> guard(mutex);

        for (EntityCacheMap::iterator iter = cached_entities.begin();
             iter != cached_entities.end();
             ++iter)
        {
            if (iter->second->is_referenced())
            {
                // Found one that is referenced; we can stop here.
                referenced = true;
                break;
            }
        }

        return referenced;
    }
}
}