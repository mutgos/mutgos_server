/*
 * dbinterface_DbBackend.h
 */

#ifndef MUTGOS_DBINTERFACE_DBBACKEND_H
#define MUTGOS_DBINTERFACE_DBBACKEND_H

#include <vector>
#include <string>
#include <map>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Id.h"

#include "dbinterface/dbinterface_CommonTypes.h"
#include "dbinterface/dbinterface_EntityMetadata.h"

#include "utilities/utility_MemoryBuffer.h"

#include <boost/thread/shared_mutex.hpp>

namespace mutgos
{
namespace dbinterface
{
    // TODO Stats (Entities per user, total per site, etc)
    // TODO Enhanced player searching
    // TODO Enhanced searched ('filter by player', plugin filter)
    // TODO ID 'valid' check

    /**
     * Class that a database backend must implement for MUTGOS
     * to use it to load, save, and search for entities.  All creation of
     * Entities must be done using this class's implementor.
     * This also serves as the interface for the database backend as a
     * dyanmically loaded library.
     *
     * The implementation does not need to implement caching as MUTGOS will
     * cache internally.  Optimized structures to handle the operations
     * listed are highly recommended.
     *
     * IDs (Entity and site) may be reused.  Deleting an Entity or Site will
     * free up its ID for another to use.  Site IDs start at 0, Entity IDs
     * start at 1.
     *
     * This code must be thread safe.
     */
    class DbBackend
    {
    public:
        /**
         * Default.
         */
        DbBackend(void);

        /**
         * Default destructor.
         */
        virtual ~DbBackend();

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
        virtual std::string get_backend_name(void) =0;

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
            const std::string &name) =0;

        /**
         * @param entity_ptr[in] A pointer to an Entity.
         * @return True if this pointer was created by this DbBackend.  If
         * true, when Entity is to be deleted from memory, you MUST use
         * delete_entity_mem().
         */
        virtual bool entity_mem_owned_by_this(
            const dbtype::Entity *entity_ptr) =0;

        /**
         * Deletes the given entity from memory, if owned by this DbBackend.
         * The Entity will NOT be deleted from the database.
         * @param entity_ptr[in] The entity pointer to delete from memory.
         */
        virtual void delete_entity_mem(dbtype::Entity *entity_ptr) =0;

        /**
         * Gets the Entity from the database.  If the Entity is already
         * present in memory, the existing pointer is returned.
         * Caller must manage pointer and delete it with delete_entity_mem().
         * @param id[in] The ID of the Entity to retrieve.
         * @return A pointer to the Entity retrieved, or null if not found.
         */
        virtual dbtype::Entity *get_entity_db(const dbtype::Id &id) =0;

        /**
         * Determines if the given entity ID exists in the database.
         * @param id[in] The ID to check.
         * @return True if it exists, false if not.
         */
        virtual bool entity_exists_db(const dbtype::Id &id) =0;

        /**
         * Saves the given Entity to the database.  Existing Entity data for
         * that ID and version are overwritten.
         * @param entity_ptr[in] The Entity to save.
         * @return True if success.
         */
        virtual bool save_entity_db(dbtype::Entity *entity_ptr) =0;

        /**
         * Deletes the given Entity from the database.  If the Entity is
         * currently in memory, deletion will fail.
         * @param id[in] The ID of the Entity to delete.
         * @return True if success (or does not exist), false if failure.
         */
        virtual bool delete_entity_db(const dbtype::Id &id) =0;

        /**
         * Deleted entities are included in this query.
         * @param id[in] The ID whose type is to be retrieved.
         * @return The type of the given ID, or 'invalid' if not found.
         */
        virtual dbtype::EntityType get_entity_type_db(const dbtype::Id &id) =0;

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
            const bool exact) =0;

        /**
         * @param site_id[in] The site ID to get all IDs for.
         * @return All valid Entity IDs for the given site, or empty if none
         * or site doesn't exist.
         */
        virtual dbtype::Entity::IdVector find_in_db(
            const dbtype::Id::SiteIdType site_id) =0;

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
            const std::string &registration_name) =0;

        /**
         * Searches for the given ID and determines if a registration
         * is associated with it.
         * @param id[in] The ID to search for.
         * @return The registration name of the Program found, or empty if none
         * found matching or the ID is invalid.
         */
        virtual std::string find_program_reg_name_in_db(
            const dbtype::Id &id) =0;

        /**
         * @return A list of all known site IDs in the database.
         */
        virtual dbtype::Id::SiteIdVector get_site_ids_in_db(void) =0;

        /**
         * Gets the metadata for a single Entity.
         * @param id[in] The ID of the entity to get metadata for.
         * @return The Metadata for the Entity, or invalid if not found.
         */
        virtual EntityMetadata get_entity_metadata(const dbtype::Id &id) =0;

        /**
         * Gets the metadata for a group of Entities.  This will generally
         * be more efficient than getting one at a time.
         * @param ids[in] The IDs of the entities to get metadata for.
         * @return The Metadata for the Entities, or empty if not found.
         * If only a few Entities cannot be found, there will simply not be
         * an entry for them.
         */
        virtual MetadataVector get_entity_metadata(
            const dbtype::Entity::IdVector &ids) =0;

        /**
         * Creates a new site in the database.
         * @param site_id[out] The ID of the site that was created, if success.
         * @return True if successfully created the new site (site_id will be
         * populated if so).
         */
        virtual bool new_site_in_db(dbtype::Id::SiteIdType &site_id) =0;

        /**
         * Deletes a site and all its entities in the database.  The site ID
         * will then be available for reuse.
         * @param site_id[in] The site ID to delete.
         * @return True if success, false if the site ID cannot be found.
         */
        virtual bool delete_site_in_db(const dbtype::Id::SiteIdType site_id) =0;

        /**
         * Gets the name for a site.
         * @param site_id[in] The existing site ID to get the name for.
         * @param site_name[out] The site name, or empty if error or none.
         * @return True if successfully retrieved the site name.
         */
        virtual bool get_site_name_in_db(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_name) =0;

        /**
         * Sets the name for a site.
         * @param site_id[in] The existing site ID to set the name for.
         * @param site_name[in] The site's new name.
         * @return True if successfully set the site name.
         */
        virtual bool set_site_name_in_db(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_name) =0;

        /**
         * Gets the description for a site.
         * @param site_id[in] The existing site ID to get the description for.
         * @param site_description[out] The site description, or empty if
         * error or none.
         * @return True if successfully retrieved the site description.
         */
        virtual bool get_site_description_in_db(
            const dbtype::Id::SiteIdType site_id,
            std::string &site_description) =0;

        /**
         * Sets the description for a site.
         * @param site_id[in] The existing site ID to set the description for.
         * @param site_description[in] The site's new description.
         * @return True if successfully set the site description.
         */
        virtual bool set_site_description_in_db(
            const dbtype::Id::SiteIdType site_id,
            const std::string &site_description) =0;

    protected:
        /**
         * Adds an entity pointer as being owned by this DbBackend.
         * @param entity_ptr[in] The entity pointer to mark as owned.
         * @return True if success, false if already owned.
         */
        bool added_mem_owned(dbtype::Entity *entity_ptr);

        /**
         * Removes an entity pointer as being owned.  The pointer will not be
         * deleted.
         * @param entity_ptr[in] The entity pointer to mark as no longer owned.
         * @return True if success, false if entity pointer is not found.
         */
        bool removed_mem_owned(dbtype::Entity *entity_ptr);

        /**
         * @param entity_ptr[in] The entity pointer to check.
         * @return True if the pointer is owned by this DbBackend.
         */
        bool is_mem_owned(const dbtype::Entity *entity_ptr);

        /**
         * @param id[in] The Entity ID to check.
         * @return True if the Entity is owned by this DbBackend.
         */
        bool is_mem_owned(const dbtype::Id &id);

        /**
         * @param id[in] The ID whose Entity pointer is to be retrieved from
         * the in-memory lookup table.
         * @return The Entity pointer if it is currently in memory, or null
         * if not in memory or not owned by this DbBackend.
         */
        dbtype::Entity *get_entity_pointer(const dbtype::Id &id);

        /**
         * @return True if any pointers are owned by this DbBackend.
         */
        bool any_mem_owned(void);

        /**
         * Given a type, creates a corresponding new Entity in memory only.
         * The version number shall be 0.
         * Caller must manage the pointer.
         * @param type[in] The type of Entity to create.
         * @param id[in] The ID of the Entity.
         * @param owner[in] The owner of the new Entity.
         * @param name[in] The name of the new Entity.
         * @return The pointer to the newly created entity, or null if error
         * or invalid type.
         */
        dbtype::Entity *make_new_entity(
            const dbtype::EntityType type,
            const dbtype::Id &id,
            const dbtype::Id &owner,
            const std::string &name);

        /**
         * Given a type, creates a corresponding new Entity in memory only, and
         * deserializes it.
         * Caller must manage the pointer.
         * @param type[in] The type of Entity to deserialize.
         * @param_type[in,out] The buffer of data to be deserialized.  The
         * position within the buffer is modified.
         * @return The pointer to the newly created and deserialized entity,
         * or null if error or invalid type.
         */
        dbtype::Entity *make_deserialize_entity(
            const dbtype::EntityType type,
            utility::MemoryBuffer &buffer);

        /**
         * Given an Entity, serialize it and place the result into the buffer.
         * @param entity_ptr[in] The Entity to serialize.
         * @param buffer[out] The serialized Entity placed in the buffer.
         * @return True if success, false if error.
         */
        bool serialize_entity(
            dbtype::Entity *entity_ptr,
            utility::MemoryBuffer &buffer);

    private:
        // Map of entity ID to the entity pointer.
        typedef std::map<dbtype::Id::EntityIdType , dbtype::Entity *> OwnedEntityMemMap;
        // Map of Site ID to entities in memory.
        typedef std::map<dbtype::Id::SiteIdType, OwnedEntityMemMap> OwnedSiteMemMap;

        boost::shared_mutex entity_mem_map_mutex; ///< For owned_entity_mem_map
        OwnedSiteMemMap owned_entity_mem_map; ///< Map of ID to entity pointer
    };
}
}

#endif //MUTGOS_DBINTERFACE_DBBACKEND_H
