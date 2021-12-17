/*
 * events_EntityChangedEvent.h
 */

#ifndef MUTGOS_EVENTS_ENTITYCHANGEDEVENT_H
#define MUTGOS_EVENTS_ENTITYCHANGEDEVENT_H

#include "events/events_Event.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Entity.h"


namespace mutgos
{
namespace events
{
    /**
     * Represents an Entity in the database being created, updated, or deleted.
     * This is expected to be a very frequent event.
     * Not all fields will be populated for every event.
     */
    class EntityChangedEvent : public Event
    {
    public:
        /** Enum that indicates what has occurred to an Entity in this event */
        enum EntityAction
        {
            /** Entity has been created */
            ENTITY_CREATED,
            /** One or more fields on Entity have been updated */
            ENTITY_UPDATED,
            /** Entity has been deleted */
            ENTITY_DELETED
        };

        /**
         * Constructor to set attributes of event when Entity has been
         * updated.
         * @param id[in] The ID of the Entity which changed.
         * @param type[in] The type of the Entity which changed.
         * @param owner[in] The owner of id.
         * @param fields_changed[in] The fields of the Entity which have
         * changed.  This should not be empty for this constructor unless
         * only flags have changed.
         * @param flags_changed[in] The flags of the Entity which have
         * changed.  This may be empty.
         * @param id_fields_changed[in] Details on which IDs have been added
         * or removed for changed fields which have IDs.  No field listed
         * here should be missing from fields_changed.
         */
        EntityChangedEvent(
            const dbtype::Id &id,
            const dbtype::EntityType type,
            const dbtype::Id owner,
            const dbtype::Entity::EntityFieldSet &fields_changed,
            const dbtype::Entity::FlagsRemovedAdded &flags_changed,
            const dbtype::Entity::ChangedIdFieldsMap &id_fields_changed)
            :  Event(Event::EVENT_ENTITY_CHANGED),
               entity_id(id),
               entity_type(type),
               entity_owner(owner),
               entity_action(EntityChangedEvent::ENTITY_UPDATED),
               entity_fields_changed(fields_changed),
               entity_flags_changed(flags_changed),
               entity_id_fields_changed(id_fields_changed)
        {  }

        /**
         * Constructor used when event indicates an Entity has been created
         * or deleted.
         * @param id[in] The ID of the Entity which has been created or deleted.
         * @param type[in] The type of the Entity which was created or deleted.
         * @param owner[in] The owner of id.
         * @param action[in] The action which occurred to the Entity (create,
         * delete, etc).
         */
        EntityChangedEvent(
            const dbtype::Id &id,
            const dbtype::EntityType type,
            const dbtype::Id owner,
            const EntityAction action)
            :  Event(Event::EVENT_ENTITY_CHANGED),
               entity_id(id),
               entity_type(type),
               entity_owner(owner),
               entity_action(action)
        { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        EntityChangedEvent(const EntityChangedEvent &rhs)
            : Event(rhs),
              entity_id(rhs.entity_id),
              entity_type(rhs.entity_type),
              entity_owner(rhs.entity_owner),
              entity_action(rhs.entity_action),
              entity_fields_changed(rhs.entity_fields_changed),
              entity_flags_changed(rhs.entity_flags_changed),
              entity_id_fields_changed(rhs.entity_id_fields_changed)
        { }

        /**
         * Required virtual destructor.
         */
        virtual ~EntityChangedEvent()
          { }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return The ID of the Entity this event is about.
         */
        const dbtype::Id &get_entity_id(void) const
          { return entity_id; }

        /**
         * @return The type of the Entity this event is about.
         */
        const dbtype::EntityType get_entity_type(void) const
          { return entity_type; }

        /**
         * @return The entity's owner.
         */
        dbtype::Id get_entity_owner(void) const
          { return entity_owner; }

        /**
         * @return The action occuring to the Entity that this event is about.
         * This will determine what other fields will be populated.
         */
        const EntityAction get_entity_action(void) const
          { return entity_action; }

        /**
         * @return The fields which changed, if the action type is update.
         */
        const dbtype::Entity::EntityFieldSet &get_entity_fields_changed(
            void) const
          { return entity_fields_changed; }

        /**
         * @return The flags on the Entity which changed, if the action type
         * is update.
         */
        const dbtype::Entity::FlagsRemovedAdded &get_entity_flags_changed(
            void) const
          { return entity_flags_changed; }

        /**
         * @return Detailed information about fields of type 'ID' (including
         * lists and sets) which indicate which IDs have been added, removed,
         * or replaced.
         */
        const dbtype::Entity::ChangedIdFieldsMap &get_entity_id_fields_changed(
            void) const
          { return entity_id_fields_changed; }

    private:
        const dbtype::Id entity_id; ///< ID of Entity this event is about
        const dbtype::EntityType entity_type; ///< Type of Entity this event is about
        const dbtype::Id entity_owner; ///< Owner of entity_id
        const EntityAction entity_action; ///< What happened to Entity
        const dbtype::Entity::EntityFieldSet entity_fields_changed; ///< Fields changed on Entity
        const dbtype::Entity::FlagsRemovedAdded entity_flags_changed; ///< Flags changed on Entity
        const dbtype::Entity::ChangedIdFieldsMap entity_id_fields_changed; ///< Details what IDs changed on ID fields
    };
}
}

#endif //MUTGOS_EVENTS_ENTITYCHANGEDEVENT_H
