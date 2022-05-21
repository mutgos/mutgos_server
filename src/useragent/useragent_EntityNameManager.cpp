/*
 * useragent_EntityNameManager.cpp
 */

#include <string>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "executor/executor_ProcessServices.h"
#include "executor/executor_Process.h"
#include "executor/executor_ExecutorAccess.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "primitives/primitives_NameRegistry.h"

#include "events/events_EventAccess.h"
#include "events/events_SubscriptionCallback.h"
#include "events/events_EventMatchedMessage.h"
#include "events/events_ConnectionSubscriptionParams.h"
#include "events/events_ConnectionEvent.h"
#include "events/events_EntityChangedEvent.h"
#include "events/events_EntityChangedSubscriptionParams.h"

#include "useragent/useragent_EntityNameManager.h"

namespace
{
    const std::string PROCESS_NAME = "Entity Name Manager";
}

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    EntityNameManager::EntityNameManager(void)
      : my_pid(0)
    {
    }

    // ----------------------------------------------------------------------
    EntityNameManager::~EntityNameManager()
    {
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::process_added(
        const mutgos::executor::PID pid,
        mutgos::executor::ProcessServices &services)
    {
        my_pid = pid;
        subscribe_events();
    }

    // ----------------------------------------------------------------------
    EntityNameManager::ProcessStatus
    EntityNameManager::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // We are event-driven only, so wait for the next message.
        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    EntityNameManager::ProcessStatus
    EntityNameManager::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;

        switch (message.message_get_type())
        {
            case executor::ProcessMessage::MESSAGE_EVENT:
            {
                events::EventMatchedMessage const * event_matched_ptr =
                    dynamic_cast<events::EventMatchedMessage *>(&message);

                if (event_matched_ptr)
                {
                    switch (event_matched_ptr->get_event_type())
                    {
                        case events::Event::EVENT_CONNECTION:
                        {
                            process_connection_event(
                                dynamic_cast<events::ConnectionEvent *>(
                                    & event_matched_ptr->get_event()));

                            break;
                        }

                        case events::Event::EVENT_ENTITY_CHANGED:
                        {
                            process_entity_event(
                                dynamic_cast<events::EntityChangedEvent *>(
                                    & event_matched_ptr->get_event()));

                            break;
                        }

                        default:
                        {
                            LOG(error, "useragent", "process_execute(message)",
                                "Unknown event type: " + text::to_string(
                                    event_matched_ptr->get_event_type()));
                        }
                    }
                }

                break;
            }

            case executor::ProcessMessage::MESSAGE_SUBSCRIPTION_DELETED:
            {
                // We should never get this.  If we do, abort.
                //
                LOG(error, "useragent", "process_execute(message)",
                    "Our subscription was deleted!  Shutting down process...");
                status = PROCESS_STATUS_FINISHED;

                break;
            }

            default:
            {
                LOG(error, "useragent", "process_execute(message)",
                    "Unknown message type: "
                    + text::to_string(message.message_get_type()));
            }
        }

        return status;
    }

    // ----------------------------------------------------------------------
    std::string EntityNameManager::process_get_name(
        const executor::PID pid)
    {
        return PROCESS_NAME;
    }

    // ----------------------------------------------------------------------
    bool EntityNameManager::process_delete_when_finished(
        const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    EntityNameManager::ErrorMessageText
    EntityNameManager::process_get_error_text(const executor::PID pid)
    {
        // Not supported.
        return ErrorMessageText();
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::process_killed(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::process_finished(const executor::PID pid)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::subscribe_events(void)
    {
        // Subscribe to all connections and disconnections
        events::ConnectionSubscriptionParams connection_params;
        events::SubscriptionCallback callback(my_pid);

        if (not events::EventAccess::instance()->subscribe(
            connection_params,
            callback))
        {
            LOG(error, "useragent", "subscribe_events()",
                "Could not subscribe to connect/disconnect events");
        }

        events::EntityChangedSubscriptionParams::EntityActions entity_actions;
        const dbtype::Entity::IdVector ignored_entities;
        dbtype::Entity::EntityFieldSet changed_fields;
        const dbtype::Id::SiteIdType ignored_site = 0;
        events::EntityChangedSubscriptionParams::EntityTypes entity_types;
        const dbtype::Entity::FlagSet ignored_flags;
        const dbtype::Entity::IdVector ignored_field_ids;

        entity_actions.push_back(events::EntityChangedEvent::ENTITY_CREATED);
        entity_actions.push_back(events::EntityChangedEvent::ENTITY_DELETED);
        entity_types.push_back(dbtype::ENTITYTYPE_puppet);

        // Subscribe to creates and destroys of all puppets
        const events::EntityChangedSubscriptionParams create_destroy_sub(
            entity_actions,
            ignored_entities,
            false,
            ignored_site,
            entity_types,
            changed_fields,
            ignored_flags,
            ignored_flags,
            ignored_field_ids,
            ignored_field_ids);

        entity_actions.clear();
        entity_actions.push_back(events::EntityChangedEvent::ENTITY_UPDATED);
        changed_fields.insert(dbtype::ENTITYFIELD_owner);
        changed_fields.insert(dbtype::ENTITYFIELD_name);
        entity_types.push_back(dbtype::ENTITYTYPE_player);

        // Subscribe to name changes of players and puppets, and chowning
        // puppets to and from online/offline players
        const events::EntityChangedSubscriptionParams update_sub(
            entity_actions,
            ignored_entities,
            true,
            ignored_site,
            entity_types,
            changed_fields,
            ignored_flags,
            ignored_flags,
            ignored_field_ids,
            ignored_field_ids);

        if (not events::EventAccess::instance()->subscribe(
            create_destroy_sub,
            callback))
        {
            LOG(error, "useragent", "subscribe_events()",
                "Could not subscribe to create/destroy events");
        }

        if (not events::EventAccess::instance()->subscribe(
            update_sub,
            callback))
        {
            LOG(error, "useragent", "subscribe_events()",
                "Could not subscribe to update events");
        }
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::process_connection_event(
        events::ConnectionEvent * const connect_event_ptr)
    {
        if (connect_event_ptr)
        {
            const dbtype::Id &player_id = connect_event_ptr->get_entity_id();
            primitives::NameRegistry * const registry =
                primitives::NameRegistry::instance();
            dbinterface::DatabaseAccess * const db =
                dbinterface::DatabaseAccess::instance();

            switch (connect_event_ptr->get_action())
            {
                case events::ConnectionEvent::ACTION_CONNECTED:
                {
                    LOG(debug, "useragent", "process_connection_event",
                        "Processing connection of player: " +
                        player_id.to_string(true));

                    // New connection; add user and existing puppets
                    //
                    const dbtype::Entity::IdVector found_puppets =
                        dbinterface::DatabaseAccess::instance()->find(
                            player_id.get_site_id(),
                            dbtype::ENTITYTYPE_puppet,
                            player_id.get_entity_id(),
                            std::string());
                    const dbinterface::EntityMetadata player_metadata =
                        db->get_entity_metadata(player_id);
                    const dbinterface::MetadataVector puppets_metadata =
                        db->get_entity_metadata(found_puppets);
                    std::vector<primitives::NameRegistryInfo> name_infos;

                    if (player_metadata.valid())
                    {
                        name_infos.push_back(primitives::NameRegistryInfo(
                            player_metadata.get_name(),
                            player_id,
                            dbtype::ENTITYTYPE_player));

                        for (dbinterface::MetadataVector::const_iterator iter =
                                puppets_metadata.begin();
                            iter != puppets_metadata.end();
                            ++iter)
                        {
                            name_infos.push_back(primitives::NameRegistryInfo(
                                iter->get_name(),
                                iter->get_id(),
                                dbtype::ENTITYTYPE_puppet));
                        }

                        registry->add_entity(name_infos);
                        online_players.insert(player_id);
                    }

                    break;
                }

                case events::ConnectionEvent::ACTION_DISCONNECTED:
                {
                    LOG(debug, "useragent", "process_connection_event",
                        "Processing disconnection of player: " +
                        player_id.to_string(true));

                    // User disconnected; remove user and existing puppets
                    //
                    const dbtype::Entity::IdVector found_puppets =
                        dbinterface::DatabaseAccess::instance()->find(
                            player_id.get_site_id(),
                            dbtype::ENTITYTYPE_puppet,
                            player_id.get_entity_id(),
                            std::string());
                    const dbtype::Entity::IdSet puppet_set(
                        found_puppets.begin(),
                        found_puppets.end());

                    registry->remove_entity(player_id, dbtype::ENTITYTYPE_player);
                    registry->remove_entity(puppet_set, dbtype::ENTITYTYPE_puppet);

                    online_players.erase(connect_event_ptr->get_entity_id());
                    break;
                }

                default:
                {
                    LOG(error, "useragent", "process_connection_event",
                        "Unknown event type: " + text::to_string(
                            connect_event_ptr->get_action()));
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void EntityNameManager::process_entity_event(
        events::EntityChangedEvent *const entity_event_ptr)
    {
        primitives::NameRegistry * const registry =
            primitives::NameRegistry::instance();
        const dbtype::Id &entity_id = entity_event_ptr->get_entity_id();

        switch (entity_event_ptr->get_entity_action())
        {
            case events::EntityChangedEvent::ENTITY_DELETED:
            {
                LOG(debug, "useragent", "process_entity_event",
                    "Processing deletion of puppet: " +
                    entity_id.to_string(true));

                // Players cannot be deleted without first being kicked offline,
                // so this can only be a puppet.
                // Puppet deleted, remove from registry
                registry->remove_entity(entity_id, dbtype::ENTITYTYPE_puppet);
                break;
            }

            case events::EntityChangedEvent::ENTITY_CREATED:
            {
                LOG(debug, "useragent", "process_entity_event",
                    "Processing creation of puppet: " +
                    entity_id.to_string(true));

                // Puppet created, add to registry
                //
                dbinterface::DatabaseAccess * const db =
                    dbinterface::DatabaseAccess::instance();
                dbinterface::EntityRef entity = db->get_entity(entity_id);

                if (entity.valid())
                {
                    primitives::NameRegistryInfo puppet_info(
                        entity->get_entity_name(),
                        entity_id,
                        dbtype::ENTITYTYPE_puppet);

                    registry->add_entity(puppet_info);
                }

                break;
            }

            case events::EntityChangedEvent::ENTITY_UPDATED:
            {
                // Rename
                const dbtype::Entity::EntityFieldSet &fields =
                    entity_event_ptr->get_entity_fields_changed();

                if (fields.count(dbtype::ENTITYFIELD_name))
                {
                    LOG(debug, "useragent", "process_entity_event",
                        "Processing updated name of entity: " +
                        entity_id.to_string(true));

                    dbinterface::DatabaseAccess * const db =
                        dbinterface::DatabaseAccess::instance();
                    dbinterface::EntityRef entity = db->get_entity(entity_id);

                    // Name changed for player or puppet
                    //
                    if (entity.valid())
                    {
                        registry->update_entity_name(
                            entity_id,
                            entity_event_ptr->get_entity_type(),
                            entity->get_entity_name());
                    }
                    else
                    {
                        LOG(debug, "useragent", "process_entity_event",
                            "Updated Entity somehow invalid: " +
                            entity_id.to_string(true));

                        // Somehow invalid, remove.
                        //
                        registry->remove_entity(
                            entity_id,
                            entity_event_ptr->get_entity_type());
                    }
                }

                if (fields.count(dbtype::ENTITYFIELD_owner))
                {
                    LOG(debug, "useragent", "process_entity_event",
                        "Processing updated owner of entity: " +
                        entity_id.to_string(true));

                    // Only puppets can change owners so we don't need to
                    // check type.  Since we don't know who the old owner
                    // is, remove from registry and re-add only if new owner
                    // is online.
                    //
                    registry->remove_entity(
                        entity_id,
                        entity_event_ptr->get_entity_type());

                    dbinterface::DatabaseAccess * const db =
                        dbinterface::DatabaseAccess::instance();
                    dbinterface::EntityRef entity = db->get_entity(entity_id);

                    if (entity.valid())
                    {
                        if (online_players.count(entity->get_entity_owner()))
                        {
                            LOG(debug, "useragent", "process_entity_event",
                                "Owner of entity " + entity_id.to_string(true)
                                + " changing to online owner " +
                                entity->get_entity_owner().to_string(true));

                            primitives::NameRegistryInfo puppet_info(
                                entity->get_entity_name(),
                                entity_id,
                                entity_event_ptr->get_entity_type());

                            registry->add_entity(puppet_info);
                        }
                    }
                    else
                    {
                        LOG(debug, "useragent", "process_entity_event",
                            "Updated Entity somehow invalid: " +
                            entity_id.to_string(true));
                    }
                }

                break;
            }

            default:
            {
                LOG(error, "useragent", "process_entity_event",
                    "Unknown event type: " + text::to_string(
                        entity_event_ptr->get_entity_action()));
            }
        }
    }
}
}
