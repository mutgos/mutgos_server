/*
 * primitives_NameRegistry.h
 */

#ifndef MUTGOS_PRIMITIVES_NAMEREGISTRY_H
#define MUTGOS_PRIMITIVES_NAMEREGISTRY_H

#include <string>
#include <map>
#include <vector>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Entity.h"

#include <boost/thread/shared_mutex.hpp>


namespace mutgos
{
namespace primitives
{
    /**
     * Container class that stores the result of a search, but is also used
     * as a bulk add container.
     */
    class NameRegistryResult
    {
    public:
        /**
         * Constructor that sets the parameters for adding
         * Entities in batch.
         * @param entity_name[in]  Name of the Entity.
         * @param entity_id[in]  Entity ID.
         * @param entity_type[in] Type of Entity.
         */
        NameRegistryResult(
            const std::string &entity_name,
            const dbtype::Id &entity_id,
            const dbtype::EntityType &entity_type) :
            name(entity_name),
            id(entity_id),
            type(entity_type),
            exact_match(true)
        { }

        /**
         * Constructor that sets everything for use as a result to a search.
         * @param entity_name[in]  Name of the Entity.
         * @param entity_id[in]  Entity ID.
         * @param entity_type[in] Type of Entity.
         * @param entity_exact_match[in] True if exact match.
         */
        NameRegistryResult(
            const std::string &entity_name,
            const dbtype::Id &entity_id,
            const dbtype::EntityType &entity_type,
            const bool entity_exact_match) :
                name(entity_name),
                id(entity_id),
                type(entity_type),
                exact_match(entity_exact_match)
        { }

        std::string name; ///< Full name of Entity
        dbtype::Id id; ///< Entity ID
        dbtype::EntityType type; ///< Type of Entity
        bool exact_match; ///< True if it was an exact match
    };

    typedef NameRegistryResult NameRegistryInfo;

    /**
     * Stores the names and IDs of every online player and puppet.
     * This is used primarily to do searches by name (full and partial)
     * of online players or puppets.
     *
     * Note this class does not populate itself.  An external process
     * does that work.
     */
    class NameRegistry
    {
    public:
        typedef std::vector<NameRegistryResult> ResultVector;

        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static NameRegistry *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static NameRegistry *instance(void)
        { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists.
         */
        static void destroy_singleton(void);

        /**
         * Searches for a name by a prefix.
         * @param site[in] The site to search in.
         * @param prefix[in] Prefix to search for.  Case insensitive.
         * @param type[in] The type to search for.  Can be Entity (all),
         * Player, or Puppet.
         * @return The matching Entities, or empty if none found.
         */
        ResultVector search_by_prefix(
            const dbtype::Id::SiteIdType site,
            const std::string &prefix,
            const dbtype::EntityType type);

        /**
         * Searches for a name by exact match.
         * @param site[in] The site to search in.
         * @param name[in] Exact name to search for.  Case insensitive.
         * @param type[in] The type to search for.  Can be Entity (all),
         * Player, or Puppet.
         * @return The matching Entities, or empty if none found.
         */
        ResultVector search_by_exact(
            const dbtype::Id::SiteIdType site,
            const std::string &name,
            const dbtype::EntityType type);

        /**
         * Adds a single Entity to the registry.  Use multiple-add version
         * for more than one Entity.
         * @param entity_info[in] The Entity to add.
         * @return True if success (no entry already existed, and all had
         * needed data filled in).
         */
        bool add_entity(const NameRegistryInfo &entity_info);

        /**
         * Adds multiple Entities to the registry.  This is more efficient
         * than adding one at a time.
         * @param entities[in] The Entities to add.  All must be from the
         * same site.
         * @return True if success (no entry already existed, and all had
         * needed data filled in).
         */
        bool add_entity(const std::vector<NameRegistryInfo> &entities);

        /**
         * Used when an existing Entity is renamed, this will update
         * the registry with the new name.
         * @param id[in] The ID of the Entity being renamed.
         * @param new_name
         * @return True if ID found and the name updated, and all needed
         * data was filled in.
         */
        bool update_entity_name(
            const dbtype::Id &id,
            const dbtype::EntityType type,
            const std::string &new_name);

        /**
         * Removes the given Entities from the registry.
         * @param ids[in] The IDs to remove.  All must be from the same site.
         * @param type[in] The types of the IDs to remove.  All IDs must
         * be of this type; the begin() Entity is used to determine the
         * site ID.
         * @return True if all IDs found and removed, false otherwise.
         */
        bool remove_entity(
            const dbtype::Entity::IdSet &ids,
            const dbtype::EntityType type);

        /**
         * Removes a single Entity from the registry.
         * @param id[in] The ID to remove.
         * @param type[in] The type of the ID to remove.
         * @return True if ID found and removed, false otherwise.
         */
        bool remove_entity(
            const dbtype::Id &id,
            const dbtype::EntityType type);

    private:
        /**
         * Singleton constructor.
         */
        NameRegistry(void);

        /**
         * Container class that stores info about an Entity and its name.
         */
        class NameInfo
        {
        public:
            /**
             * Constructor that sets everything.
             * @param entity_id[in]  Entity ID.
             * @param entity_name[in]  Exact name of the Entity.
             * @param entity_normalized_name[in] Normalized name of the
             * Entity for easy searching.
             */
            NameInfo(
                const dbtype::Id &entity_id,
                const std::string &entity_name,
                const std::string &entity_normalized_name) :
                id(entity_id),
                name(entity_name),
                normalized_name(entity_normalized_name)
            { }

            dbtype::Id id; ///< ID of Entity
            std::string name; ///< Name as in DB
            std::string normalized_name; ///< Normalized name for fast searching
        };

        typedef std::vector<NameInfo> NameInfoVector;
        // First is online players, second is online puppets
        typedef std::pair<NameInfoVector, NameInfoVector>
            PlayersPuppetsPair;
        // Site to online names for that site
        typedef std::map<dbtype::Id::SiteIdType, PlayersPuppetsPair>
            SiteToNamesMap;

        boost::shared_mutex mutex;  ///< Shared lock for names

        SiteToNamesMap names; ///< Maps Sites to the Entities and their associated name

        static NameRegistry *singleton_ptr; ///< Singleton pointer.

        // TODO: Process will subscribe to puppet and player creates, deletes, and name changes (2 subs)
        // TODO: Process will subscribe to connects and disconnects (1 sub)
    };
}
}

#endif //MUTGOS_PRIMITIVES_NAMEREGISTRY_H
