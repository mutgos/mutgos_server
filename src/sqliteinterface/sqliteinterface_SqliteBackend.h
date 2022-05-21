/*
 * sqliteinterface_sqlitebackend.h
 */

#ifndef MUTGOS_SQLITEINTERFACE_SQLITEBACKEND_H
#define MUTGOS_SQLITEINTERFACE_SQLITEBACKEND_H

#include <sqlite3.h>
#include <boost/thread/mutex.hpp>

#include "dbinterface/dbinterface_DbBackend.h"

#include "concurrency/concurrency_WriterLockToken.h"

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace sqliteinterface
{
    /**
     * Implements a DbBackend that uses SQLite.
     */
    class SqliteBackend : public dbinterface::DbBackend
    {
    public:
        /**
         * Constructor.
         */
        SqliteBackend(void);

        /**
         * Destructor.
         */
        virtual ~SqliteBackend();

        /**
         * Called when most of MUTGOS has been initialized, but before the
         * DbBackend is actually used.
         * @return True if successfully initialized.
         */
        virtual bool init(void);

        /**
         * Informs the DbBackend that it is to be shut down.  All outstanding
         * owned pointers shall be deleted and the database itself shall be
         * flushed and closed.
         * @return True if success.
         */
        virtual bool shutdown(void);

        /**
         * @return The name of this backend.  This should be a string
         * suitable for logging and display and is for informational
         * purposes only.
         */
        virtual std::string get_backend_name(void);

        /**
         * @param entity_ptr[in] A pointer to an Entity.
         * @return True if this pointer was created by this DbBackend.  If
         * true, when Entity is to be deleted from memory, you MUST use
         * delete_entity_mem().
         */
        virtual bool entity_mem_owned_by_this(
            const dbtype::Entity *entity_ptr);

        /**
         * Deletes the given entity from memory, if owned by this DbBackend.
         * The Entity will NOT be deleted from the database.
         * @param entity_ptr[in] The entity pointer to delete from memory.
         */
        virtual void delete_entity_mem(dbtype::Entity *entity_ptr);

        /**
         * Creates a new Entity of the given type (version 0), in memory and the
         * database.
         * Caller must manage pointer and delete it with delete_entity_mem().
         * @param type[in] The type of Entity to create.
         * @param site_id[in] The valid Site ID the Entity is associated with.
         * @param owner[in] The owner of the new Entity.
         * @param name[in] The name of the new Entity.
         * @return The newly created entity as a pointer, or null if error.
         */
        virtual dbtype::Entity *new_entity(
            const dbtype::EntityType type,
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Id &owner,
            const std::string &name);

        /**
         * Gets the Entity from the database.  If the Entity is already
         * present in memory, the existing pointer is returned.
         * Caller must manage pointer and delete it with delete_entity_mem().
         * @param id[in] The ID of the Entity to retrieve.
         * @return A pointer to the Entity retrieved, or null if not found.
         */
        virtual dbtype::Entity *get_entity_db(const dbtype::Id &id);

        /**
         * Determines if the given entity ID exists in the database.
         * @param id[in] The ID to check.
         * @return True if it exists, false if not.
         */
        virtual bool entity_exists_db(const dbtype::Id &id);

        /**
         * Saves the given Entity to the database.  Existing Entity data for
         * that ID and version are overwritten.  The entity must have been
         * previously brought into memory via get_entity_db() or new_entity()
         * and not  have been deleted from memory.
         * The dirty flag on the Entity will be cleared if success.
         * For Programs, this assumes the program registation name is
         * already known to be unique.
         * @param entity_ptr[in] The Entity to save.
         * @return True if success.  Will return false if failed
         * to update program registration name lookup table, but
         * update will otherwise have succeeded.
         */
        virtual bool save_entity_db(dbtype::Entity *entity_ptr);

        /**
         * Deletes the given Entity from the database.  If the Entity is
         * currently in memory, deletion will fail.
         * @param id[in] The ID of the Entity to delete.
         * @return True if success (or not found), false if failure.
         */
        virtual bool delete_entity_db(const dbtype::Id &id);

        /**
         * Deleted entities are included in this query.
         * @param id[in] The ID whose type is to be retrieved.
         * @return The type of the given ID, or 'invalid' if not found.
         */
        virtual dbtype::EntityType get_entity_type_db(const dbtype::Id &id);

        /**
         * Searches for entities using the parameters specified that
         * contain the given string somewhere in their name, or an exact
         * name match if specified.
         * @param site_id[in] The site to search within.
         * @param type[in] The type of entity to search for, or invalid
         * for all types.
         * @param owner_id[in] The ID of the owner, or default for all owners.
         * @param name[in] The name of the Entity to look for.  Can be empty
         * in some situations to search for all names.
         * @param exact[in] If true, match name exactly.  Note you may still
         * get multiple matches depending on the type.  This is ignored
         * when no name given.
         * @return The matching IDs, or empty if none.
         */
        virtual dbtype::Entity::IdVector find_in_db(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::EntityType type,
             const dbtype::Id::EntityIdType owner_id,
            const std::string &name,
            const bool exact);

        /**
         * @param site_id[in] The site ID to get all IDs for.
         * @return All valid Entity IDs for the given site, or empty if none
         * or site doesn't exist.
         */
        virtual dbtype::Entity::IdVector find_in_db(
            const dbtype::Id::SiteIdType site_id);

        /**
         * Searches the given site for the program registration name.
         * @param site_id[in] The site ID to search for the program
         * registration name.
         * @param registration_name[in] The program registration name to
         * find.
         * @return The ID of the Program found, or default/invalid if none
         * found matching or the site is invalid.
         */
        virtual dbtype::Id find_program_reg_in_db(
            const dbtype::Id::SiteIdType site_id,
            const std::string &registration_name);

        /**
         * Searches for the given ID and determines if a registration
         * is associated with it.
         * @param id[in] The ID to search for.
         * @return The registration name of the Program found, or empty if none
         * found matching or the ID is invalid.
         */
        virtual std::string find_program_reg_name_in_db(const dbtype::Id &id);

        /**
         * @return A list of all known site IDs in the database.
         */
        virtual dbtype::Id::SiteIdVector get_site_ids_in_db(void);

        /**
         * Gets the metadata for a single Entity.
         * @param id[in] The ID of the entity to get metadata for.
         * @return The Metadata for the Entity, or invalid if not found.
         */
        virtual dbinterface::EntityMetadata get_entity_metadata(
            const dbtype::Id &id);

        /**
         * Gets the metadata for a group of Entities.  This is more efficient
         * than getting one at a time.
         * @param ids[in] The IDs of the entities to get metadata for.
         * @return The Metadata for the Entities, or empty if not found.
         * If only a few Entities cannot be found, there will simply not be
         * an entry for them.
         */
        virtual dbinterface::MetadataVector get_entity_metadata(
            const dbtype::Entity::IdVector &ids);

        /**
         * Creates a new site in the database.
         * @param site_id[out] The ID of the site that was created, if success.
         * @return True if successfully created the new site (site_id will be
         * populated if so).
         */
        virtual bool new_site_in_db(dbtype::Id::SiteIdType &site_id);

        /**
         * Deletes a site and all its entities in the database.  The site ID
         * will then be available for reuse.
         * @param site_id[in] The site ID to delete.
         * @return True if success, false if the site ID cannot be found.
         */
        virtual bool delete_site_in_db(const dbtype::Id::SiteIdType site_id);

        /**
         * Gets the name for a site.
         * @param site_id[in] The existing site ID to get the name for.
         * @param site_name[out] The site name, or empty if error or none.
         * @return True if successfully retrieved the site name.
         */
        virtual bool get_site_name_in_db(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_name);

        /**
         * Sets the name for a site.
         * @param site_id[in] The existing site ID to set the name for.
         * @param site_name[in] The site's new name.
         * @return True if successfully set the site name.
         */
        virtual bool set_site_name_in_db(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_name);

        /**
         * Gets the description for a site.
         * @param site_id[in] The existing site ID to get the description for.
         * @param site_description[out] The site description, or empty if
         * error or none.
         * @return True if successfully retrieved the site description.
         */
        virtual bool get_site_description_in_db(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_description);

        /**
         * Sets the description for a site.
         * @param site_id[in] The existing site ID to set the description for.
         * @param site_description[in] The site's new description.
         * @return True if successfully set the site description.
         */
        virtual bool set_site_description_in_db(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_description);

    private:

        /**
         * Creates the needed tables in the database if they do not already
         * exist.  Assumes database is already opened.
         * @return True if success.
         */
        bool create_tables(void);

        /**
         * Creates prepared statements, does any other prep work after the
         * database has been opened.
         * @return True if success.
         */
        bool sql_init(void);

        /**
         * Gets the metadata for an Entity.  Assumes locking has already
         * taken place.
         * @param id[in] The ID of the Entity's metadata to get.
         * @param metadata[out] The Entity's metadata, or invalid if not found.
         */
         void get_metadata_internal(
             const dbtype::Id &id,
             dbinterface::EntityMetadata &metadata);

        /**
         * Delete all entities and display name lookups for a site.
         * @param site_id[in] The site ID to delete.
         * @return True if success.
         */
        bool delete_site_entity_data(const dbtype::Id::SiteIdType site_id);

        /**
         * Given a statement with a result, add all IDs present to result.
         * @param result_stmt_ptr[in,out] A statement with parameters bound,
         * ready to run for a result.  Only a single column should be in the
         * result, the entity ID.
         * @param site_id[in] The site ID to combine with the entity ID to
         * create the final site-entity ID that gets inserted into the result.
         * @param result[out] The IDs found by running the result_stmt_ptr.
         */
        void add_entity_ids(
            sqlite3_stmt *result_stmt_ptr,
            const dbtype::Id::SiteIdType site_id,
            dbtype::Entity::IdVector &result);

        /**
         * Binds common parameters to a create/update entity type statement.
         * Also serializes the Entity.
         * @param entity_ptr[in] The Entity to be bound to the statement.
         * @param token[in] The lock token for entity_ptr.
         * @param stmt[in,out] The statement to bind to.
         * @return True if success.
         */
        bool bind_entity_update_params(
            dbtype::Entity *entity_ptr,
            concurrency::WriterLockToken &token,
            sqlite3_stmt *stmt);

        /**
         * Deletes the program registration name in the fast lookup table.
         * It is safe to call this if ID is not a program or even if the
         * registration does not exist.
         * It is assumed the mutex has already been locked.
         * @param id[in] The ID of the program registration to delete.
         */
        void delete_program_reg(const dbtype::Id &id);

        /**
         * Inserts a program registration name into the fast lookup table.
         * It is assumed the registration name is unique for the given
         * site, or else update will fail.
         * It is assumed the mutex has already been locked.
         * @param id[in] The ID for the program whose registration is being
         * inserted.
         * @param registration_name[in] The registration name for the program
         * being inserted.  The name must be unique for the site.
         * @return True if successfully inserted, false if error
         * (program registration name already exists in site).
         */
        bool insert_program_reg(
            const dbtype::Id &id,
            const std::string &registration_name);

        /**
         * Resets and clears bindings for a statement.
         * @param stmt_ptr[in,out] Prepared statement to reset.
         */
        void reset(sqlite3_stmt *stmt_ptr);

        sqlite3 *dbhandle_ptr; ///< SQLite handle data structure

        // Searches
        //
        sqlite3_stmt *list_sites_stmt; ///< Lists all valid site IDs
        sqlite3_stmt *list_deleted_sites_stmt; ///< Show all deleted sites
        sqlite3_stmt *list_all_entities_site_stmt; ///< Show all entities in site

        sqlite3_stmt *find_site_type_owner_name_exact_stmt;  ///< Find site, type, owner, exact name
        sqlite3_stmt *find_site_type_owner_name_stmt;  ///< Find site, type, owner, partial name
        sqlite3_stmt *find_site_type_name_exact_stmt; ///< Find site, type, exact name
        sqlite3_stmt *find_site_type_name_stmt; ///< Find site, type, partial name
        sqlite3_stmt *find_site_owner_type_stmt;  ///< Find site, owner, type

        sqlite3_stmt *find_site_owner_name_stmt;  ///< Find site, owner, partial name
        sqlite3_stmt *find_site_owner_stmt;  ///< Find site, owner
        sqlite3_stmt *find_site_name_stmt;  ///< Find site, partial name

        sqlite3_stmt *get_entity_type_stmt; ///< Gets the type for an Entity
        sqlite3_stmt *get_site_name_stmt; ///< Gets a site's name.
        sqlite3_stmt *get_site_description_stmt; ///< Gets a site's description.
        sqlite3_stmt *find_program_reg_stmt; ///< Find a program by registration name
        sqlite3_stmt *find_program_reg_id_stmt; ///< Find a program by registration by ID
        sqlite3_stmt *entity_exists_stmt; ///< Determine if an Entity exists

        // Create, edit, delete sites
        //
        sqlite3_stmt *undelete_site_stmt; ///< Update deleted site to be active
        sqlite3_stmt *next_site_id_stmt; ///< Get next new site id
        sqlite3_stmt *insert_first_next_site_id_stmt; ///< Insert first site ID
        sqlite3_stmt *update_next_site_id_stmt; ///< Update next new site ID
        sqlite3_stmt *insert_new_site_stmt; ///< insert new site ID into site table
        sqlite3_stmt *insert_first_site_entity_id_stmt; ///< Insert first entity ID for site
        sqlite3_stmt *delete_site_entities_stmt; ///< Delete all entities of a site
        sqlite3_stmt *delete_site_display_names_stmt; ///< Delete site's display names
        sqlite3_stmt *set_site_name_stmt; ///< Sets a new name for a site.
        sqlite3_stmt *set_site_description_stmt; ///< Sets a new description for a site.

        // Update and load entity
        //
        sqlite3_stmt *update_entity_stmt; ///< Updates Entity data, including blob
        sqlite3_stmt *get_entity_stmt; ///< Gets the blob data for an Entity
        sqlite3_stmt *get_entity_metadata_stmt; ///< Gets the entity's metadata

        // New entity
        //
        sqlite3_stmt *get_next_deleted_entity_id_stmt; ///< Get next deleted ent ID
        sqlite3_stmt *mark_deleted_id_used_stmt; ///< Mark a reused ent ID as used
        sqlite3_stmt *get_next_entity_id_stmt; ///< Get next fresh entity ID
        sqlite3_stmt *update_next_entity_id_stmt; ///< Update next entity ID counter
        sqlite3_stmt *add_entity_stmt; ///< Inserts complete new entity

        // Delete entity
        //
        sqlite3_stmt *delete_entity_stmt; ///< Deletes entity
        sqlite3_stmt *add_reuse_entity_id_stmt; ///< Adds entity ID to reuse table

        // Delete site
        //
        sqlite3_stmt *mark_site_deleted_stmt; ///< Mark site as deleted
        sqlite3_stmt *delete_all_site_entity_id_reuse_stmt; ///< Delete site ent ID reuse
        sqlite3_stmt *delete_site_next_entity_id_stmt; ///< Delete site next ent ID

        // Program registration fast lookup management
        sqlite3_stmt *insert_program_reg_stmt; // Adds a new program registration entry
        sqlite3_stmt *delete_program_reg_stmt; // Deletes an existing program registration entry

        boost::mutex mutex; ///< Enforces single access at a time.
    };
}
}


#endif //MUTGOS_SQLITEINTERFACE_SQLITEBACKEND_H
