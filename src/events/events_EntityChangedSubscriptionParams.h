/*
 * events_EntityChangedSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_ENTITYCHANGEDSUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_ENTITYCHANGEDSUBSCRIPTIONPARAMS_H

#include <set>
#include <vector>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_EntityField.h"

#include "events/events_SubscriptionParams.h"
#include "events/events_EntityChangedEvent.h"

namespace mutgos
{
namespace events
{
    /**
     * An entity state changed subscription.  This allows the subscriber to
     * be notified of Entity creates, updates to specific attributes, and
     * deletes.
     *
     * When an attribute of this subscription parameters is left blank,
     * it means 'any'.  When an attribute allows or has more than one
     * entry, it can be assumed all the entries are ORed together (example for
     * entity type:  type ROOM, or type PLAYER, or type PROGRAM).  Parameters
     * are ANDed with each other, though as mentioned prior, entries within
     * parameters are ORed.
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * cannot be unset.
     */
    class EntityChangedSubscriptionParams : public SubscriptionParams
    {
    public:
        /** Collection of EntityTypes */
        typedef std::vector<dbtype::EntityType> EntityTypes;
        /** Collection of EntityActions */
        typedef std::vector<EntityChangedEvent::EntityAction> EntityActions;

        /**
         * Standard constructor.
         */
        EntityChangedSubscriptionParams(void);

        /**
         * Constructor that sets everything.
         * See individual setters for notes on valid contents.
         * @param actions[in] The actions of interest (create, update, etc).
         * Duplicates are automatically removed.
         * @param entities[in] The entities of interest.  Exclusive of sites.
         * @param entities_are_owners[in] True if 'entities' are owners
         * of the entities to watch, false if 'entities' are the actual
         * entities to watch.  When watching by owner, ALL entities owned
         * by the owner are watched.
         * @param site[in] The site of interest.  Exclusive of Entities.
         * @param types[in] The types of Entities of interest.
         * Duplicates are automatically removed.
         * @param fields[in] The Entity changed fields of interest.
         * @param flags_added[in] The Entity flags being added of interest.
         * @param flags_removed[in] The Entity flags being removed of interest.
         * @param field_ids_added[in] The IDs within fields being added of
         * interest.
         * @param field_ids_removed[in] The IDs within fields being removed of
         * interest.
         */
        EntityChangedSubscriptionParams(
            const EntityActions &actions,
            const dbtype::Entity::IdVector &entities,
            const bool entities_are_owners,
            const dbtype::Id::SiteIdType site,
            const EntityTypes &types,
            const dbtype::Entity::EntityFieldSet &fields,
            const dbtype::Entity::FlagSet &flags_added,
            const dbtype::Entity::FlagSet &flags_removed,
            const dbtype::Entity::IdVector &field_ids_added,
            const dbtype::Entity::IdVector &field_ids_removed);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        EntityChangedSubscriptionParams(
            const EntityChangedSubscriptionParams &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~EntityChangedSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        EntityChangedSubscriptionParams &operator=(
            const EntityChangedSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const EntityChangedSubscriptionParams &rhs) const;

        /**
         * Adds an entity action of interest, if not already added.
         * @param action[in] The action interested in.
         */
        void add_entity_action(const EntityChangedEvent::EntityAction action);

        /**
         * @return The entity actions interested in.
         */
        const EntityActions &get_entity_actions(void) const
          { return entity_actions; }

        /**
         * Adds an Entity ID to the list of interested IDs.  If none are added,
         * all IDs are assumed.  This cannot be populated when site IDs are
         * populated.  Invalid IDs are allowed, but will never match.
         * @param id[in] The ID to add.
         */
        void add_entity_id(const dbtype::Id &id)
          { entity_ids.push_back(id); }

        /**
         * @return The interested Entity IDs.
         */
        const dbtype::Entity::IdVector &get_entity_ids(void) const
          { return entity_ids; }

        /**
         * @return True if the entity IDs are actually the owners rather
         * than the entities themselves.
         */
        const bool get_entities_are_owners(void) const
          { return entity_ids_owners; }

        /**
         * Sets a site ID of interest.  If not set or set to default (0),
         * all sites are assumed (entity ID filter is then used).  This cannot
         * be populated when entity IDs are populated.  Invalid IDs are allowed,
         * but will never match.
         * @param site_id[in] The site ID to set.
         */
        void set_site_id(const dbtype::Id::SiteIdType &site_id)
          { entity_site_id = site_id; }

        /**
         * @return The interested site ID, or default (0) for all.
         */
        const dbtype::Id::SiteIdType get_site_id(void) const
          { return entity_site_id; }

        /**
         * Adds an entity type to the list of interested types, if not already
         * added.  If none are added, all types are assumed.
         * Invalid types (non-leaf types, the END enum etc) are allowed, but
         * will never match anything.
         * @param entity_type[in] The entity type to add.
         */
        void add_entity_type(const dbtype::EntityType entity_type);

        /**
         * @return The interested entity types.
         */
        const EntityTypes &get_entity_types(void) const
          { return entity_types; }

        /**
         * Adds an entity field to the list of interested types.  If none are
         * added, all fields are assumed.
         * Invalid fields are allowed (the END enum, separator enums, etc),
         * but will never match anything.
         * @param field[in] The entity field to add.
         */
        void add_entity_field(const dbtype::EntityField field)
          { entity_fields.insert(field); }

        /**
         * @return The interested entity fields.
         */
        const dbtype::Entity::EntityFieldSet &get_entity_fields(void) const
          { return entity_fields; }

        /**
         * Adds a flag that is interested in knowing when it is added to
         * an Entity.
         * Invalid flags are allowed, but will never match anything.
         * @param flag[in] A flag that is added to an Entity.
         */
        void add_entity_flag_added(const dbtype::Entity::FlagType &flag)
          { entity_flags_added.insert(flag); }

        /**
         * @return The flags that are being added to an Entity.
         */
        const dbtype::Entity::FlagSet&get_entity_flags_added(void) const
          { return entity_flags_added; }

        /**
         * Adds a flag that is interested in knowing when it is removed from
         * an Entity.
         * Invalid flags are allowed, but will never match anything.
         * @param flag[in] A flag that is removed from an Entity.
         */
        void add_entity_flag_removed(const dbtype::Entity::FlagType &flag)
          { entity_flags_removed.insert(flag); }

        /**
         * @return The flags that are being removed from an Entity.
         */
        const dbtype::Entity::FlagSet&get_entity_flags_removed(void) const
          { return entity_flags_removed; }

        /**
         * Adds an ID that is interested in knowing when it is added to any
         * ID field of an Entity.  This can be filtered by adding
         * interested Entity fields.
         * Invalid IDs are allowed, but will never match anything.
         * @see add_entity_field()
         * @param id[in] An ID that is added to any ID field of an Entity.
         */
        void add_entity_field_ids_added(const dbtype::Id &id)
          { entity_field_ids_added.push_back(id); }

        /**
         * @return IDs that are being added to any ID field of an Entity.
         */
        const dbtype::Entity::IdVector &get_entity_field_ids_added(void) const
          { return entity_field_ids_added; }

        /**
         * Adds an ID that is interested in knowing when it is removed from any
         * ID field of an Entity.  This can be filtered by adding
         * interested Entity fields.
         * Invalid IDs are allowed, but will never match anything.
         * @see add_entity_field()
         * @param id[in] An ID that is removed from any ID field of an Entity.
         */
        void add_entity_field_ids_removed(const dbtype::Id &id)
          { entity_field_ids_removed.push_back(id); }

        /**
         * @return IDs that are being removed from any ID field of an Entity.
         */
        const dbtype::Entity::IdVector &get_entity_field_ids_removed(void) const
          { return entity_field_ids_removed; }

        /**
         * Validates that the subscription is valid (has all needed fields
         * filled in and that they are properly filled in.
         * @return True if subscription is valid, false if there is a problem.
         */
        virtual bool validate(void) const;

        /**
         * @return A copy of this subscription.  Caller is responsible for
         * managing the pointer.
         */
        virtual SubscriptionParams *clone(void) const;

        /**
         * @param id[in] The ID to check.
         * @return True if the subscription parameters specifically reference
         * the given ID anywhere.
         */
        virtual bool references_id(const dbtype::Id &id) const;

        /**
         * @param site_id[in] The site ID to check.
         * @return True if the subscription parameters specifically reference
         * the given site ID anywhere, including in entity IDs.
         */
        virtual bool references_site(
            const dbtype::Id::SiteIdType site_id) const;

        /**
         * @return The subscription as a string, for diagnostic/logging
         * purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * Evaluates the event and determine if it matches this subscription.
         * @param event_ptr[in] The event to evaluate.
         * @return True if subscription matches event.
         */
        virtual bool is_match(const EntityChangedEvent *event_ptr) const;

    private:
        EntityActions entity_actions; ///< Actions of interest
        dbtype::Entity::IdVector entity_ids; ///< Entity IDs of interest
        bool entity_ids_owners; ///< True if entity_ids are owners instead
        dbtype::Id::SiteIdType entity_site_id;  ///< Site ID of interest
        EntityTypes entity_types; ///< Entity types of interest
        dbtype::Entity::EntityFieldSet entity_fields; ///< Entity changed fields of interest
        dbtype::Entity::FlagSet entity_flags_added; ///< Entity flags being added of interest
        dbtype::Entity::FlagSet entity_flags_removed; ///< Entity flags being removed of interest
        dbtype::Entity::IdVector entity_field_ids_added; ///< Entity IDs added in any field of interest
        dbtype::Entity::IdVector entity_field_ids_removed; ///< Entity IDs removed in any field of interest
    };
}
}

#endif //MUTGOS_EVENTS_ENTITYCHANGEDSUBSCRIPTIONPARAMS_H
