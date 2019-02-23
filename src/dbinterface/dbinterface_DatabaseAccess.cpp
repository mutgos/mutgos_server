#include <deque>

#include "dbinterface_DatabaseAccess.h"
#include "dbinterface_UpdateManager.h"
#include "sqliteinterface/sqliteinterface_SqliteBackend.h"

#include "dbtypes/dbtype_Entity.h"

#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_ReaderLockToken.h"

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace dbinterface
{
    // Statics
    //
    DatabaseAccess *DatabaseAccess::singleton_ptr = 0;
    DatabaseAccess::EntityListenerList DatabaseAccess::entity_listeners;

    // TODO Finds need to check cached items before checking database, since the DB is not always up to date yet.


    // ----------------------------------------------------------------------
    DatabaseAccess* DatabaseAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new DatabaseAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void DatabaseAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool DatabaseAccess::startup(void)
    {
        LOG(info, "dbinterface", "startup", "Starting up...");

        bool success = true;

        if (not db_backend_ptr)
        {
            db_backend_ptr = new sqliteinterface::SqliteBackend();

            success = db_backend_ptr->init();

            UpdateManager::make_singleton();

            if (success)
            {
                UpdateManager::instance()->startup();

                const dbtype::Id::SiteIdVector site_ids =
                    db_backend_ptr->get_site_ids_in_db();

                valid_site_ids.insert(site_ids.begin(), site_ids.end());
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void DatabaseAccess::shutdown(void)
    {
        LOG(info, "dbinterface", "shutdown", "Shutting down...");

        if (UpdateManager::instance())
        {
            UpdateManager::instance()->shutdown();
        }

        // Everything has been written out to the database, so it is safe
        // to clear the cache and shut down the database.
        //
        for (CacheMap::iterator cache_iter = entity_cache.begin();
             cache_iter != entity_cache.end();
             ++cache_iter)
        {
            delete cache_iter->second;
        }

        entity_cache.clear();

        if (db_backend_ptr)
        {
            db_backend_ptr->shutdown();
            delete db_backend_ptr;
            db_backend_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void DatabaseAccess::add_entity_listener(
        DatabaseEntityListener * const listener_ptr)
    {
        if (listener_ptr)
        {
            // Check for duplicates.
            //
            bool dupe_found = false;

            for (EntityListenerList::iterator iter = entity_listeners.begin();
                iter != entity_listeners.end();
                ++iter)
            {
                if (*iter == listener_ptr)
                {
                    dupe_found = true;
                    break;
                }
            }

            if (not dupe_found)
            {
                entity_listeners.push_back(listener_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    void DatabaseAccess::remove_entity_listener(
        DatabaseEntityListener *const listener_ptr)
    {
        if (listener_ptr)
        {
            // Try to find and remove the listener.
            //
            for (EntityListenerList::iterator iter = entity_listeners.begin();
                 iter != entity_listeners.end();
                 ++iter)
            {
                if (*iter == listener_ptr)
                {
                    entity_listeners.erase(iter);
                    break;
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    EntityRef DatabaseAccess::get_entity(const dbtype::Id &id)
    {
        EntityRef ref = get_entity_deleted(id);

        if (ref.valid() and ref.is_delete_pending())
        {
            // Entity is in the process of being deleted.  Filter it out.
            ref.clear();
        }

        return ref;
    }

    // ----------------------------------------------------------------------
    EntityRef DatabaseAccess::get_entity_deleted(const dbtype::Id &id)
    {
        EntityRef ref;

        if (not id.is_default())
        {
            SiteCache *cache_ptr = get_site_cache(id.get_site_id());

            if (not cache_ptr)
            {
                LOG(error, "dbinterface", "get_entity_deleted",
                    "Could not get site cache for id " + id.to_string(true));
            }
            else
            {
                const DbResultCode rc = cache_ptr->get_entity_ref(id, ref);

                switch (rc)
                {
                    case DBRESULTCODE_OK:
                    case DBRESULTCODE_BAD_ENTITY_ID:
                    {
                        // These are OK and can be ignored.
                        break;
                    }

                    default:
                    {
                        LOG(error, "dbinterface", "get_entity_deleted",
                            "Could not get entity ref for id "
                            + id.to_string(true)
                            + " due to error "
                            + db_result_code_to_string(rc));
                        break;
                    }
                }
            }
        }

        return ref;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::new_entity(
        const dbtype::EntityType type,
        const dbtype::Id::SiteIdType site_id,
        const dbtype::Id &owner,
        const std::string &name,
        EntityRef &entity_ref)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        entity_ref.clear();

        if (owner.is_default())
        {
            rc = DBRESULTCODE_BAD_OWNER;
        }
        else if (name.empty())
        {
            rc = DBRESULTCODE_BAD_NAME;
        }

        switch (type)
        {
            case dbtype::ENTITYTYPE_region:
            case dbtype::ENTITYTYPE_room:
            case dbtype::ENTITYTYPE_player:
            case dbtype::ENTITYTYPE_guest:
            case dbtype::ENTITYTYPE_thing:
            case dbtype::ENTITYTYPE_puppet:
            case dbtype::ENTITYTYPE_vehicle:
            case dbtype::ENTITYTYPE_group:
            case dbtype::ENTITYTYPE_capability:
            case dbtype::ENTITYTYPE_program:
            case dbtype::ENTITYTYPE_exit:
            case dbtype::ENTITYTYPE_command:
            {
                // Valid.
                break;
            }

            default:
            {
                // Anything else is not valid
                rc = DBRESULTCODE_BAD_ENTITY_TYPE;
            }
        }

        if (rc == DBRESULTCODE_OK)
        {
            SiteCache *cache_ptr = get_site_cache(site_id);

            if (not cache_ptr)
            {
                LOG(error, "dbinterface", "new_entity",
                    "Could not get site cache for site id "
                    + text::to_string(site_id));

                rc = DBRESULTCODE_BAD_SITE_ID;
            }
            else
            {
                dbtype::Entity *entity_ptr = 0;

                if (type == dbtype::ENTITYTYPE_player)
                {
                    // Can only create new players if the name is not already
                    // in use.
                    //
                    boost::lock_guard<boost::mutex> guard(mutex);
                    dbtype::Entity::IdVector find_result =
                        find(site_id, type, name, true);

                    if (not find_result.empty())
                    {
                        // Name already in use.
                        rc = DBRESULTCODE_BAD_NAME;
                    }
                    else
                    {
                        // Good name, create.  Code is duplicated here so
                        // we remain under the mutex.
                        //
                        entity_ptr = db_backend_ptr->
                            new_entity(type, site_id, owner, name);

                        if (not entity_ptr)
                        {
                            rc = DBRESULTCODE_ERROR;
                        }
                    }
                }
                else
                {
                    entity_ptr =
                        db_backend_ptr->new_entity(type, site_id, owner, name);

                    if (not entity_ptr)
                    {
                        rc = DBRESULTCODE_ERROR;
                    }
                }

                if (entity_ptr)
                {
                    // Created Entity.  Retrieve it again using the proper
                    // channels so it's cached and provided as a ref.
                    entity_ref = get_entity(entity_ptr->get_entity_id());

                    if (not entity_listeners.empty())
                    {
                        // Call listeners
                        //
                        for (EntityListenerList::iterator iter =
                                entity_listeners.begin();
                             iter != entity_listeners.end();
                             ++iter)
                        {
                            (*iter)->entity_created(entity_ref.get());
                        }
                    }

                    delete entity_ptr;
                    entity_ptr = 0;
                }
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::delete_entity(const dbtype::Id &id)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        UpdateManager *update_ptr = UpdateManager::instance();

        if (id.is_default())
        {
            rc = DBRESULTCODE_BAD_ID;
        }
        else if (update_ptr->is_entity_delete_pending(id))
        {
            // Already processed by a previous call
            rc = DBRESULTCODE_OK_DELAYED;
        }
        else
        {
            SiteCache *cache_ptr = get_site_cache(id.get_site_id());

            if (not cache_ptr)
            {
                rc = DBRESULTCODE_BAD_SITE_ID;
            }
            else
            {
                // Candidate for deletion.  Get all Entities contained by this
                // one to mark them for deletion, and delete them too.
                dbtype::Entity::IdSet entities;
                get_entities_contained_by(id, entities);

                for (dbtype::Entity::IdSet::iterator entities_iter =
                        entities.begin();
                    entities_iter != entities.end();
                    ++entities_iter)
                {
                    EntityRef entity = get_entity(*entities_iter);

                    if (entity.valid())
                    {
                        entity.get()->set_deleted_flag(true);

                        if (not entity_listeners.empty())
                        {
                            // Call listeners
                            //
                            for (EntityListenerList::iterator iter =
                                entity_listeners.begin();
                                 iter != entity_listeners.end();
                                 ++iter)
                            {
                                (*iter)->entity_deleted(entity.get());
                            }
                        }
                    }
                }

                update_ptr->entities_deleted(entities);
                rc = DBRESULTCODE_OK_DELAYED;
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    dbtype::EntityType DatabaseAccess::get_entity_type(const dbtype::Id &id)
    {
        dbtype::EntityType type = dbtype::ENTITYTYPE_invalid;

        if (not id.is_default())
        {
            SiteCache *cache_ptr = get_site_cache(id.get_site_id());

            if (not cache_ptr)
            {
                LOG(error, "dbinterface", "get_entity_type",
                    "Could not get site cache for id " + id.to_string(true));
            }
            else
            {
                EntityRef ref;
                cache_ptr->get_entity_ref(id, ref);

                if (ref.valid())
                {
                    // The entity is cached.  Use that instead of going to the
                    // database.
                    type = ref->get_entity_type();
                }
                else
                {
                    // Entity is not cached; use the database.
                    type = db_backend_ptr->get_entity_type_db(id);
                }
            }
        }

        return type;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector DatabaseAccess::find(
        const dbtype::Id::SiteIdType site_id,
        const dbtype::EntityType type,
        const std::string &name,
        const bool exact)
    {
        return db_backend_ptr->find_in_db(site_id, type, name, exact);
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector DatabaseAccess::find(
        const dbtype::Id::SiteIdType site_id,
        const std::string &name)
    {
        return db_backend_ptr->find_in_db(site_id, name);
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector DatabaseAccess::find(
        const dbtype::Id::SiteIdType site_id)
    {
        return db_backend_ptr->find_in_db(site_id);
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector DatabaseAccess::get_all_site_ids(void)
    {
        return db_backend_ptr->get_site_ids_in_db();
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::new_site(dbtype::Id::SiteIdType &site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        DbResultCode rc = DBRESULTCODE_OK;

        if (db_backend_ptr->new_site_in_db(site_id))
        {
            valid_site_ids.insert(site_id);
        }
        else
        {
            rc = DBRESULTCODE_ERROR;
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::delete_site(
        const dbtype::Id::SiteIdType site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        DbResultCode rc = DBRESULTCODE_OK;

        SiteCache *cache_ptr = get_site_cache(site_id, true);

        if (not cache_ptr)
        {
            rc = DBRESULTCODE_BAD_SITE_ID;
        }
        else
        {
            cache_ptr->set_delete_pending();

            if (not entity_listeners.empty())
            {
                // Call listeners
                //
                for (EntityListenerList::iterator iter =
                    entity_listeners.begin();
                     iter != entity_listeners.end();
                     ++iter)
                {
                    (*iter)->site_deleted(site_id);
                }
            }

            if (cache_ptr->is_anything_referenced())
            {
                // Some other thread is still using the site, so let the
                // update manager delete it later.
                UpdateManager::instance()->site_deleted(site_id);

                rc = DBRESULTCODE_OK_DELAYED;
            }
            else
            {
                // Nothing is referencing the site, safe to delete immediately.
                //
                delete cache_ptr;
                cache_ptr = 0;
                valid_site_ids.erase(site_id);
                entity_cache.erase(site_id);

                if (not db_backend_ptr->delete_site_in_db(site_id))
                {
                    rc = DBRESULTCODE_ERROR;
                }
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    bool DatabaseAccess::internal_commit_entity(EntityRef entity)
    {
        bool success = false;

        if (db_backend_ptr and entity.valid())
        {
            success = db_backend_ptr->save_entity_db(entity.get());
        }

        return success;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::internal_delete_entity(
        const dbtype::Id entity_id)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        if (entity_id.is_default())
        {
            rc = DBRESULTCODE_BAD_ID;
        }
        else
        {
            SiteCache *cache_ptr = get_site_cache(entity_id.get_site_id());

            if (not cache_ptr)
            {
                rc = DBRESULTCODE_BAD_ID;
            }
            else
            {
                if (not cache_ptr->delete_entity_cache(entity_id))
                {
                    rc = DBRESULTCODE_ERROR_ENTITY_IN_USE;
                }
                else
                {
                    // Successfully removed from cache.  Now try and delete
                    // it from the database.  It could fail if someone sneaks
                    // in and loads it again.
                    if (not db_backend_ptr->delete_entity_db(entity_id))
                    {
                        rc = DBRESULTCODE_ERROR_ENTITY_IN_USE;
                    }
                }
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DatabaseAccess::DatabaseAccess(void)
      : db_backend_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    DatabaseAccess::~DatabaseAccess()
    {
        if (db_backend_ptr)
        {
            shutdown();
        }
    }

    // ----------------------------------------------------------------------
    void DatabaseAccess::get_entities_contained_by(
        const dbtype::Id &root_entity_id,
        dbtype::Entity::IdSet &entities)
    {
        std::deque<dbtype::Id> deletes_to_process;
        dbtype::Entity::IdVector current_references;

        entities.clear();

        // Prime the loop.  This is a non-recursive breadth-first search.
        //
        deletes_to_process.push_back(root_entity_id);

        dbtype::Id current_id;

        while (not deletes_to_process.empty())
        {
            current_id = deletes_to_process.front();
            deletes_to_process.pop_front();

            if (entities.find(current_id) == entities.end())
            {
                EntityRef current_entity = get_entity_deleted(current_id);

                if (current_entity.valid())
                {
                    entities.insert(current_id);

                    // Find everything 'under' this Entity in the hierarchy by
                    // looking at the references.  Append them to the deletes
                    // to process.
                    //
                    concurrency::ReaderLockToken token(*current_entity.get());

                    current_references = current_entity.get()->get_reference_ids(
                        dbtype::ENTITYFIELD_contained_by, token);
                    deletes_to_process.insert(
                        deletes_to_process.end(),
                        current_references.begin(),
                        current_references.end());

                    current_references = current_entity.get()->get_reference_ids(
                        dbtype::ENTITYFIELD_action_contained_by, token);
                    deletes_to_process.insert(
                        deletes_to_process.end(),
                        current_references.begin(),
                        current_references.end());
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    SiteCache *DatabaseAccess::get_site_cache(
        const dbtype::Id::SiteIdType site_id,
        const bool include_delete_pending)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        SiteCache *site_ptr = 0;

        if (valid_site_ids.find(site_id) != valid_site_ids.end())
        {
            CacheMap::iterator cache_iter = entity_cache.find(site_id);

            if (cache_iter == entity_cache.end())
            {
                // Doesn't exist; create.
                //
                entity_cache[site_id] = new SiteCache(db_backend_ptr, site_id);
                cache_iter = entity_cache.find(site_id);
            }

            const bool delete_pending = cache_iter->second->is_delete_pending();

            if ((not delete_pending) or (delete_pending and include_delete_pending))
            {
                site_ptr = cache_iter->second;
            }
        }

        return site_ptr;
    }
}
}
