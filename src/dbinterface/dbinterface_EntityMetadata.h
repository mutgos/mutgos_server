/*
 * dbinterface_EntityMetadata.h
 */

#ifndef MUTGOS_DBINTERFACE_ENTITYMETADATA_H
#define MUTGOS_DBINTERFACE_ENTITYMETADATA_H

#include <string>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * Simple container class that has basic metadata about an Entity.
     * Used as a much quicker and more efficient alternative than loading
     * the entire Entity into memory.
     */
    class EntityMetadata
    {
    public:
        /**
         * Constructor that makes an invalid metadata.
         */
        EntityMetadata(void)
            : entity_type(dbtype::ENTITYTYPE_invalid),
              entity_version(0)
        {
        }

        /**
         * Constructor that sets everything.
         * @param id[in] The Entity ID of the Entity.
         * @param owner[in] The owner of the Entity.
         * @param type[in] The type of the Entity.
         * @param version[in] The version of the Entity.
         * @param name[in] The name of the Entity.
         */
        EntityMetadata(
            const dbtype::Id &id,
            const dbtype::Id &owner,
            const dbtype::EntityType type,
            const dbtype::Entity::VersionType version,
            const std::string &name)
              : entity_id(id),
                owner_id(owner),
                entity_type(type),
                entity_version(version),
                entity_name(name)
        {
        }

        /**
         * Method that (re)sets everything.
         * @param id[in] The Entity ID of the Entity.
         * @param owner[in] The owner of the Entity.
         * @param type[in] The type of the Entity.
         * @param version[in] The version of the Entity.
         * @param name[in] The name of the Entity.
         */
        void set(
            const dbtype::Id &id,
            const dbtype::Id &owner,
            const dbtype::EntityType type,
            const dbtype::Entity::VersionType version,
            const std::string &name)
        {
            entity_id = id;
            owner_id = owner;
            entity_type = type;
            entity_version = version;
            entity_name = name;
        }

        /**
         * Resets metadata to invalid.
         */
        void reset(void)
        {
            entity_id = dbtype::Id();
            owner_id = dbtype::Id();
            entity_type = dbtype::ENTITYTYPE_invalid;
            entity_version = 0;
            entity_name.clear();
        }

        /**
         * @return True if EntityMetadata contains valid data.
         */
        bool valid(void) const
          { return entity_type != dbtype::ENTITYTYPE_invalid; }

        /**
         * @return The Entity ID.
         */
        const dbtype::Id &get_id(void) const
          { return entity_id; }

        /**
         * @return The owner ID.
         */
        const dbtype::Id &get_owner(void) const
          { return owner_id; }

        /**
         * @return The Entity type.
         */
        dbtype::EntityType get_type(void) const
          { return entity_type; }

        /**
         * @return The Entity version.
         */
        dbtype::Entity::VersionType get_version(void) const
          { return entity_version; }

        /**
         * @return The Entity name.
         */
        const std::string &get_name(void) const
          { return entity_name; }

    private:
        dbtype::Id entity_id;  ///< Entity ID of the Entity
        dbtype::Id owner_id;  ///< Owner of the Entity
        dbtype::EntityType entity_type;  ///< Type of the Entity
        dbtype::Entity::VersionType entity_version; ///< Version of the Entity
        std::string entity_name; ///< Name of the Entity
    };
}
}

#endif //MUTGOS_DBINTERFACE_ENTITYMETADATA_H
