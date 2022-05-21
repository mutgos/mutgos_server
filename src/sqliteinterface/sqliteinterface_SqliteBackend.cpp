/*
 * sqliteinterface_SqliteBackend.cpp.cpp
 */

#include <stddef.h>
#include <string>
#include <sqlite3.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "utilities/mutgos_config.h"
#include "text/text_StringConversion.h"

#include "sqliteinterface/sqliteinterface_SqliteBackend.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "concurrency/concurrency_ReaderLockToken.h"

#include "utilities/utility_MemoryBuffer.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_EntityType.h"
#include "concurrency/concurrency_WriterLockToken.h"

#include "logging/log_Logger.h"


namespace mutgos
{
namespace sqliteinterface
{
    // ----------------------------------------------------------------------
    SqliteBackend::SqliteBackend(void)
      : dbhandle_ptr(0),
        list_sites_stmt(0),
        list_deleted_sites_stmt(0),
        list_all_entities_site_stmt(0),
        find_site_type_owner_name_exact_stmt(0),
        find_site_type_owner_name_stmt(0),
        find_site_type_name_exact_stmt(0),
        find_site_type_name_stmt(0),
        find_site_owner_type_stmt(0),
        find_site_owner_name_stmt(0),
        find_site_owner_stmt(0),
        find_site_name_stmt(0),
        get_entity_type_stmt(0),
        get_site_name_stmt(0),
        get_site_description_stmt(0),
        find_program_reg_stmt(0),
        find_program_reg_id_stmt(0),
        entity_exists_stmt(0),
        undelete_site_stmt(0),
        next_site_id_stmt(0),
        insert_first_next_site_id_stmt(0),
        update_next_site_id_stmt(0),
        insert_new_site_stmt(0),
        insert_first_site_entity_id_stmt(0),
        delete_site_entities_stmt(0),
        delete_site_display_names_stmt(0),
        set_site_name_stmt(0),
        set_site_description_stmt(0),
        update_entity_stmt(0),
        get_entity_stmt(0),
        get_entity_metadata_stmt(0),
        get_next_deleted_entity_id_stmt(0),
        mark_deleted_id_used_stmt(0),
        get_next_entity_id_stmt(0),
        update_next_entity_id_stmt(0),
        add_entity_stmt(0),
        delete_entity_stmt(0),
        add_reuse_entity_id_stmt(0),
        mark_site_deleted_stmt(0),
        delete_all_site_entity_id_reuse_stmt(0),
        delete_site_next_entity_id_stmt(0),
        insert_program_reg_stmt(0),
        delete_program_reg_stmt(0)
    {
    }

    // ----------------------------------------------------------------------
    SqliteBackend::~SqliteBackend()
    {
        shutdown();
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::init(void)
    {
        LOG(info, "sqliteinterface", "init", "Starting up...");

        bool success = true;

        if (not dbhandle_ptr)
        {
            LOG(info, "sqliteinterface", "init", "Mounting database...");

            const int rc = sqlite3_open(
                config::db::db_file().c_str(),
                &dbhandle_ptr);
            success = (rc == SQLITE_OK);

            if (success)
            {
                success = (sqlite3_exec(
                    dbhandle_ptr,
                    "PRAGMA main.PAGE_SIZE=8192;",
                    0,
                    0,
                    0) == SQLITE_OK) and
                    (sqlite3_exec(
                        dbhandle_ptr,
                        "PRAGMA main.CACHE_SIZE=4000;",
                        0,
                        0,
                        0) == SQLITE_OK) and
                    (sqlite3_exec(
                        dbhandle_ptr,
                        "PRAGMA journal_mode=WAL;",
                        0,
                        0,
                        0) == SQLITE_OK) and
                    (sqlite3_exec(
                        dbhandle_ptr,
                        "PRAGMA synchronous=NORMAL;",
                        0,
                        0,
                        0) == SQLITE_OK)
                    and create_tables() and sql_init();

                if (success)
                {
                    LOG(info, "sqliteinterface", "init", "Database mounted.");
                }
                else
                {
                    LOG(fatal, "sqliteinterface", "init",
                        "Unable to configure SQL.");
                }
            }
            else
            {
                LOG(fatal, "sqliteinterface", "init",
                    "Unable to mount: " + std::string(sqlite3_errstr(rc)));
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::shutdown(void)
    {
        LOG(info, "sqliteinterface", "shutdown", "Shutting down...");

        bool success = not any_mem_owned();

        if (success and dbhandle_ptr)
        {
            sqlite3_finalize(list_sites_stmt);
            list_sites_stmt = 0;

            sqlite3_finalize(list_deleted_sites_stmt);
            list_deleted_sites_stmt = 0;

            sqlite3_finalize(list_all_entities_site_stmt);
            list_all_entities_site_stmt = 0;

            sqlite3_finalize(find_site_type_owner_name_exact_stmt);
            find_site_type_owner_name_exact_stmt = 0;

            sqlite3_finalize(find_site_type_owner_name_stmt);
            find_site_type_owner_name_stmt = 0;

            sqlite3_finalize(find_site_type_name_exact_stmt);
            find_site_type_name_exact_stmt = 0;

            sqlite3_finalize(find_site_type_name_stmt);
            find_site_type_name_stmt = 0;

            sqlite3_finalize(find_site_owner_type_stmt);
            find_site_owner_type_stmt = 0;

            sqlite3_finalize(find_site_owner_name_stmt);
            find_site_owner_name_stmt = 0;

            sqlite3_finalize(find_site_owner_stmt);
            find_site_owner_stmt = 0;

            sqlite3_finalize(find_site_name_stmt);
            find_site_name_stmt = 0;

            sqlite3_finalize(get_entity_type_stmt);
            get_entity_type_stmt = 0;

            sqlite3_finalize(get_site_name_stmt);
            get_site_name_stmt = 0;

            sqlite3_finalize(get_site_description_stmt);
            get_site_description_stmt = 0;

            sqlite3_finalize(find_program_reg_stmt);
            find_program_reg_stmt = 0;

            sqlite3_finalize(find_program_reg_id_stmt);
            find_program_reg_id_stmt = 0;

            sqlite3_finalize(entity_exists_stmt);
            entity_exists_stmt = 0;

            sqlite3_finalize(undelete_site_stmt);
            undelete_site_stmt = 0;

            sqlite3_finalize(next_site_id_stmt);
            next_site_id_stmt = 0;

            sqlite3_finalize(insert_first_next_site_id_stmt);
            insert_first_next_site_id_stmt = 0;

            sqlite3_finalize(update_next_site_id_stmt);
            update_next_site_id_stmt = 0;

            sqlite3_finalize(insert_new_site_stmt);
            insert_new_site_stmt = 0;

            sqlite3_finalize(insert_first_site_entity_id_stmt);
            insert_first_site_entity_id_stmt = 0;

            sqlite3_finalize(delete_site_entities_stmt);
            delete_site_entities_stmt = 0;

            sqlite3_finalize(delete_site_display_names_stmt);
            delete_site_display_names_stmt = 0;

            sqlite3_finalize(set_site_name_stmt);
            set_site_name_stmt = 0;

            sqlite3_finalize(set_site_description_stmt);
            set_site_description_stmt = 0;

            sqlite3_finalize(update_entity_stmt);
            update_entity_stmt = 0;

            sqlite3_finalize(get_entity_stmt);
            get_entity_stmt = 0;

            sqlite3_finalize(get_entity_metadata_stmt);
            get_entity_metadata_stmt = 0;

            sqlite3_finalize(get_next_deleted_entity_id_stmt);
            get_next_deleted_entity_id_stmt = 0;

            sqlite3_finalize(mark_deleted_id_used_stmt);
            mark_deleted_id_used_stmt = 0;

            sqlite3_finalize(get_next_entity_id_stmt);
            get_next_entity_id_stmt = 0;

            sqlite3_finalize(update_next_entity_id_stmt);
            update_next_entity_id_stmt = 0;

            sqlite3_finalize(add_entity_stmt);
            add_entity_stmt = 0;

            sqlite3_finalize(delete_entity_stmt);
            delete_entity_stmt = 0;

            sqlite3_finalize(add_reuse_entity_id_stmt);
            add_reuse_entity_id_stmt = 0;

            sqlite3_finalize(mark_site_deleted_stmt);
            mark_site_deleted_stmt = 0;

            sqlite3_finalize(delete_all_site_entity_id_reuse_stmt);
            delete_all_site_entity_id_reuse_stmt = 0;

            sqlite3_finalize(delete_site_next_entity_id_stmt);
            delete_site_next_entity_id_stmt = 0;

            sqlite3_finalize(insert_program_reg_stmt);
            insert_program_reg_stmt = 0;

            sqlite3_finalize(delete_program_reg_stmt);
            delete_program_reg_stmt = 0;

            success = (sqlite3_close(dbhandle_ptr) == SQLITE_OK);

            if (success)
            {
                dbhandle_ptr = 0;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    std::string SqliteBackend::get_backend_name(void)
    {
        return "SQLite3";
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::entity_mem_owned_by_this(
        const dbtype::Entity *entity_ptr)
    {
        return is_mem_owned(entity_ptr);
    }

    // ----------------------------------------------------------------------
    void SqliteBackend::delete_entity_mem(dbtype::Entity *entity_ptr)
    {
        if (removed_mem_owned(entity_ptr))
        {
            delete entity_ptr;
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Entity *SqliteBackend::new_entity(
        const dbtype::EntityType type,
        const dbtype::Id::SiteIdType site_id,
        const dbtype::Id &owner,
        const std::string &name)
    {
        dbtype::Entity *entity_ptr = 0;
        dbtype::Id::EntityIdType entity_id = 0;
        int rc = SQLITE_OK;
        bool fatal_error = false;

        // Do this in an inner scope so we can lock just this section
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            // See if there is a deleted ID we can reuse.
            // If so, remove from deleted list
            //
            if (sqlite3_bind_int(
                get_next_deleted_entity_id_stmt,
                sqlite3_bind_parameter_index(get_next_deleted_entity_id_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
              "For get_next_deleted_entity_id_stmt, could not bind $SITEID");
            }

            // Only stepping once since we just need one.
            rc = sqlite3_step(get_next_deleted_entity_id_stmt);

            if (rc == SQLITE_ROW)
            {
                // There is an ID we can use!
                //
                entity_id = sqlite3_column_int64(
                    get_next_deleted_entity_id_stmt, 0);

                fatal_error = not entity_id;

                // Remove ID from the reuse table, since we are now using it.
                //
                if (sqlite3_bind_int(
                    mark_deleted_id_used_stmt,
                    sqlite3_bind_parameter_index(mark_deleted_id_used_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                  "For mark_deleted_id_used_stmt, could not bind $SITEID");
                }

                if (sqlite3_bind_int64(
                    mark_deleted_id_used_stmt,
                    sqlite3_bind_parameter_index(mark_deleted_id_used_stmt,
                        "$ENTITYID"),
                    entity_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                  "For mark_deleted_id_used_stmt, could not bind $ENTITYID");
                }

                rc = sqlite3_step(mark_deleted_id_used_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "Could not remove selected ID from reuse list: "
                        + std::string(sqlite3_errstr(rc)));

                    fatal_error = true;
                }
            }
            else if (rc != SQLITE_DONE)
            {
                // An error occurred.
                //
                LOG(error, "sqliteinterface", "new_entity",
                    "Could not get next available recycled ID: "
                    + std::string(sqlite3_errstr(rc)));

                fatal_error = true;
            }

            reset(get_next_deleted_entity_id_stmt);
            reset(mark_deleted_id_used_stmt);

            // If no existing ID, get a fresh one
            //
            if (not fatal_error and (not entity_id))
            {
                if (sqlite3_bind_int(
                    get_next_entity_id_stmt,
                    sqlite3_bind_parameter_index(get_next_entity_id_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "For get_next_entity_id_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(get_next_entity_id_stmt);

                if (rc != SQLITE_ROW)
                {
                    LOG(error, "sqliteinterface", "new_entity",
                        "Could not get next fresh ID: "
                        + std::string(sqlite3_errstr(rc)));

                    fatal_error = true;
                }
                else
                {
                    entity_id =
                        (dbtype::Id::EntityIdType) sqlite3_column_int64(
                            get_next_entity_id_stmt, 0);

                    fatal_error = not entity_id;
                }

                if (not fatal_error)
                {
                    if (sqlite3_bind_int(
                        update_next_entity_id_stmt,
                        sqlite3_bind_parameter_index(
                            update_next_entity_id_stmt, "$SITEID"),
                        site_id) != SQLITE_OK)
                    {
                        LOG(error, "sqliteinterface", "new_entity",
                     "For update_next_entity_id_stmt, could not bind $SITEID");
                    }

                    if (sqlite3_bind_int64(
                        update_next_entity_id_stmt,
                        sqlite3_bind_parameter_index(
                            update_next_entity_id_stmt, "$NEXTID"),
                        entity_id + 1) != SQLITE_OK)
                    {
                        LOG(error, "sqliteinterface", "new_entity",
                     "For update_next_entity_id_stmt, could not bind $NEXTID");
                    }

                    rc = sqlite3_step(update_next_entity_id_stmt);

                    if (rc != SQLITE_DONE)
                    {
                        LOG(error, "sqliteinterface", "new_entity",
                            "Could not update next fresh ID: "
                            + std::string(sqlite3_errstr(rc)));

                        fatal_error = true;
                    }
                }
            }

            reset(get_next_entity_id_stmt);
            reset(update_next_entity_id_stmt);
        }

        // Don't lock ourselves here to avoid recursive locking
        // due to callbacks elsewhere wanting to call into us (creating new
        // player with unique name, etc).
        entity_ptr = fatal_error ? 0 : make_new_entity(
            type,
            dbtype::Id(site_id, entity_id),
            owner,
            name);
        fatal_error = not entity_ptr;

        if (entity_ptr)
        {
            boost::lock_guard<boost::mutex> guard(mutex);
            concurrency::WriterLockToken token(*entity_ptr);

            fatal_error = not bind_entity_update_params(
                entity_ptr,
                token,
                add_entity_stmt);

            if (sqlite3_bind_int(
                add_entity_stmt,
                sqlite3_bind_parameter_index(add_entity_stmt, "$VERSION"),
                entity_ptr->get_entity_version()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_entity",
                    "For add_entity_stmt, could not bind $VERSION");
            }

            if (fatal_error)
            {
                // Could not complete binding, abort
                //
                LOG(error, "sqliteinterface", "new_entity",
                    "Binding did not complete.  Aborted.  SiteID: "
                      + text::to_string(site_id)
                      + "  EntityID: "
                      + text::to_string(entity_id));
            }
            else
            {
                // Commit and return entity
                //
                const int rc = sqlite3_step(add_entity_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "save_entity_db",
                        "Could not save new Entity: "
                        + std::string(sqlite3_errstr(rc)));
                    fatal_error = true;
                }
            }

            reset(add_entity_stmt);
        }

        if (fatal_error)
        {
            delete entity_ptr;
            entity_ptr = 0;
        }

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity *SqliteBackend::get_entity_db(const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        dbtype::Entity *entity_ptr = get_entity_pointer(id);

        if (not entity_ptr)
        {
            // Not in memory, try and get it from the database
            //
            if (sqlite3_bind_int(
                get_entity_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_db",
                    "For get_entity_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                get_entity_stmt,
                sqlite3_bind_parameter_index(get_entity_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_db",
                    "For get_entity_stmt, could not bind $ENTITYID");
            }

            // Should be 0 or 1 lines
            //
            if (sqlite3_step(get_entity_stmt) == SQLITE_ROW)
            {
                // Entity exists.  Deserialize it.
                //
                const int entity_type_int = sqlite3_column_int(get_entity_stmt, 0);
                const void *blob_ptr = sqlite3_column_blob(get_entity_stmt, 1);
                const int blob_size = sqlite3_column_bytes(get_entity_stmt, 1);

                if ((not blob_ptr) or (blob_size <= 0))
                {
                    LOG(error, "sqliteinterface", "get_entity_db",
                        "No blob data for ID " + id.to_string(true));
                }
                else
                {
                    const dbtype::EntityType entity_type =
                        (dbtype::EntityType) entity_type_int;

                    utility::MemoryBuffer buffer(blob_ptr, blob_size);
                    entity_ptr = make_deserialize_entity(entity_type, buffer);

                    if (not entity_ptr)
                    {
                        LOG(error, "sqliteinterface", "get_entity_db",
                            "Unknown type to deserialize: "
                              + dbtype::entity_type_to_string(entity_type)
                              + "  ID: " + id.to_string(true));
                    }
                    else
                    {
                        // Put into lookup map
                        added_mem_owned(entity_ptr);
                    }
                }
            }

            reset(get_entity_stmt);
        }

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::entity_exists_db(const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool exists = get_entity_pointer(id);

        if (not exists)
        {
            // Not in memory, try and get it from the database
            //
            if (sqlite3_bind_int(
                entity_exists_stmt,
                sqlite3_bind_parameter_index(entity_exists_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "entity_exists_db",
                    "For entity_exists_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                entity_exists_stmt,
                sqlite3_bind_parameter_index(entity_exists_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "entity_exists_db",
                    "For entity_exists_stmt, could not bind $ENTITYID");
            }

            // Should be 0 or 1 lines
            exists = (sqlite3_step(entity_exists_stmt) == SQLITE_ROW);

            reset(entity_exists_stmt);
        }

        return exists;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::save_entity_db(dbtype::Entity *entity_ptr)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = entity_ptr and is_mem_owned(entity_ptr);

        if (success)
        {
            concurrency::WriterLockToken token(*entity_ptr);

            success = bind_entity_update_params(
                entity_ptr,
                token,
                update_entity_stmt);

            if (not success)
            {
                LOG(error, "sqliteinterface", "save_entity_db",
                    "Could not save entity!");
            }

            if (success)
            {
                const int rc = sqlite3_step(update_entity_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "save_entity_db",
                        "Could not update Entity: "
                        + std::string(sqlite3_errstr(rc)));
                    success = false;
                }
                else
                {
                    // For now, brute force update program registration
                    // cache, since updates are expected to be rare and cheap.
                    //
                    dbtype::Program * const program_ptr =
                        dynamic_cast<dbtype::Program *>(entity_ptr);

                    if (program_ptr)
                    {
                        delete_program_reg(program_ptr->get_entity_id());

                        const std::string reg_name =
                            program_ptr->get_program_reg_name(token);

                        if (not reg_name.empty())
                        {
                            if (not insert_program_reg(
                                program_ptr->get_entity_id(),
                                reg_name))
                            {
                                success = false;
                            }
                        }
                    }

                    entity_ptr->clear_dirty(token);
                }
            }

            reset(update_entity_stmt);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_entity_db(const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        // Confirm not still in memory
        const bool success = not is_mem_owned(id);

        if (success and (not id.is_default()))
        {
            bool delete_good = true;
            int rc = SQLITE_OK;

            // Delete from entities table
            //

            if (sqlite3_bind_int(
                delete_entity_stmt,
                sqlite3_bind_parameter_index(delete_entity_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "For delete_entity_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                delete_entity_stmt,
                sqlite3_bind_parameter_index(delete_entity_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "For delete_entity_stmt, could not bind $ENTITYID");
            }

            rc = sqlite3_step(delete_entity_stmt);

            if (rc == SQLITE_DONE)
            {
                delete_good = sqlite3_changes(dbhandle_ptr) > 0;
            }
            else
            {
                LOG(error, "sqliteinterface", "delete_entity_db",
                    "Could not delete Entity: "
                    + std::string(sqlite3_errstr(rc)));
                delete_good = false;
            }

            reset(delete_entity_stmt);

            if (delete_good)
            {
                // Delete worked, add ID into table for future reuse
                //
                if (sqlite3_bind_int(
                    add_reuse_entity_id_stmt,
                    sqlite3_bind_parameter_index(add_reuse_entity_id_stmt, "$SITEID"),
                    id.get_site_id()) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "For add_reuse_entity_id_stmt, could not bind $SITEID");
                }

                if (sqlite3_bind_int64(
                    add_reuse_entity_id_stmt,
                    sqlite3_bind_parameter_index(add_reuse_entity_id_stmt, "$ENTITYID"),
                    id.get_entity_id()) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "For add_reuse_entity_id_stmt, could not bind $ENTITYID");
                }

                rc = sqlite3_step(add_reuse_entity_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "delete_entity_db",
                        "Could not insert Entity ID into reuse table: "
                        + std::string(sqlite3_errstr(rc)));
                }

                reset(add_reuse_entity_id_stmt);

                // Delete from program registration if present.
                delete_program_reg(id);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    dbtype::EntityType SqliteBackend::get_entity_type_db(const dbtype::Id &id)
    {
        dbtype::EntityType entity_type = dbtype::ENTITYTYPE_invalid;

        boost::lock_guard<boost::mutex> guard(mutex);
        dbtype::Entity * const entity_ptr = get_entity_pointer(id);

        if (entity_ptr)
        {
            // In our cache of Entities in use.  Return the type.
            entity_type = entity_ptr->get_entity_type();
        }
        else
        {
            // Not in cache, try and get it from the database
            //
            if (sqlite3_bind_int(
                get_entity_type_stmt,
                sqlite3_bind_parameter_index(get_entity_type_stmt, "$SITEID"),
                id.get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_type_db",
                    "For get_entity_type_stmt, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                get_entity_type_stmt,
                sqlite3_bind_parameter_index(get_entity_type_stmt, "$ENTITYID"),
                id.get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "get_entity_type_db",
                    "For get_entity_type_stmt, could not bind $ENTITYID");
            }

            // Should be 0 or 1 lines
            //
            if (sqlite3_step(get_entity_type_stmt) == SQLITE_ROW)
            {
                // Entity exists.  Get the type.
                //
                const int entity_type_int =
                    sqlite3_column_int(get_entity_type_stmt, 0);

                entity_type = (dbtype::EntityType) entity_type_int;
            }

            reset(get_entity_type_stmt);
        }

        return entity_type;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector SqliteBackend::find_in_db(
        const dbtype::Id::SiteIdType site_id,
        const dbtype::EntityType type,
        const dbtype::Id::EntityIdType owner_id,
        const std::string &name,
        const bool exact)
    {
        dbtype::Entity::IdVector result;

        if (not site_id)
        {
            LOG(error, "sqliteinterface", "find_in_db()",
                "Site was not specified; cannot search");
            return result;
        }

        boost::lock_guard<boost::mutex> guard(mutex);

        sqlite3_stmt *stmt = 0;

        // Figure what type of search they want base don which parameters were
        // filled in.
        //
        if (type == dbtype::ENTITYTYPE_invalid)
        {
            if (owner_id and (not name.empty()))
            {
                stmt = find_site_owner_name_stmt;
            }
            else if (owner_id)
            {
                stmt = find_site_owner_stmt;
            }
            else if (not name.empty())
            {
                stmt = find_site_name_stmt;
            }
        }
        else
        {
            if (owner_id and name.empty())
            {
                stmt = find_site_owner_type_stmt;
            }
            else if (owner_id)
            {
                if (exact)
                {
                    stmt = find_site_type_owner_name_exact_stmt;
                }
                else
                {
                    stmt = find_site_type_owner_name_stmt;
                }
            }
            else
            {
                if (exact)
                {
                    stmt = find_site_type_name_exact_stmt;
                }
                else
                {
                    stmt = find_site_type_name_stmt;
                }
            }
        }

        if (not stmt)
        {
            LOG(error, "sqliteinterface", "find_in_db()",
                "Bad combination of parameters given; cannot search");
            return result;
        }

        if (sqlite3_bind_int(
            stmt,
            sqlite3_bind_parameter_index(stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db()",
                "For stmt, could not bind $SITEID");
        }

        if (type != dbtype::ENTITYTYPE_invalid)
        {
            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$TYPE"),
                type) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "find_in_db()",
                    "For stmt, could not bind $TYPE");
            }
        }

        if (owner_id)
        {
            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$OWNER"),
                owner_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "find_in_db()",
                    "For stmt, could not bind $OWNER");
            }
        }

        if (not name.empty())
        {
            if (sqlite3_bind_text(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$NAME"),
                name.c_str(),
                name.size(),
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "find_in_db()",
                    "For stmt, could not bind $NAME");
            }
        }

        add_entity_ids(stmt, site_id, result);

        reset(stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector SqliteBackend::find_in_db(
        const dbtype::Id::SiteIdType site_id)
    {
        dbtype::Entity::IdVector result;

        if (not site_id)
        {
            LOG(error, "sqliteinterface", "find_in_db(site)",
                "Site was not specified; cannot search");
            return result;
        }

        boost::lock_guard<boost::mutex> guard(mutex);

        if (sqlite3_bind_int(
            list_all_entities_site_stmt,
            sqlite3_bind_parameter_index(list_all_entities_site_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_in_db(site)",
                "For list_all_entities_site_stmt, could not bind $SITEID");
        }

        add_entity_ids(list_all_entities_site_stmt, site_id, result);

        reset(list_all_entities_site_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Id SqliteBackend::find_program_reg_in_db(
        const dbtype::Id::SiteIdType site_id,
        const std::string &registration_name)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        dbtype::Id result;

        if (sqlite3_bind_int(
            find_program_reg_stmt,
            sqlite3_bind_parameter_index(find_program_reg_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_program_reg_in_db",
                "For find_program_reg_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_text(
            find_program_reg_stmt,
            sqlite3_bind_parameter_index(find_program_reg_stmt, "$REGNAME"),
            registration_name.c_str(),
            registration_name.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_program_reg_in_db",
                "For find_program_reg_stmt, could not bind $REGNAME");
        }

        const int rc = sqlite3_step(find_program_reg_stmt);

        // Expecting 0 or 1 results.
        //
        if (rc == SQLITE_ROW)
        {
            // Found a registration
            //
            result =
                dbtype::Id(
                    site_id,
                    (dbtype::Id::EntityIdType)
                        sqlite3_column_int64(find_program_reg_stmt, 0));
        }
        else if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "find_program_reg_in_db",
                "Error getting registration info for program.");
        }

        reset (find_program_reg_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    std::string SqliteBackend::find_program_reg_name_in_db(
        const dbtype::Id &id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        std::string result;

        if (sqlite3_bind_int(
            find_program_reg_id_stmt,
            sqlite3_bind_parameter_index(find_program_reg_id_stmt, "$SITEID"),
            id.get_site_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_program_reg_name_in_db",
                "For find_program_reg_id_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_int(
            find_program_reg_id_stmt,
            sqlite3_bind_parameter_index(find_program_reg_id_stmt, "$ENTITYID"),
            id.get_entity_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "find_program_reg_name_in_db",
                "For find_program_reg_id_stmt, could not bind $REGNAME");
        }

        const int rc = sqlite3_step(find_program_reg_id_stmt);

        // Expecting 0 or 1 results.
        //
        if (rc == SQLITE_ROW)
        {
            // Found a registration
            //
            const char *reg_char_ptr = (const char *)
                sqlite3_column_text(find_program_reg_id_stmt, 0);
            const int reg_char_size =
                sqlite3_column_bytes(find_program_reg_id_stmt, 0);

            if (reg_char_size)
            {
                result.assign(reg_char_ptr, reg_char_size);
            }
        }
        else if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "find_program_reg_name_in_db",
                "Error getting registration info for program.");
        }

        reset (find_program_reg_id_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector SqliteBackend::get_site_ids_in_db(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        dbtype::Id::SiteIdVector result;
        int rc = sqlite3_step(list_sites_stmt);

        while (rc == SQLITE_ROW)
        {
            result.push_back(
                (dbtype::Id::SiteIdType) sqlite3_column_int(list_sites_stmt, 0));
            rc = sqlite3_step(list_sites_stmt);
        }

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "get_site_ids_in_db",
                "Error getting list of site IDs: "
                + std::string(sqlite3_errstr(rc)));
        }

        reset(list_sites_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    dbinterface::EntityMetadata
    SqliteBackend::get_entity_metadata(const dbtype::Id &id)
    {
        dbinterface::EntityMetadata result;

        // See if in memory;  If so, use that version instead
        if (is_mem_owned(id))
        {
            dbinterface::EntityRef entity_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(id);

            if (entity_ref.valid())
            {
                concurrency::ReaderLockToken token(*entity_ref.get());

                result.set(
                    id,
                    entity_ref->get_entity_owner(token),
                    entity_ref->get_entity_type(),
                    entity_ref->get_entity_version(),
                    entity_ref->get_entity_name(token));
            }

            return result;
        }

        // Not in memory, use database metadata.
        //
        boost::lock_guard<boost::mutex> guard(mutex);
        get_metadata_internal(id, result);

        return result;
    }

    // ----------------------------------------------------------------------
    dbinterface::MetadataVector
    SqliteBackend::get_entity_metadata(const dbtype::Entity::IdVector &ids)
    {
        dbinterface::MetadataVector result;
        std::vector<const dbtype::Id *> not_in_mem;

        not_in_mem.reserve(ids.size());

        for (dbtype::Entity::IdVector::const_iterator id_iter = ids.begin();
            id_iter != ids.end();
            ++id_iter)
        {
            if (is_mem_owned(*id_iter))
            {
                // Can look entity up directly in memory
                //
                dbinterface::EntityRef entity_ref =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        *id_iter);

                if (entity_ref.valid())
                {
                    result.push_back(dbinterface::EntityMetadata());
                    concurrency::ReaderLockToken token(*entity_ref.get());

                    result.back().set(
                        *id_iter,
                        entity_ref->get_entity_owner(token),
                        entity_ref->get_entity_type(),
                        entity_ref->get_entity_version(),
                        entity_ref->get_entity_name(token));

                    if (not result.back().valid())
                    {
                        result.pop_back();
                    }
                }
            }
            else
            {
                // Need to look it up in the database in a batch operation.
                not_in_mem.push_back(&(*id_iter));
            }
        }

        if (not not_in_mem.empty())
        {
            // Have some IDs to look up direct in the database.
            // Do them all at once under the same lock for efficiency
            // (hopefully).
            //
            boost::lock_guard<boost::mutex> guard(mutex);

            for (std::vector<const dbtype::Id *>::const_iterator id_ptr_iter =
                not_in_mem.begin();
                 id_ptr_iter != not_in_mem.end();
                 ++id_ptr_iter)
            {
                result.push_back(dbinterface::EntityMetadata());
                get_metadata_internal(**id_ptr_iter, result.back());

                if (not result.back().valid())
                {
                    result.pop_back();
                }
            }

            not_in_mem.clear();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::new_site_in_db(dbtype::Id::SiteIdType &site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = false;
        int rc = SQLITE_OK;

        // Query all deleted sites.  Pick first.  Mark as not deleted. Done.

        // See if we can reuse a site that has been deleted.
        //
        rc = sqlite3_step(list_deleted_sites_stmt);

        if (rc == SQLITE_ROW)
        {
            // Found one we can reuse!
            //
            site_id = (dbtype::Id::SiteIdType)
                sqlite3_column_int(list_deleted_sites_stmt, 0);

            // Update site as no longer deleted.
            //
            if (sqlite3_bind_int(
                undelete_site_stmt,
                sqlite3_bind_parameter_index(undelete_site_stmt, "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "new_site_in_db",
                    "For undelete_site_stmt, could not bind $SITEID");
            }

            rc = sqlite3_step(undelete_site_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "new_site_in_db",
                    "Could not reuse site: "
                    + std::string(sqlite3_errstr(rc)));
            }
            else
            {
                rc = SQLITE_OK;
                success = true;
            }

            reset(undelete_site_stmt);
        }
        else if (rc != SQLITE_DONE)
        {
            // Query errored out
            LOG(error, "sqliteinterface", "new_site_in_db",
                "Error getting list of deleted site IDs: "
                + std::string(sqlite3_errstr(rc)));
        }
        else
        {
            // So the next section knows it wasn't an error that stopped us
            // from getting an ID.
            rc = SQLITE_OK;
        }

        reset(list_deleted_sites_stmt);

        if ((not success) and (rc == SQLITE_OK))
        {
            // No available deleted sites to reuse.  Get a new one.
            //
            rc = sqlite3_step(next_site_id_stmt);

            if (rc == SQLITE_ROW)
            {
                // Found next ID.  Use it, and increment in table.
                //
                site_id = (dbtype::Id::SiteIdType)
                    sqlite3_column_int(next_site_id_stmt, 0);

                if (sqlite3_bind_int(
                    update_next_site_id_stmt,
                    sqlite3_bind_parameter_index(
                        update_next_site_id_stmt,
                        "$SITEID"),
                    site_id + 1) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For update_next_site_id_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(update_next_site_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not update next site ID: "
                        + std::string(sqlite3_errstr(rc)));
                }
                else
                {
                    rc = SQLITE_OK;
                    success = true;
                }

                reset(update_next_site_id_stmt);
            }
            else
            {
                // No next ID found - first use.  Insert default and
                // use it.
                //
                rc = sqlite3_step(insert_first_next_site_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert first next site ID: "
                        + std::string(sqlite3_errstr(rc)));
                }
                else
                {
                    rc = SQLITE_OK;
                    success = true;
                    site_id = 1;
                }

                reset(insert_first_next_site_id_stmt);
            }

            reset(next_site_id_stmt);

            if (success)
            {
                // Insert the new site into the sites table
                //

                if (sqlite3_bind_int(
                    insert_new_site_stmt,
                    sqlite3_bind_parameter_index(
                        insert_new_site_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_new_site_stmt, could not bind $SITEID");
                }

                const std::string default_site_name =
                    "Untitled Site " + text::to_string(site_id);

                if (sqlite3_bind_text(
                    insert_new_site_stmt,
                    sqlite3_bind_parameter_index(
                        insert_new_site_stmt,
                        "$SITENAME"),
                    default_site_name.c_str(),
                    default_site_name.size(),
                    SQLITE_TRANSIENT) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_new_site_stmt, could not bind $SITENAME");
                }

                const std::string empty_description;

                if (sqlite3_bind_text(
                    insert_new_site_stmt,
                    sqlite3_bind_parameter_index(
                        insert_new_site_stmt,
                        "$SITEDESCRIPTION"),
                    empty_description.c_str(),
                    empty_description.size(),
                    SQLITE_TRANSIENT) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_new_site_stmt, could not bind $SITEDESCRIPTION");
                }

                rc = sqlite3_step(insert_new_site_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert new site: "
                        + std::string(sqlite3_errstr(rc)));

                    success = false;
                }
                else
                {
                    rc = SQLITE_OK;
                }

                reset(insert_new_site_stmt);

                // Delete all existing entity data with that site ID in case
                // anything is still there.
                //
                delete_site_entity_data(site_id);
            }

            if (success)
            {
                if (sqlite3_bind_int(
                    insert_first_site_entity_id_stmt,
                    sqlite3_bind_parameter_index(
                        insert_first_site_entity_id_stmt,
                        "$SITEID"),
                    site_id) != SQLITE_OK)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "For insert_first_site_entity_id_stmt, could not bind $SITEID");
                }

                rc = sqlite3_step(insert_first_site_entity_id_stmt);

                if (rc != SQLITE_DONE)
                {
                    LOG(error, "sqliteinterface", "new_site_in_db",
                        "Could not insert new site first entity ID: "
                        + std::string(sqlite3_errstr(rc)));

                    success = false;
                }
                else
                {
                    rc = SQLITE_OK;
                }

                reset(insert_first_site_entity_id_stmt);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_site_in_db(const dbtype::Id::SiteIdType site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = delete_site_entity_data(site_id);
        int rc = SQLITE_OK;

        if (success)
        {
            if (sqlite3_bind_int(
                mark_site_deleted_stmt,
                sqlite3_bind_parameter_index(
                    mark_site_deleted_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For mark_site_deleted_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(mark_site_deleted_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not mark site as deleted: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            if (sqlite3_bind_int(
                delete_all_site_entity_id_reuse_stmt,
                sqlite3_bind_parameter_index(
                    delete_all_site_entity_id_reuse_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For delete_all_site_entity_id_reuse_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(delete_all_site_entity_id_reuse_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not delete all site reuse entity IDs: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            if (sqlite3_bind_int(
                delete_site_next_entity_id_stmt,
                sqlite3_bind_parameter_index(
                    delete_site_next_entity_id_stmt,
                    "$SITEID"),
                site_id) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "For delete_site_next_entity_id_stmt, could not bind $SITEID");
            }


            rc = sqlite3_step(delete_site_next_entity_id_stmt);

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "delete_site_in_db",
                    "Could not delete site fresh ID table entry: "
                    + std::string(sqlite3_errstr(rc)));

                success = false;
            }

            reset(mark_site_deleted_stmt);
            reset(delete_all_site_entity_id_reuse_stmt);
            reset(delete_site_next_entity_id_stmt);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::get_site_name_in_db(
        const dbtype::Id::SiteIdType site_id,
        std::string &site_name)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        bool success = false;

        site_name.clear();

        if (sqlite3_bind_int(
            get_site_name_stmt,
            sqlite3_bind_parameter_index(get_site_name_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "get_site_name_in_db",
                "For get_site_name_stmt, could not bind $SITEID");
        }

        // Should be 0 or 1 lines
        //
        if (sqlite3_step(get_site_name_stmt) == SQLITE_ROW)
        {
            // Site exists.  Get the name.
            //
            success = true;
            const char *name_char_ptr = (const char *)
                sqlite3_column_text(get_site_name_stmt, 0);
            const int name_char_size =
                sqlite3_column_bytes(get_site_name_stmt, 0);

            if (name_char_size)
            {
                site_name.assign(name_char_ptr, name_char_size);
            }
        }

        reset(get_site_name_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::set_site_name_in_db(
        const dbtype::Id::SiteIdType site_id,
        const std::string &site_name)
    {
        bool success = false;
        int rc = SQLITE_OK;
        boost::lock_guard<boost::mutex> guard(mutex);

        if (sqlite3_bind_int(
            set_site_name_stmt,
            sqlite3_bind_parameter_index(set_site_name_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "set_site_name_in_db",
                "For set_site_name_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_text(
            set_site_name_stmt,
            sqlite3_bind_parameter_index(
                set_site_name_stmt,
                "$SITENAME"),
            site_name.c_str(),
            site_name.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "set_site_name_in_db",
                "For set_site_name_stmt, could not bind $SITENAME");
        }

        rc = sqlite3_step(set_site_name_stmt);

        if (rc == SQLITE_DONE)
        {
            success = true;
        }
        else
        {
            LOG(error, "sqliteinterface", "set_site_name_in_db",
                "Could not set site name: " + std::string(sqlite3_errstr(rc)));
        }

        reset(set_site_name_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::get_site_description_in_db(
        const dbtype::Id::SiteIdType site_id,
        std::string &site_description)
    {
        bool success = false;

        boost::lock_guard<boost::mutex> guard(mutex);

        site_description.clear();

        if (sqlite3_bind_int(
            get_site_description_stmt,
            sqlite3_bind_parameter_index(get_site_description_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "get_site_description_in_db",
                "For get_site_description_stmt, could not bind $SITEID");
        }

        // Should be 0 or 1 lines
        //
        if (sqlite3_step(get_site_description_stmt) == SQLITE_ROW)
        {
            // Site exists.  Get the description.
            //
            success = true;
            const char *description_char_ptr = (const char *)
                sqlite3_column_text(get_site_description_stmt, 0);
            const int description_char_size =
                sqlite3_column_bytes(get_site_description_stmt, 0);

            if (description_char_size)
            {
                site_description.assign(
                    description_char_ptr,
                    description_char_size);
            }
        }

        reset(get_site_description_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::set_site_description_in_db(
        const dbtype::Id::SiteIdType site_id,
        const std::string &site_description)
    {
        bool success = false;
        int rc = SQLITE_OK;
        boost::lock_guard<boost::mutex> guard(mutex);

        if (sqlite3_bind_int(
            set_site_description_stmt,
            sqlite3_bind_parameter_index(set_site_description_stmt, "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "set_site_description_in_db",
                "For set_site_description_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_text(
            set_site_description_stmt,
            sqlite3_bind_parameter_index(
                set_site_description_stmt,
                "$SITEDESCRIPTION"),
            site_description.c_str(),
            site_description.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "set_site_description_in_db",
                "For set_site_description_stmt, could not bind $SITEDESCRIPTION");
        }

        rc = sqlite3_step(set_site_description_stmt);

        if (rc == SQLITE_DONE)
        {
            success = true;
        }
        else
        {
            LOG(error, "sqliteinterface", "set_site_description_in_db",
                "Could not set site description: " + std::string(sqlite3_errstr(rc)));
        }

        reset(set_site_description_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::create_tables(void)
    {
        const std::string create_tables_str =
         "CREATE TABLE IF NOT EXISTS entities("
            "site_id INTEGER NOT NULL,"
            "entity_id INTEGER NOT NULL,"
            "owner INTEGER,"
            "type INTEGER NOT NULL,"
            "version INTEGER NOT NULL,"
            "name TEXT NOT NULL COLLATE NOCASE,"
            "data BLOB NOT NULL,"
         "PRIMARY KEY(site_id, entity_id)) WITHOUT ROWID;"
         "CREATE INDEX IF NOT EXISTS entity_type_idx ON entities(site_id, name, type);"

         "CREATE TABLE IF NOT EXISTS program_registrations("
            "site_id INTEGER NOT NULL,"
            "entity_id INTEGER NOT NULL,"
            "registration_name TEXT NOT NULL COLLATE NOCASE,"
         "PRIMARY KEY(site_id, registration_name)) WITHOUT ROWID;"
         "CREATE INDEX IF NOT EXISTS program_registrations_idx ON program_registrations(site_id, entity_id);"

         "CREATE TABLE IF NOT EXISTS sites("
            "site_id INTEGER NOT NULL,"
            "deleted INTEGER NOT NULL,"
            "site_name TEXT NOT NULL,"
            "site_description TEXT NOT NULL,"
            "PRIMARY KEY(site_id, deleted)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS id_reuse("
            "site_id INTEGER NOT NULL,"
            "deleted_entity_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id, deleted_entity_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS next_id("
            "site_id INTEGER NOT NULL,"
            "next_entity_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS next_site_id("
            "site_id INTEGER NOT NULL,"
            "PRIMARY KEY(site_id)) WITHOUT ROWID;"

         "CREATE TABLE IF NOT EXISTS display_names("
            "site_id INTEGER NOT NULL,"
            "entity_id INTEGER NOT NULL,"
            "player INTEGER NOT NULL,"
            "name TEXT NOT NULL COLLATE NOCASE,"
            "display_name TEXT NOT NULL COLLATE NOCASE,"
            "PRIMARY KEY(site_id, entity_id),"
            "FOREIGN KEY(site_id, entity_id) REFERENCES entities(site_id, entity_id)) WITHOUT ROWID;"
         "CREATE INDEX IF NOT EXISTS display_name_player_idx ON display_names(site_id, player, name, display_name);";

        bool success = true;
        char *rc_error_str_ptr = 0;
        const int rc = sqlite3_exec(
            dbhandle_ptr,
            create_tables_str.c_str(),
            0,
            0,
            &rc_error_str_ptr);

        if (rc != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "create_tables",
                "Unable to create tables: " + std::string(sqlite3_errstr(rc))
                + "   Full error: " + std::string(rc_error_str_ptr));

            sqlite3_free(rc_error_str_ptr);
            rc_error_str_ptr = 0;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::sql_init(void)
    {
        bool success = true;

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM sites WHERE deleted = 0;",
            -1,
            &list_sites_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding valid sites.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM sites WHERE deleted = 1;",
            -1,
            &list_deleted_sites_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding deleted sites.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID;",
            -1,
            &list_all_entities_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing all of site's entities.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
            "type = $TYPE AND owner = $OWNER AND "
            "name = $NAME;",
            -1,
            &find_site_type_owner_name_exact_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by site, "
                "type, owner, and exact name");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
            "type = $TYPE AND owner = $OWNER AND "
            "name LIKE '%' || $NAME || '%';",
            -1,
            &find_site_type_owner_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by site, "
                "type, owner, and name");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
            "owner = $OWNER AND type = $TYPE;",
            -1,
            &find_site_owner_type_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by site, "
                "owner, and type");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
            "owner = $OWNER AND "
            "name LIKE '%' || $NAME || '%';",
            -1,
            &find_site_owner_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by site, "
                "owner, and name");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
            "owner = $OWNER",
            -1,
            &find_site_owner_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by site "
                "and owner");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name LIKE '%' || $NAME || '%';",
            -1,
            &find_site_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by name.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name LIKE '%' || $NAME || '%' AND type = $TYPE;",
            -1,
            &find_site_type_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by name and type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM entities WHERE site_id = $SITEID AND "
                "name = $NAME AND type = $TYPE;",
            -1,
            &find_site_type_name_exact_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for listing entity by exact "
                    "name and type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT type FROM entities WHERE site_id = $SITEID "
            "and entity_id = $ENTITYID;",
            -1,
            &get_entity_type_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting an Entity type.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_name FROM sites WHERE site_id = $SITEID;",
            -1,
            &get_site_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting a Site's name.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_description FROM sites WHERE site_id = $SITEID;",
            -1,
            &get_site_description_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting a Site's description.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT entity_id FROM program_registrations WHERE "
              "site_id = $SITEID AND registration_name = $REGNAME;",
            -1,
            &find_program_reg_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding a program registration.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT registration_name FROM program_registrations WHERE "
            "site_id = $SITEID AND entity_id = $ENTITYID;",
            -1,
            &find_program_reg_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for finding a program registration by ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT type FROM entities WHERE site_id = $SITEID "
            "and entity_id = $ENTITYID;",
            -1,
            &entity_exists_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for checking Entity existence.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET deleted = 0 WHERE site_id = $SITEID;",
            -1,
            &undelete_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for reusing a site.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT site_id FROM next_site_id;",
            -1,
            &next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting a new site ID.");
        }

        // Starts at 2 because this is called after first site created.
        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO next_site_id(site_id) VALUES (2);",
            -1,
            &insert_first_next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting the first next site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE next_site_id SET site_id = $SITEID;",
            -1,
            &update_next_site_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating the next site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO sites(site_id, site_name, site_description, deleted) "
                "VALUES ($SITEID, $SITENAME, $SITEDESCRIPTION, 0);",
            -1,
            &insert_new_site_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting a new site.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO next_id(site_id, next_entity_id) VALUES ($SITEID, 1);",
            -1,
            &insert_first_site_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting first site ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM entities WHERE site_id = $SITEID;",
            -1,
            &delete_site_entities_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for delete a site's entities.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM display_names WHERE site_id = $SITEID;",
            -1,
            &delete_site_display_names_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for delete a site's display names.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET site_name = $SITENAME WHERE site_id = $SITEID;",
            -1,
            &set_site_name_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for setting a site's name.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET site_description = $SITEDESCRIPTION "
                "WHERE site_id = $SITEID;",
            -1,
            &set_site_description_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for setting a site's description.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE entities SET owner = $OWNER, type = $TYPE, name = $NAME, "
                                "data = $DATA "
                "WHERE site_id = $SITEID and entity_id = $ENTITYID;",
            -1,
            &update_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT type, data FROM entities WHERE site_id = $SITEID "
                "and entity_id = $ENTITYID;",
            -1,
            &get_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT owner, type, version, name FROM entities WHERE "
            "site_id = $SITEID and entity_id = $ENTITYID;",
            -1,
            &get_entity_metadata_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting an Entity metadata.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT deleted_entity_id FROM id_reuse WHERE site_id = $SITEID;",
            -1,
            &get_next_deleted_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for get next deleted entity ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM id_reuse WHERE site_id = $SITEID "
                "AND deleted_entity_id = $ENTITYID;",
            -1,
            &mark_deleted_id_used_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for marking reused entity ID as used.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "SELECT next_entity_id FROM next_id WHERE site_id = $SITEID;",
            -1,
            &get_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for getting next fresh entity ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE next_id SET next_entity_id = $NEXTID WHERE site_id = $SITEID;",
            -1,
            &update_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for updating next entity ID counter.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO "
              "entities(site_id, entity_id, owner, type, version, name, data) "
              "VALUES ($SITEID, $ENTITYID, $OWNER, $TYPE, $VERSION, $NAME, $DATA);",
            -1,
            &add_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting new Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM entities WHERE site_id = $SITEID AND entity_id = $ENTITYID;",
            -1,
            &delete_entity_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting an Entity.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO id_reuse(site_id, deleted_entity_id) VALUES "
                "($SITEID, $ENTITYID);",
            -1,
            &add_reuse_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for adding Entity to reuse table.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "UPDATE sites SET deleted = 1 WHERE site_id = $SITEID;",
            -1,
            &mark_site_deleted_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for marking site as deleted.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM id_reuse WHERE site_id = $SITEID;",
            -1,
            &delete_all_site_entity_id_reuse_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting site reuse Entity IDs.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM next_id WHERE site_id = $SITEID;",
            -1,
            &delete_site_next_entity_id_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting site next Entity ID.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "INSERT INTO program_registrations(site_id, entity_id, registration_name) VALUES "
            "($SITEID, $ENTITYID, $REGNAME);",
            -1,
            &insert_program_reg_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for inserting a program registration.");
        }

        if (sqlite3_prepare_v2(
            dbhandle_ptr,
            "DELETE FROM program_registrations WHERE "
              "site_id = $SITEID AND entity_id = $ENTITYID;",
            -1,
            &delete_program_reg_stmt,
            0) != SQLITE_OK)
        {
            success = false;

            LOG(fatal, "sqliteinterface", "sql_init",
                "Failed prepared statement for deleting  a program registration.");
        }

        return success;
    }

    void SqliteBackend::get_metadata_internal(
        const dbtype::Id &id,
        dbinterface::EntityMetadata &metadata)
    {
        // Look up the Entity
        //
        if (sqlite3_bind_int(
            get_entity_metadata_stmt,
            sqlite3_bind_parameter_index(get_entity_metadata_stmt, "$SITEID"),
            id.get_site_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "get_metadata_internal",
                "For get_metadata_internal, could not bind $SITEID");
        }

        if (sqlite3_bind_int64(
            get_entity_metadata_stmt,
            sqlite3_bind_parameter_index(get_entity_metadata_stmt, "$ENTITYID"),
            id.get_entity_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "get_metadata_internal",
                "For get_metadata_internal, could not bind $ENTITYID");
        }

        if (sqlite3_step(get_entity_metadata_stmt) != SQLITE_ROW)
        {
            metadata.reset();
        }
        else
        {
            // Found an entry.  Extract the info.
            //

            // Owner
            const dbtype::Id::EntityIdType entity_owner_int =
                sqlite3_column_int64(get_entity_metadata_stmt, 0);
            // Type (as int)
            const int entity_type_int =
                sqlite3_column_int(get_entity_metadata_stmt, 1);
            // version
            const dbtype::Entity::VersionType version =
                sqlite3_column_int(get_entity_metadata_stmt, 2);
            // name (string)
            const char *name_char_ptr = (const char *)
                sqlite3_column_text(get_entity_metadata_stmt, 3);
            const int name_char_size =
                sqlite3_column_bytes(get_entity_metadata_stmt, 3);
            std::string name;

            if (name_char_size)
            {
                name.assign(name_char_ptr, name_char_size);
            }

            // Type as enum
            const dbtype::EntityType entity_type =
                (dbtype::EntityType) entity_type_int;

            metadata.set(
                id,
                dbtype::Id(id.get_site_id(), entity_owner_int),
                entity_type,
                version,
                name);
        }

        reset(get_entity_metadata_stmt);
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::delete_site_entity_data(
        const dbtype::Id::SiteIdType site_id)
    {
        bool success = true;
        int rc = SQLITE_OK;

        if (sqlite3_bind_int(
            delete_site_entities_stmt,
            sqlite3_bind_parameter_index(
                delete_site_entities_stmt,
                "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "For delete_site_entities_stmt, could not bind $SITEID");
        }

        rc = sqlite3_step(delete_site_entities_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "Could not delete site entities: "
                + std::string(sqlite3_errstr(rc)));

            success = false;
        }
        else
        {
            rc = SQLITE_OK;
        }

        reset(delete_site_entities_stmt);

        if (sqlite3_bind_int(
            delete_site_display_names_stmt,
            sqlite3_bind_parameter_index(
                delete_site_display_names_stmt,
                "$SITEID"),
            site_id) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "For delete_site_display_names_stmt, could not bind $SITEID");
        }

        rc = sqlite3_step(delete_site_display_names_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "delete_site_entity_data",
                "Could not delete site display names: "
                + std::string(sqlite3_errstr(rc)));

            success = false;
        }
        else
        {
            rc = SQLITE_OK;
        }

        reset(delete_site_display_names_stmt);

        return success;
    }

    // ----------------------------------------------------------------------
    void SqliteBackend::add_entity_ids(
        sqlite3_stmt *result_stmt_ptr,
        const dbtype::Id::SiteIdType site_id,
        dbtype::Entity::IdVector &result)
    {
        if (result_stmt_ptr)
        {
            int rc = sqlite3_step(result_stmt_ptr);

            while (rc == SQLITE_ROW)
            {
                result.push_back(
                    dbtype::Id(
                        site_id,
                        (dbtype::Id::EntityIdType)
                            sqlite3_column_int64(result_stmt_ptr, 0)));
                rc = sqlite3_step(result_stmt_ptr);
            }

            if (rc != SQLITE_DONE)
            {
                LOG(error, "sqliteinterface", "add_entity_ids",
                    "Error getting list of entity IDs: "
                    + std::string(sqlite3_errstr(rc)));
            }
        }
    }

    // ----------------------------------------------------------------------
    void SqliteBackend::delete_program_reg(const mutgos::dbtype::Id &id)
    {
        if (sqlite3_bind_int(
            delete_program_reg_stmt,
            sqlite3_bind_parameter_index(delete_program_reg_stmt, "$SITEID"),
            id.get_site_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_program_reg",
                "For delete_program_reg_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_int64(
            delete_program_reg_stmt,
            sqlite3_bind_parameter_index(delete_program_reg_stmt, "$ENTITYID"),
            id.get_entity_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "delete_program_reg",
                "For delete_program_reg_stmt, could not bind $ENTITYID");
        }

        const int rc = sqlite3_step(delete_program_reg_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "delete_program_reg",
                "Could not delete Entity program registration: "
                + std::string(sqlite3_errstr(rc)));
        }

        reset(delete_program_reg_stmt);
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::insert_program_reg(
        const mutgos::dbtype::Id &id,
        const std::string &registration_name)
    {
        bool result = true;

        if (sqlite3_bind_int(
            insert_program_reg_stmt,
            sqlite3_bind_parameter_index(insert_program_reg_stmt, "$SITEID"),
            id.get_site_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "insert_program_reg",
                "For insert_program_reg_stmt, could not bind $SITEID");
        }

        if (sqlite3_bind_int64(
            insert_program_reg_stmt,
            sqlite3_bind_parameter_index(insert_program_reg_stmt, "$ENTITYID"),
            id.get_entity_id()) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "insert_program_reg",
                "For insert_program_reg_stmt, could not bind $ENTITYID");
        }

        if (sqlite3_bind_text(
            insert_program_reg_stmt,
            sqlite3_bind_parameter_index(insert_program_reg_stmt, "$REGNAME"),
            registration_name.c_str(),
            registration_name.size(),
            SQLITE_TRANSIENT) != SQLITE_OK)
        {
            LOG(error, "sqliteinterface", "insert_program_reg",
                "For insert_program_reg_stmt, could not bind $REGNAME");
        }

        const int rc = sqlite3_step(insert_program_reg_stmt);

        if (rc != SQLITE_DONE)
        {
            LOG(error, "sqliteinterface", "insert_program_reg",
                "Could not insert Program registration: "
                + std::string(sqlite3_errstr(rc)));

            result = false;
        }

        reset(insert_program_reg_stmt);

        return result;
    }

    // ----------------------------------------------------------------------
    bool SqliteBackend::bind_entity_update_params(
        dbtype::Entity *entity_ptr,
        concurrency::WriterLockToken &token,
        sqlite3_stmt *stmt)
    {
        bool success = entity_ptr;

        if (success)
        {
            utility::MemoryBuffer buffer;
            const std::string name = entity_ptr->get_entity_name(token);
            char *data_ptr = 0;
            size_t data_size = 0;
            success = serialize_entity(entity_ptr, buffer) and
                      buffer.get_data(data_ptr, data_size);

            if (not success)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "Could not serialize entity!");
            }

            if (sqlite3_bind_int64(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$OWNER"),
                entity_ptr->get_entity_owner(token).get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $OWNER");
            }

            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$TYPE"),
                entity_ptr->get_entity_type()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $TYPE");
            }

            if (sqlite3_bind_text(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$NAME"),
                name.c_str(),
                name.size(),
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $NAME");
            }

            if (sqlite3_bind_int(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$SITEID"),
                entity_ptr->get_entity_id().get_site_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $SITEID");
            }

            if (sqlite3_bind_int64(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$ENTITYID"),
                entity_ptr->get_entity_id().get_entity_id()) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $ENTITYID");
            }

            if (sqlite3_bind_blob(
                stmt,
                sqlite3_bind_parameter_index(stmt, "$DATA"),
                data_ptr,
                data_size,
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                LOG(error, "sqliteinterface", "bind_entity_update_params",
                    "For statement, could not bind $DATA");
            }
        }

        return success;

    }

    // ----------------------------------------------------------------------
    void SqliteBackend::reset(sqlite3_stmt *stmt_ptr)
    {
        if (stmt_ptr)
        {
            sqlite3_reset(stmt_ptr);
            sqlite3_clear_bindings(stmt_ptr);
        }
    }
}
}
