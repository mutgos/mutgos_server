#ifndef MUTGOS_DBINTERFACE_DATABASE_ACCESS_H
#define MUTGOS_DBINTERFACE_DATABASE_ACCESS_H

#include <map>
#include <set>
#include <vector>

#include "osinterface/osinterface_TimeJumpListener.h"

#include "dbtypes/dbtype_Id.h"
#include "dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DbBackend.h"
#include "sqliteinterface/sqliteinterface_SqliteBackend.h"
#include "dbinterface/dbinterface_SiteCache.h"
#include "dbinterface/dbinterface_DatabaseEntityListener.h"
#include "dbinterface/dbinterface_SiteInfo.h"

#include "dbinterface_DbResultCode.h"

#include <boost/thread/mutex.hpp>

namespace mutgos
{
namespace dbinterface
{
    // TODO  Entity delete needs to be re-thought out.
    // TODO  Make sure site deletion is thread safe.

    // TODO  Need to have a 'get name' function, that can get an Entity's name without loading the entity (or using cache if loaded)

    // TODO At some point may need a 'get type' method that returns parent types, like 'group' for capability AND group.

    /**
     * This is the main interface class that other namespaces should use to
     * access and modify stuff from the database (Entities, searches, sites,
     * etc).  Generally, it will instantiate the DB backend, UpdateManager,
     * and any other classes needed to manage the database.
     */
    class DatabaseAccess : public osinterface::TimeJumpListener
    {
    public:
        typedef std::vector<SiteInfo> SiteInfoVector;

        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static DatabaseAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static DatabaseAccess *instance(void)
          { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         */
        void shutdown(void);

        /**
         * Called when a massive (more than a few seconds) system time jump has
         * been detected.
         * @param backwards[in] True if the jump was backwards.
         */
        virtual void os_time_has_jumped(bool backwards);

        /**
         * Adds a DatabaseEntityListener.
         * This must be done prior to any Entity operations being called on
         * this class (in other words, during mutgos startup).
         * Duplicates will be checked for and ignored.
         * @param listener_ptr[in] The listener pointer.  Control of the
         * pointer will NOT pass to this class, but the pointer cannot be
         * deleted unless it is first removed as a listener.
         */
        static void add_entity_listener(
            DatabaseEntityListener * const listener_ptr);

        /**
         * Removes a DatabaseEntityListener.
         * This must be done prior to any Entity operations being called on
         * this class (in other words, during mutgos startup).
         * @param listener_ptr[in] The listener pointer to remove.
         */
        static void remove_entity_listener(
            DatabaseEntityListener * const listener_ptr);

        /**
         * Gets the Entity for the given ID from the database and returns it.
         * NOTE: There is no 'update entity' method because the entity
         * returned can be updated in place.
         * NOTE: This will not return any Entities that are marked as deleted.
         * @param id[in] The ID of the Entity to get.
         * @return The reference to the Entity, or an EntityRef that is not
         * valid if the ID is defaulted, not found, marked as deleted, or has
         * an error.
         */
        EntityRef get_entity(const dbtype::Id &id);

        /**
         * Gets the Entity for the given ID from the database and returns it,
         * even if the Entity is marked for deletion.
         * NOTE: There is no 'update entity' method because the entity
         * returned can be updated in place.
         * NOTE: This is normally not used outside of the database subsystem.
         * Most should be using get_entity().
         * @param id[in] The ID of the Entity to get.
         * @return The reference to the Entity, or an EntityRef that is not
         * valid if the ID is defaulted, not found, or has an error.
         */
        EntityRef get_entity_deleted(const dbtype::Id &id);

        /**
         * Determines if an Entity with the given ID exists at all, even
         * if deleted.
         * @param id[in] The ID to check.
         * @return True if exists, false if not.
         */
        bool entity_exists(const dbtype::Id &id);

        /**
         * Creates a new Entity of the given type (version 0).
         * @param type[in] The type of Entity to create.
         * @param site_id[in] The valid Site ID the Entity is associated with.
         * @param owner[in] The owner of the new Entity.
         * @param name[in] The name of the new Entity.
         * @param entity_ref[out] The reference to the newly created entity
         * if success, or invalid if failure.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_ENTITY_TYPE,
         * DBRESULTCODE_BAD_SITE_ID,
         * DBRESULTCODE_BAD_OWNER,
         * DBRESULTCODE_BAD_NAME,
         * DBRESULTCODE_ERROR
         */
        DbResultCode new_entity(
            const dbtype::EntityType type,
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Id &owner,
            const std::string &name,
            EntityRef &entity_ref);

        /**
         * Deletes the given Entity from the database.  If the Entity is
         * currently in use/referenced, the delete will be delayed until it
         * is no longer in use.
         * @param id[in] The ID of the Entity to delete.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_OK_DELAYED,
         * DBRESULTCODE_BAD_SITE_ID,
         * DBRESULTCODE_BAD_ENTITY_ID,
         * DBRESULTCODE_BAD_ID,
         * DBRESULTCODE_ERROR
         */
        DbResultCode delete_entity(const dbtype::Id &id);

        /**
         * Determines the final type of an Entity.  Deleted entities are
         * included in this query.
         * @param id[in] The ID of the Entity to check.
         * @return The type of the entity, or 'invalid' if not found.
         */
        dbtype::EntityType get_entity_type(const dbtype::Id &id);

        /**
         * Searches for entities of the given type in the given site ID that
         * contain the given string somewhere in their name, or an exact
         * if specified.
         * @param site_id[in] The site to search within.
         * @param type[in] The type of entity to search for.
         * @param name[in] The partial match of the name to look for.  Must not
         * be empty.
         * @param exact[in] Optional (default false).  If true, match name
         * exactly.  Note you may still get multiple matches depending on the
         * type.
         * @return The matching IDs, or empty if none or error.
         */
        dbtype::Entity::IdVector find(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::EntityType type,
            const std::string &name,
            const bool exact = false);

        /**
         * Searches for entities of any type in the given site ID that
         * contain the given string somewhere in their name.
         * @param site_id[in] The site to search within.
         * @param name[in] The partial match of the name to look for.  Must not
         * be empty.
         * @return The matching IDs, or empty if none or error.
         */
        dbtype::Entity::IdVector find(
            const dbtype::Id::SiteIdType site_id,
            const std::string &name);

        /**
         * @param site_id[in] The site ID to get all IDs for.
         * @return All valid Entity IDs for the given site, or empty if none
         * or site doesn't exist.
         */
        dbtype::Entity::IdVector find(
            const dbtype::Id::SiteIdType site_id);

        /**
         * Tries to find a program with the given registration name
         * in the given site.
         * @param site_id[in] The ID of the site to check.
         * @param regname[in] The program registration name to look for.
         * @param prog_id[out] The ID of the found program, or default/invalid
         * if not found.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode find_program_by_reg_name(
            const dbtype::Id::SiteIdType site_id,
            const std::string &regname,
            dbtype::Id &prog_id);

        /**
         * @return A list of all known site IDs in the database.
         */
        dbtype::Id::SiteIdVector get_all_site_ids(void);

        /**
         * @return A copy of the information about all known sites.
         */
        SiteInfoVector get_all_site_info(void);

        /**
         * Gets the name for a site.
         * @param site_id[in] The site ID to get the name for.
         * @param site_name[out] The site name, or empty if error or none.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_ERROR,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode get_site_name(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_name);

        /**
         * Sets the name for a site.
         * @param site_id[in] The site ID to set the name for.
         * @param site_name[in] The site's new name.  The name must not
         * already be in use.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_ERROR,
         * DBRESULTCODE_BAD_SITE_ID,
         * DBRESULTCODE_BAD_NAME
         */
        DbResultCode set_site_name(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_name);

        /**
         * Gets the description for a site.
         * @param site_id[in] The site ID to get the name for.
         * @param site_description[out] The site description, or empty if
         * error or none.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_ERROR,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode get_site_description(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_description);

        /**
         * Sets the description for a site.
         * @param site_id[in] The site ID to set the name for.
         * @param site_description[in] The site's new description.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_ERROR,
         * DBRESULTCODE_BAD_SITE_ID,
         * DBRESULTCODE_BAD_NAME
         */
        DbResultCode set_site_description(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_description);

        /**
         * Creates a new site in the database.
         * @param site_id[out] The ID of the site that was created, if success.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_ERROR
         */
        DbResultCode new_site(dbtype::Id::SiteIdType &site_id);

        /**
         * Deletes a site and all its entities in the database.  The site ID
         * will then be available for reuse.
         * If the site has Entities currently being referenced in memory,
         * the deletion will be delayed until all entities are no longer
         * referenced.
         * @param site_id[in] The site ID to delete.
         * @return The status code.  Can return
         * DBRESULTCODE_OK,
         * DBRESULTCODE_OK_DELAYED,
         * DBRESULTCODE_BAD_SITE_ID,
         * DBRESULTCODE_ERROR
         */
        DbResultCode delete_site(const dbtype::Id::SiteIdType site_id);

        /**
         * ** Internal namespace use only **
         * Commits an Entity's changes to the actual database backend.
         * @param entity[in] The Entity to commit.
         * @return True if success.
         */
        bool internal_commit_entity(EntityRef entity);

        /**
         * ** Internal namespace use only **
         * Deletes an Entity from its cache and the actual database backend,
         * but only if the Entity's in-memory instance is not being referenced.
         * @param entity_id[in] The entity ID to delete.
         * @return The status code.  Can return:
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_ID,
         * DBRESULTCODE_ERROR_ENTITY_IN_USE
         */
        DbResultCode internal_delete_entity(const dbtype::Id &entity_id);

        /**
         * ** Internal namespace use only **
         * Finds a program's ID by regname.  Checks the database only;
         * renames in progress will not be found.
         * @param site_id[in] The site ID to check.
         * @param regname[in] The registration name to look for.
         * @param prog_id[out] The ID of the found program.  If the
         * regname is not found, this will be set to default/invalid.
         * @return The status code.  Can return:
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode internal_get_prog_by_regname(
            const dbtype::Id::SiteIdType &site_id,
            const std::string &regname,
            dbtype::Id &prog_id);

        /**
         * ** Internal namespace use only **
         * Finds a player's ID by exact name.  Checks the database only;
         * renames in progress will not be found.
         * @param site_id[in] The site ID to check.
         * @param name[in] The player name to look for exactly.
         * @param player_id[out] The ID of the found player.  If the
         * name is not found, this will be set to default/invalid.
         * @return The status code.  Can return:
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode internal_get_player_by_name(
            const dbtype::Id::SiteIdType &site_id,
            const std::string &name,
            dbtype::Id &player_id);

        /**
         * ** Internal namespace use only **
         * Finds a program's regname by ID.  Checks the database only;
         * renames in progress will not be found.
         * @param prog_id[in] The ID of the program whose registration name
         * is to be looked up.
         * @param regname[out] The found registration name.  Will be
         * empty if program doesn't exist, doesn't have a registration, or
         * there is an error.
         * @return The status code.  Can return:
         * DBRESULTCODE_OK,
         * DBRESULTCODE_BAD_SITE_ID
         */
        DbResultCode internal_get_prog_regname_by_id(
            const dbtype::Id &prog_id,
            std::string &regname);

    private:
        /**
         * Private singleton constructor.
         */
        DatabaseAccess(void);

        /**
         * Private singleton destructor.
         */
        ~DatabaseAccess();

        /**
         * Finds all the Entities 'under' this one and returns them.
         * Entities that are 'delete pending' are excluded.
         * @param root_entity_id[in] The 'root' Entity to start the search at.
         * This Entity will be included in the output.
         * @param entities[out] All Entities 'under' the root Entity, including
         * the root Entity.
         * @return True if root_entity_id is valid.
         */
        void get_entities_contained_by(
            const dbtype::Id &root_entity_id,
            dbtype::Entity::IdSet &entities);

        /**
         * Gets the existing or makes a site cache for the given site.
         * Thread safe.
         * @param site_id[in] The site ID of the cache to retrieve.
         * @param include_delete_pending[in] If true, will return a cache
         * even if the delete is pending.
         * @return The site entity cache or null if error.
         */
        SiteCache *get_site_cache(
            const dbtype::Id::SiteIdType site_id,
            const bool include_delete_pending = false);

        /**
         * Retrieves information from the DB backend about a site
         * that is known to exist, and puts its info in the site info cache.
         * Assumes locking has already been done.
         * @param site_id[in] The site ID to get info for.
         */
        void add_site_info_to_cache(const dbtype::Id::SiteIdType site_id);

        typedef std::map<dbtype::Id::SiteIdType, SiteCache *> CacheMap;
        typedef std::map<dbtype::Id::SiteIdType, SiteInfo> SiteIdToInfo;
        typedef std::vector<DatabaseEntityListener *> EntityListenerList;

        static DatabaseAccess *singleton_ptr; ///< Singleton pointer.
        static EntityListenerList entity_listeners; ///< List of Entity listeners
        DbBackend *db_backend_ptr; ///< Pointer to database backend.
        CacheMap entity_cache; ///< Cache of entities, organized by site.
        SiteIdToInfo site_id_to_info_cache; ///< All known existing site IDs and their info
        unsigned short int player_name_ser; ///< Looping serial number for temporary player creation name
        boost::mutex mutex; ///< Enforces single access at a time.
    };
}
}

#endif //MUTGOS_DBINTERFACE_DATABASE_ACCESS_H
