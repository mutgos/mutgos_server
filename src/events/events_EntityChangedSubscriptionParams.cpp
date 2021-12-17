/*
 * events_EntityChangedSubscriptionParams.cpp
 */

#include <string>
#include <ostream>
#include <sstream>
#include <algorithm>

#include <boost/foreach.hpp>

#include "events/events_EntityChangedSubscriptionParams.h"

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
    // ----------------------------------------------------------------------
    EntityChangedSubscriptionParams::EntityChangedSubscriptionParams(void)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_ENTITY_CHANGED),
        entity_ids_owners(false),
        entity_site_id(0)
    {
    }

    // ----------------------------------------------------------------------
    EntityChangedSubscriptionParams::EntityChangedSubscriptionParams(
        const EntityChangedSubscriptionParams::EntityActions &actions,
        const dbtype::Entity::IdVector &entities,
        const bool entities_are_owners,
        const dbtype::Id::SiteIdType site,
        const EntityChangedSubscriptionParams::EntityTypes &types,
        const dbtype::Entity::EntityFieldSet &fields,
        const dbtype::Entity::FlagSet &flags_added,
        const dbtype::Entity::FlagSet &flags_removed,
        const dbtype::Entity::IdVector &field_ids_added,
        const dbtype::Entity::IdVector &field_ids_removed)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_ENTITY_CHANGED),
        entity_actions(actions),
        entity_ids(entities),
        entity_ids_owners(entities_are_owners),
        entity_site_id(site),
        entity_types(types),
        entity_fields(fields),
        entity_flags_added(flags_added),
        entity_flags_removed(flags_removed),
        entity_field_ids_added(field_ids_added),
        entity_field_ids_removed(field_ids_removed)
    {
        // Remove duplicates for actions and types, per
        // https://stackoverflow.com/a/1041939
        //
        std::sort(entity_actions.begin(), entity_actions.end());
        entity_actions.erase(
            std::unique(entity_actions.begin(), entity_actions.end()),
            entity_actions.end());

        std::sort(entity_types.begin(), entity_types.end());
        entity_types.erase(
            std::unique(entity_types.begin(), entity_types.end()),
            entity_types.end());
    }

    // ----------------------------------------------------------------------
    EntityChangedSubscriptionParams::EntityChangedSubscriptionParams(
        const EntityChangedSubscriptionParams &rhs)
      : SubscriptionParams(rhs),
        entity_actions(rhs.entity_actions),
        entity_ids(rhs.entity_ids),
        entity_ids_owners(rhs.entity_ids_owners),
        entity_site_id(rhs.entity_site_id),
        entity_types(rhs.entity_types),
        entity_fields(rhs.entity_fields),
        entity_flags_added(rhs.entity_flags_added),
        entity_flags_removed(rhs.entity_flags_removed),
        entity_field_ids_added(rhs.entity_field_ids_added),
        entity_field_ids_removed(rhs.entity_field_ids_removed)
    {
    }

    // ----------------------------------------------------------------------
    EntityChangedSubscriptionParams::~EntityChangedSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    EntityChangedSubscriptionParams& EntityChangedSubscriptionParams::operator=(
        const EntityChangedSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);

        entity_actions = rhs.entity_actions;
        entity_ids = rhs.entity_ids;
        entity_ids_owners = rhs.entity_ids_owners;
        entity_site_id = rhs.entity_site_id;
        entity_types = rhs.entity_types;
        entity_fields = rhs.entity_fields;
        entity_flags_added = rhs.entity_flags_added;
        entity_flags_removed = rhs.entity_flags_removed;
        entity_field_ids_added = rhs.entity_field_ids_added;
        entity_field_ids_removed = rhs.entity_field_ids_removed;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool EntityChangedSubscriptionParams::operator==(
        const EntityChangedSubscriptionParams &rhs) const
    {
        bool equals = false;

        if (SubscriptionParams::params_equal_to(rhs))
        {
            equals = (entity_actions == rhs.entity_actions) and
                (entity_ids == rhs.entity_ids) and
                (entity_ids_owners == rhs.entity_ids_owners) and
                (entity_site_id == rhs.entity_site_id) and
                (entity_types == rhs.entity_types) and
                (entity_fields == rhs.entity_fields) and
                (entity_flags_added == rhs.entity_flags_added) and
                (entity_flags_removed == rhs.entity_flags_removed) and
                (entity_field_ids_added == rhs.entity_field_ids_added) and
                (entity_field_ids_removed == rhs.entity_field_ids_removed);
        }

        return equals;
    }

    // ----------------------------------------------------------------------
    void EntityChangedSubscriptionParams::add_entity_action(
        const EntityChangedEvent::EntityAction action)
    {
        if (std::find(entity_actions.begin(), entity_actions.end(), action) ==
            entity_actions.end())
        {
            entity_actions.push_back(action);
            std::sort(entity_actions.begin(), entity_actions.end());
        }
    }

    // ----------------------------------------------------------------------
    void EntityChangedSubscriptionParams::add_entity_type(
        const dbtype::EntityType entity_type)
    {
        if (std::find(entity_types.begin(), entity_types.end(), entity_type) ==
            entity_types.end())
        {
            entity_types.push_back(entity_type);
            std::sort(entity_types.begin(), entity_types.end());
        }
    }

    // ----------------------------------------------------------------------
    bool EntityChangedSubscriptionParams::validate(void) const
    {
        const bool valid = (entity_ids.empty() and (not entity_site_id)) or
            (entity_ids.empty() != (not entity_site_id));

        return valid;
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *EntityChangedSubscriptionParams::clone(void) const
    {
        return new EntityChangedSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool EntityChangedSubscriptionParams::references_id(
        const dbtype::Id &id) const
    {
        return has_entity_id(id, entity_ids) or
            has_entity_id(id, entity_field_ids_added) or
            has_entity_id(id, entity_field_ids_removed);
    }

    // ----------------------------------------------------------------------
    bool EntityChangedSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return (site_id == entity_site_id) or
               has_site_id(site_id, entity_ids) or
               has_site_id(site_id, entity_field_ids_added) or
               has_site_id(site_id, entity_field_ids_removed);
    }

    // ----------------------------------------------------------------------
    std::string EntityChangedSubscriptionParams::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "EntityChangedSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << "entity actions:   ";

        BOOST_FOREACH(const EntityChangedEvent::EntityAction &action, entity_actions)
        {
            strstream << action << "  ";
        }

        strstream << std::endl
                  << "entity IDs:       ";

        BOOST_FOREACH(const dbtype::Id &id, entity_ids)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "entity IDs are owners: " << entity_ids_owners;

        strstream << std::endl
                  << "site ID:          " << entity_site_id
                  << std::endl
                  << "entity types:     ";

        BOOST_FOREACH(const dbtype::EntityType &type, entity_types)
        {
            strstream << dbtype::entity_type_to_string(type) << "  ";
        }

        strstream << std::endl
                  << "entity fields:    ";

        BOOST_FOREACH(const dbtype::EntityField &field, entity_fields)
        {
            strstream << dbtype::entity_field_to_string(field) << "  ";
        }

        strstream << std::endl
                  << "entity flags add: ";

        BOOST_FOREACH(const dbtype::Entity::FlagType &flag, entity_flags_added)
        {
            strstream << "\"" << flag << "\"  ";
        }

        strstream << std::endl
                  << "entity flags del: ";

        BOOST_FOREACH(const dbtype::Entity::FlagType &flag, entity_flags_removed)
        {
            strstream << "\"" << flag << "\"  ";
        }

        strstream << std::endl
                  << "field IDs add:    ";

        BOOST_FOREACH(const dbtype::Id &id, entity_field_ids_added)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "field IDs del:    ";

        BOOST_FOREACH(const dbtype::Id &id, entity_field_ids_removed)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool EntityChangedSubscriptionParams::is_match(
        const EntityChangedEvent *event_ptr) const
    {
        if (not event_ptr)
        {
            return false;
        }

        bool match = true;

        if (not entity_actions.empty())
        {
            match = std::find(
                entity_actions.begin(),
                entity_actions.end(),
                event_ptr->get_entity_action()) != entity_actions.end();
        }

        if (match)
        {
            if (not entity_ids.empty())
            {
                if (entity_ids_owners)
                {
                    match = has_entity_id(
                        event_ptr->get_entity_owner(),
                        entity_ids);
                }
                else
                {
                    match = has_entity_id(
                        event_ptr->get_entity_id(),
                        entity_ids);
                }
            }
            else if (entity_site_id)
            {
                match = (entity_site_id ==
                    event_ptr->get_entity_id().get_site_id());
            }
        }

        if (match and (not entity_types.empty()))
        {
            match = std::find(
                entity_types.begin(),
                entity_types.end(),
                event_ptr->get_entity_type()) != entity_types.end();
        }

        if (match and (not entity_fields.empty()))
        {
            // It's possible entity_fields is smaller than the number of
            // fields changed, or the other way around.  Iterate over the
            // shortest.
            //
            const dbtype::Entity::EntityFieldSet &event_fields =
                event_ptr->get_entity_fields_changed();
            const bool param_smaller = entity_fields.size() <=
                event_fields.size();
            bool found_match = false;

            for (dbtype::Entity::EntityFieldSet::const_iterator iter =
                   (param_smaller ? entity_fields.begin() : event_fields.begin());
                 iter != (param_smaller ? entity_fields.end() : event_fields.end());
                ++iter)
            {
                if (param_smaller)
                {
                    if (event_fields.find(*iter) != event_fields.end())
                    {
                        found_match = true;
                        break;
                    }
                }
                else
                {
                    if (entity_fields.find(*iter) != entity_fields.end())
                    {
                        found_match = true;
                        break;
                    }
                }
            }

            match = found_match;
        }

        if (match and (not entity_flags_added.empty()))
        {
            // The event list of added flags is likely to be shorter.
            //
            const dbtype::Entity::FlagSet &flags_added =
                event_ptr->get_entity_flags_changed().second;
            bool found_match = false;

            for (dbtype::Entity::FlagSet::const_iterator iter =
                    flags_added.begin();
                iter != flags_added.end();
                ++iter)
            {
                if (entity_flags_added.find(*iter) != entity_flags_added.end())
                {
                    found_match = true;
                    break;
                }
            }

            match = found_match;
        }

        if (match and (not entity_flags_removed.empty()))
        {
            // The event list of removed flags is likely to be shorter.
            //
            const dbtype::Entity::FlagSet &flags_removed =
                event_ptr->get_entity_flags_changed().first;
            bool found_match = false;

            for (dbtype::Entity::FlagSet::const_iterator iter =
                    flags_removed.begin();
                 iter != flags_removed.end();
                 ++iter)
            {
                if (entity_flags_removed.find(*iter) !=
                    entity_flags_removed.end())
                {
                    found_match = true;
                    break;
                }
            }

            match = found_match;
        }

        if (match and (not entity_field_ids_added.empty()))
        {
            // The event list of added IDs is likely to be shorter, but first
            // we must combine them into one set.
            //
            const dbtype::Entity::ChangedIdFieldsMap &event_ids_changed =
                event_ptr->get_entity_id_fields_changed();
            dbtype::Entity::IdSet ids_added;
            bool found_match = false;

            for (dbtype::Entity::ChangedIdFieldsMap::const_iterator event_iter =
                    event_ids_changed.begin();
                event_iter != event_ids_changed.end();
                ++event_iter)
            {
                ids_added.insert(
                    event_iter->second.second.begin(),
                    event_iter->second.second.end());
            }

            for (dbtype::Entity::IdSet::const_iterator iter = ids_added.begin();
                iter != ids_added.end();
                ++iter)
            {
                if (has_entity_id(*iter, entity_field_ids_added))
                {
                    found_match = true;
                    break;
                }
            }

            match = found_match;
        }

        if (match and (not entity_field_ids_removed.empty()))
        {
            // The event list of removed IDs is likely to be shorter, but first
            // we must combine them into one set.
            //
            const dbtype::Entity::ChangedIdFieldsMap &event_ids_changed =
                event_ptr->get_entity_id_fields_changed();
            dbtype::Entity::IdSet ids_removed;
            bool found_match = false;

            for (dbtype::Entity::ChangedIdFieldsMap::const_iterator event_iter =
                    event_ids_changed.begin();
                 event_iter != event_ids_changed.end();
                 ++event_iter)
            {
                ids_removed.insert(
                    event_iter->second.first.begin(),
                    event_iter->second.first.end());
            }

            for (dbtype::Entity::IdSet::const_iterator iter =
                    ids_removed.begin();
                 iter != ids_removed.end();
                 ++iter)
            {
                if (has_entity_id(*iter, entity_field_ids_removed))
                {
                    found_match = true;
                    break;
                }
            }

            match = found_match;
        }

        return match;
    }
}
}
