#include <deque>

#include "dbinterface_DatabaseAccess.h"
#include "dbinterface_UpdateManager.h"
#include "sqliteinterface/sqliteinterface_SqliteBackend.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbinterface/dbinterface_SiteInfo.h"

#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_ReaderLockToken.h"

#include "text/text_StringConversion.h"
#include "text/text_Utf8Tools.h"

#include "utilities/mutgos_config.h"
#include "logging/log_Logger.h"

namespace
{
    // Always uppercase
    const std::string TEMP_PLAYER_NAME_PREFIX = "PLRCRT-";
}

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

                for (dbtype::Id::SiteIdVector::const_iterator site_iter =
                        site_ids.begin();
                    site_iter != site_ids.end();
                    ++site_iter)
                {
                    add_site_info_to_cache(*site_iter);
                }
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
    void DatabaseAccess::os_time_has_jumped(bool backwards)
    {
        // Just pass this along for now, until an actual listener
        // infrastructure is made.
        UpdateManager::instance()->os_time_has_jumped(backwards);
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
    bool DatabaseAccess::entity_exists(const dbtype::Id &id)
    {
        bool exists = false;

        if (not id.is_default())
        {
            exists = db_backend_ptr->entity_exists_db(id);
        }

        return exists;
    }

    // ----------------------------------------------------------------------
    EntityMetadata DatabaseAccess::get_entity_metadata(const dbtype::Id &id)
    {
        return db_backend_ptr->get_entity_metadata(id);
    }

    // ----------------------------------------------------------------------
    MetadataVector
    DatabaseAccess::get_entity_metadata(const dbtype::Entity::IdVector &ids)
    {
        return db_backend_ptr->get_entity_metadata(ids);
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
                    // Do not allow anyone to use our temporary naming
                    // scheme.
                    //
                    const std::string name_upper = text::to_upper_copy(name);

                    if (name_upper.find(::TEMP_PLAYER_NAME_PREFIX) == 0)
                    {
                        rc = DBRESULTCODE_BAD_NAME;
                        return rc;
                    }

                    // Can only create new players if the name is not already
                    // in use.  Due to how creation works, assign a random
                    // name during the creation process then attempt to
                    // rename once created. If the rename fails (due to
                    // invalid name or already in use), delete the Player
                    // and return an error code.
                    //
                    boost::lock_guard<boost::mutex> guard(mutex);

                    const std::string temp_name = ::TEMP_PLAYER_NAME_PREFIX +
                        text::to_string(player_name_ser++);

                    entity_ptr = db_backend_ptr->
                        new_entity(type, site_id, owner, temp_name);

                    if (not entity_ptr)
                    {
                        rc = DBRESULTCODE_ERROR;
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

                    if (type == dbtype::ENTITYTYPE_player)
                    {
                        // Attempt to set the actual name
                        if (not entity_ref->set_entity_name(name))
                        {
                            // Failed, likely due to name in use.
                            // Delete and exit
                            delete_entity(entity_ptr->get_entity_id());
                            rc = DBRESULTCODE_BAD_NAME;
                            delete entity_ptr;
                            entity_ptr = 0;
                            entity_ref.clear();
                        }
                    }

                    if (entity_ptr and (not entity_listeners.empty()))
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
        const dbtype::Id::EntityIdType owner_id,
        const std::string &name,
        const bool exact)
    {
        dbtype::Entity::IdVector result = db_backend_ptr->find_in_db(
            site_id,
            (type == dbtype::EntityType::ENTITYTYPE_entity ?
                dbtype::EntityType::ENTITYTYPE_invalid : type),
            owner_id,
            name,
            exact);

        // Check for renamed players if we're looking for players,
        // AND if we're not searching exact, OR searching exact
        // but had no results.
        if ((type == dbtype::ENTITYTYPE_player) and
            ((not exact) or (result.empty())))
        {
            dbtype::Entity::IdVector player_result;

            UpdateManager::instance()->get_player_rename_id(
                site_id, name, exact, player_result);

            // If anything in player_result is not in result, add it
            //
            if (not player_result.empty())
            {
                bool found = false;

                for (dbtype::Entity::IdVector::const_iterator player_iter =
                        player_result.begin();
                     player_iter != player_result.end();
                     ++player_iter)
                {
                    found = false;

                    for (dbtype::Entity::IdVector::const_iterator result_iter =
                            result.begin();
                         result_iter != result.end();
                         ++result_iter)
                    {
                        if (*player_iter == *result_iter)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (not found)
                    {
                        result.push_back(*player_iter);
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector DatabaseAccess::find(
        const dbtype::Id::SiteIdType site_id,
        const std::string &name)
    {
        dbtype::Entity::IdVector result =
            db_backend_ptr->find_in_db(
                site_id,
                dbtype::ENTITYTYPE_invalid,
                0,
                name,
                false);
        dbtype::Entity::IdVector player_result;

        UpdateManager::instance()->
            get_player_rename_id(site_id, name, false, player_result);

        // If anything in player_result is not in result, add it
        //
        if (not player_result.empty())
        {
            bool found = false;

            for (dbtype::Entity::IdVector::const_iterator player_iter =
                    player_result.begin();
                 player_iter != player_result.end();
                 ++player_iter)
            {
                found = false;

                for (dbtype::Entity::IdVector::const_iterator result_iter =
                        result.begin();
                     result_iter != result.end();
                     ++result_iter)
                {
                    if (*player_iter == *result_iter)
                    {
                        found = true;
                        break;
                    }
                }

                if (not found)
                {
                    result.push_back(*player_iter);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector DatabaseAccess::find(
        const dbtype::Id::SiteIdType site_id)
    {
        return db_backend_ptr->find_in_db(site_id);
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::find_program_by_reg_name(
        const dbtype::Id::SiteIdType site_id,
        const std::string &regname,
        dbtype::Id &prog_id)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        if (not get_site_cache(site_id))
        {
            rc = DBRESULTCODE_BAD_SITE_ID;
            prog_id = dbtype::Id();
        }
        else
        {
            prog_id = UpdateManager::instance()->
                get_prog_reg_rename_id(site_id, regname);

            if (prog_id.is_default())
            {
                // Didn't find in active renames, check database.
                prog_id =
                    db_backend_ptr->find_program_reg_in_db(site_id, regname);
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector DatabaseAccess::get_all_site_ids(void)
    {
        dbtype::Id::SiteIdVector sites;
        boost::lock_guard<boost::mutex> guard(mutex);

        sites.reserve(site_id_to_info_cache.size());

        for (SiteIdToInfo::const_iterator site_iter =
            site_id_to_info_cache.begin();
            site_iter != site_id_to_info_cache.end();
            ++site_iter)
        {
            sites.push_back(site_iter->first);
        }

        return sites;
    }

    // ----------------------------------------------------------------------
    DatabaseAccess::SiteInfoVector DatabaseAccess::get_all_site_info(void)
    {
        SiteInfoVector result;
        boost::lock_guard<boost::mutex> guard(mutex);

        result.reserve(site_id_to_info_cache.size());

        for (SiteIdToInfo::const_iterator site_iter =
                site_id_to_info_cache.begin();
            site_iter != site_id_to_info_cache.end();
            ++site_iter)
        {
            result.push_back(site_iter->second);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::get_site_name(
        const dbtype::Id::SiteIdType site_id,
        std::string &site_name)
    {
        DbResultCode rc = DBRESULTCODE_BAD_SITE_ID;
        boost::lock_guard<boost::mutex> guard(mutex);
        SiteIdToInfo::const_iterator site_iter =
            site_id_to_info_cache.find(site_id);

        site_name.clear();

        if (site_iter != site_id_to_info_cache.end())
        {
            rc = DBRESULTCODE_OK;
            site_name = site_iter->second.get_site_name();
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::set_site_description(
        const dbtype::Id::SiteIdType site_id,
        const std::string &site_description)
    {
        DbResultCode rc = DBRESULTCODE_BAD_SITE_ID;
        const std::string site_desc_trimmed = text::trim_copy(site_description);

        if (site_desc_trimmed.empty() or
            (text::utf8_size(site_desc_trimmed) > config::db::limits_string_size()))
        {
            rc = DBRESULTCODE_BAD_NAME;
        }
        else
        {
            boost::lock_guard<boost::mutex> guard(mutex);
            SiteIdToInfo::iterator site_iter =
                site_id_to_info_cache.find(site_id);

            if (site_iter != site_id_to_info_cache.end())
            {
                if (db_backend_ptr->set_site_description_in_db(
                    site_id,
                    site_desc_trimmed))
                {
                    rc = DBRESULTCODE_OK;
                    site_iter->second.set_site_description(site_desc_trimmed);
                }
                else
                {
                    rc = DBRESULTCODE_ERROR;
                }
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::get_site_description(
        const dbtype::Id::SiteIdType site_id,
        std::string &site_description)
    {
        DbResultCode rc = DBRESULTCODE_BAD_SITE_ID;
        boost::lock_guard<boost::mutex> guard(mutex);
        SiteIdToInfo::const_iterator site_iter =
            site_id_to_info_cache.find(site_id);

        site_description.clear();

        if (site_iter != site_id_to_info_cache.end())
        {
            rc = DBRESULTCODE_OK;
            site_description = site_iter->second.get_site_description();
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::set_site_name(
        const dbtype::Id::SiteIdType site_id,
        const std::string &site_name)
    {
        DbResultCode rc = DBRESULTCODE_BAD_SITE_ID;
        const std::string site_name_trimmed = text::trim_copy(site_name);

        if (site_name_trimmed.empty() or
            (text::utf8_size(site_name_trimmed) > config::db::limits_entity_name()))
        {
            rc = DBRESULTCODE_BAD_NAME;
        }
        else
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            // Confirm name not in use
            //
            for (SiteIdToInfo::const_iterator site_check_iter =
                    site_id_to_info_cache.begin();
                site_check_iter != site_id_to_info_cache.end();
                ++site_check_iter)
            {
                if (site_check_iter->second.get_site_name() == site_name_trimmed)
                {
                    rc = DBRESULTCODE_BAD_NAME;
                    break;
                }
            }

            if (rc != DBRESULTCODE_BAD_NAME)
            {
                SiteIdToInfo::iterator site_iter =
                    site_id_to_info_cache.find(site_id);

                if (site_iter != site_id_to_info_cache.end())
                {
                    if (db_backend_ptr->set_site_name_in_db(
                        site_id,
                        site_name_trimmed))
                    {
                        rc = DBRESULTCODE_OK;
                        site_iter->second.set_site_name(site_name_trimmed);
                    }
                    else
                    {
                        rc = DBRESULTCODE_ERROR;
                    }
                }
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::new_site(dbtype::Id::SiteIdType &site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        DbResultCode rc = DBRESULTCODE_OK;

        if (db_backend_ptr->new_site_in_db(site_id))
        {
            add_site_info_to_cache(site_id);
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
                site_id_to_info_cache.erase(site_id);
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
        const dbtype::Id &entity_id)
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
    DbResultCode DatabaseAccess::internal_get_prog_by_regname(
        const dbtype::Id::SiteIdType &site_id,
        const std::string &regname,
        dbtype::Id &prog_id)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        if (not get_site_cache(site_id))
        {
            rc = DBRESULTCODE_BAD_SITE_ID;
            prog_id = dbtype::Id();
        }
        else
        {
            prog_id = db_backend_ptr->find_program_reg_in_db(site_id, regname);
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::internal_get_player_by_name(
        const dbtype::Id::SiteIdType &site_id,
        const std::string &name,
        dbtype::Id &player_id)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        if (not get_site_cache(site_id))
        {
            rc = DBRESULTCODE_BAD_SITE_ID;
            player_id = dbtype::Id();
        }
        else
        {
            dbtype::Entity::IdVector result = db_backend_ptr->
                find_in_db(site_id, dbtype::ENTITYTYPE_player, 0, name, true);

            if (result.empty())
            {
                player_id = dbtype::Id();
            }
            else
            {
                player_id = result.front();
            }
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DbResultCode DatabaseAccess::internal_get_prog_regname_by_id(
        const dbtype::Id &prog_id,
        std::string &regname)
    {
        DbResultCode rc = DBRESULTCODE_OK;

        if (not get_site_cache(prog_id.get_site_id()))
        {
            rc = DBRESULTCODE_BAD_SITE_ID;
            regname.clear();
        }
        else
        {
            regname = db_backend_ptr->find_program_reg_name_in_db(prog_id);
        }

        return rc;
    }

    // ----------------------------------------------------------------------
    DatabaseAccess::DatabaseAccess(void)
      : db_backend_ptr(0),
        player_name_ser(0)
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

        if (site_id_to_info_cache.find(site_id) != site_id_to_info_cache.end())
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

    // ----------------------------------------------------------------------
    void DatabaseAccess::add_site_info_to_cache(
        const mutgos::dbtype::Id::SiteIdType site_id)
    {
        std::string site_name;
        std::string site_description;

        if (not db_backend_ptr->get_site_name_in_db(site_id, site_name))
        {
            LOG(error, "dbinterface", "add_site_info_to_cache",
                "Could not get name for site ID "
                + text::to_string(site_id));
        }

        if (not db_backend_ptr->get_site_description_in_db(
            site_id,
            site_description))
        {
            LOG(error, "dbinterface", "add_site_info_to_cache",
                "Could not get description for site ID "
                + text::to_string(site_id));
        }

        SiteInfo site_info(site_id);
        site_info.set_site_name(site_name);
        site_info.set_site_description(site_description);

        site_id_to_info_cache[site_id] = site_info;
    }
}
}
