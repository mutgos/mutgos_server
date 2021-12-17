/*
 * useragent_PuppetManager.cpp
 */

#include "useragent/useragent_PuppetManager.h"

#include "useragent/useragent_PuppetCommandMessage.h"
#include "useragent/useragent_PuppetErrorMessage.h"
#include "useragent/useragent_PuppetStatusMessage.h"
#include "useragent/useragent_PuppetAgent.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "executor/executor_ExecutorAccess.h"

#include "events/events_EventAccess.h"
#include "events/events_EntityChangedEvent.h"
#include "events/events_EntityChangedSubscriptionParams.h"
#include "events/events_EventMatchedMessage.h"


namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    PuppetManager::PuppetManager(
        const dbtype::Id &player,
        const executor::PID player_pid)
        : player_id(player),
          player_agent_pid(player_pid),
          my_pid(0),
          create_delete_sub_id(0),
          update_sub_id(0),
          first_run(true)
    {
    }

    // ----------------------------------------------------------------------
    PuppetManager::~PuppetManager()
    {
    }

    // ----------------------------------------------------------------------
    void PuppetManager::process_added(const executor::PID pid,
        executor::ProcessServices &services)
    {
        my_pid = pid;

        // Make subscriptions to detect when puppets are created or deleted
        // for us, or if puppet is chowned TO us.
        // Puppets chowned FROM us cannot be detected efficiently with
        // subscriptions and will use messaging to trigger a check.
        //
        events::EntityChangedSubscriptionParams::EntityActions entity_actions;
        dbtype::Entity::IdVector owner_entities;
        dbtype::Entity::EntityFieldSet changed_fields;
        const dbtype::Id::SiteIdType ignored_site = 0;
        events::EntityChangedSubscriptionParams::EntityTypes puppet_types;
        const dbtype::Entity::FlagSet ignored_flags;
        const dbtype::Entity::IdVector ignored_field_ids;

        owner_entities.push_back(player_id);
        entity_actions.push_back(events::EntityChangedEvent::ENTITY_CREATED);
        entity_actions.push_back(events::EntityChangedEvent::ENTITY_DELETED);
        puppet_types.push_back(dbtype::ENTITYTYPE_puppet);

        const events::EntityChangedSubscriptionParams create_destroy_sub(
            entity_actions,
            owner_entities,
            true,
            ignored_site,
            puppet_types,
            changed_fields,
            ignored_flags,
            ignored_flags,
            ignored_field_ids,
            ignored_field_ids);

        entity_actions.clear();
        entity_actions.push_back(events::EntityChangedEvent::ENTITY_UPDATED);
        changed_fields.insert(dbtype::ENTITYFIELD_owner);

        const events::EntityChangedSubscriptionParams update_sub(
            entity_actions,
            owner_entities,
            true,
            ignored_site,
            puppet_types,
            changed_fields,
            ignored_flags,
            ignored_flags,
            ignored_field_ids,
            ignored_field_ids);

        const events::SubscriptionCallback callback(my_pid);
        create_delete_sub_id = events::EventAccess::instance()->subscribe(
            create_destroy_sub,
            callback);
        update_sub_id = events::EventAccess::instance()->subscribe(
            update_sub,
            callback);
    }

    executor::Process::ProcessStatus
    PuppetManager::process_execute(const executor::PID pid,
        executor::ProcessServices &services)
    {
        if (first_run)
        {
            do_first_run();
        }

        // Always the first method called when executing; just tell it
        // we only care about messages/events.
        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    executor::Process::ProcessStatus
    PuppetManager::process_execute(const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;

        if (first_run)
        {
            do_first_run();
        }

        switch (message.message_get_type())
        {
            case executor::ProcessMessage::MESSAGE_INTERPROCESS:
            {
                if (message.message_get_subtype() ==
                    PuppetCommandMessage::message_subtype())
                {
                    // Forward command from user agent to puppet agent.
                    //
                    PuppetCommandMessage * const puppet_command =
                        dynamic_cast<PuppetCommandMessage *>(&message);

                    if (not puppet_command)
                    {
                        LOG(error, "useragent", "process_execute(message)",
                            "Expected puppet command message but got "
                            "something else.");
                    }
                    else
                    {
                        if (puppet_command->is_ping())
                        {
                            LOG(debug, "useragent", "process_execute(message)",
                                "Handling puppet ping message for " +
                                puppet_command->get_puppet_id()
                                              .to_string(true));

                            send_puppet_ping(puppet_command->get_puppet_id());
                        }
                        else
                        {
                            LOG(debug, "useragent", "process_execute(message)",
                                "Handling puppet command message for " +
                                puppet_command->get_puppet_id().to_string(true));

                            send_puppet_message(
                                puppet_command->get_puppet_id(),
                                puppet_command->get_input_line());
                        }
                    }
                }
                else if (message.message_get_subtype() ==
                    PuppetStatusMessage::message_subtype())
                {
                    // Puppet agent has died due to puppet no longer
                    // being owned by us.  Remove from map.
                    //
                    PuppetStatusMessage * const puppet_status =
                        dynamic_cast<PuppetStatusMessage *>(&message);

                    if (not puppet_status)
                    {
                        LOG(error, "useragent", "process_execute(message)",
                            "Expected puppet status message but got "
                            "something else.");
                    }
                    else
                    {
                        LOG(debug, "useragent", "process_execute(message)",
                            "Handling puppet status message for " +
                            puppet_status->get_puppet_id().to_string(true));

                        if (not puppet_map.erase(puppet_status->get_puppet_id()))
                        {
                            LOG(warning, "useragent", "process_execute(message)",
                                "Could not delete puppet map entry for " +
                                puppet_status->get_puppet_id().to_string(true));
                        }
                    }
                }
                else
                {
                    LOG(error, "useragent", "process_execute(message)",
                        "Unknown interprocess message subtype: "
                        + text::to_string(message.message_get_subtype()));
                }

                break;
            }

            case executor::ProcessMessage::MESSAGE_EVENT:
            {
                events::EventMatchedMessage const * event_matched_ptr =
                    dynamic_cast<events::EventMatchedMessage *>(&message);

                if (event_matched_ptr)
                {
                    switch (event_matched_ptr->get_event_type())
                    {
                        case events::Event::EVENT_ENTITY_CHANGED:
                        {
                            process_entity_change_event(
                                event_matched_ptr->get_subscription_id(),
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
    std::string PuppetManager::process_get_name(const executor::PID pid)
    {
        return std::string("Puppet Manager for ") + player_id.to_string(true);
    }

    // ----------------------------------------------------------------------
    bool PuppetManager::process_delete_when_finished(const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    executor::Process::ErrorMessageText
    PuppetManager::process_get_error_text(const executor::PID pid)
    {
        executor::Process::ErrorMessageText errors;

        if (not error_message.empty())
        {
            errors.push_back(error_message);
        }

        return errors;
    }

    // ----------------------------------------------------------------------
    void PuppetManager::process_finished(const executor::PID pid)
    {
        events::EventAccess::instance()->unsubscribe(create_delete_sub_id);
        events::EventAccess::instance()->unsubscribe(update_sub_id);
        kill_all_puppets();
    }

    // ----------------------------------------------------------------------
    void PuppetManager::do_first_run(void)
    {
        if (first_run)
        {
            // Find what puppets we currently have and spawn off their agents.
            //
            const dbtype::Entity::IdVector found_puppets =
                dbinterface::DatabaseAccess::instance()->find(
                    player_id.get_site_id(),
                    dbtype::ENTITYTYPE_puppet,
                    player_id.get_entity_id(),
                    std::string());

            for (dbtype::Entity::IdVector::const_iterator puppet_iter =
                found_puppets.begin();
                 puppet_iter != found_puppets.end();
                 ++puppet_iter)
            {
                spawn_puppet(*puppet_iter);
            }

            first_run = false;
        }
    }

    // ----------------------------------------------------------------------
    void PuppetManager::process_entity_change_event(
        const events::SubscriptionId subscription_id,
        events::EntityChangedEvent * const event)
    {
        if (event)
        {
            switch (event->get_entity_action())
            {
                case events::EntityChangedEvent::ENTITY_CREATED:
                {
                    // Puppet created.  Make a new agent.
                    //
                    LOG(debug, "useragent", "process_entity_change_event()",
                        "Puppet added " +
                        event->get_entity_id().to_string(true));

                    spawn_puppet(event->get_entity_id());
                    break;
                }

                case events::EntityChangedEvent::ENTITY_DELETED:
                {
                    // Puppet deleted, kill the agent
                    //
                    LOG(debug, "useragent", "process_entity_change_event()",
                        "Puppet deleted " +
                        event->get_entity_id().to_string(true));

                    kill_puppet(event->get_entity_id());

                    break;
                }

                case events::EntityChangedEvent::ENTITY_UPDATED:
                {
                    if (event->get_entity_fields_changed().count(
                        dbtype::ENTITYFIELD_owner))
                    {
                        // If the owner changed, it can only be TO us.
                        // If it changes to a different owner, we won't
                        // get the notification since we no longer own it and
                        // is filtered out.
                        // Create an agent since we now own it.
                        //
                        LOG(debug, "useragent", "process_entity_change_event()",
                            "Puppet chowned over " +
                            event->get_entity_id().to_string(true));

                        spawn_puppet(event->get_entity_id());
                    }

                    break;
                }

                default:
                {
                    LOG(debug, "useragent", "process_entity_change_event()",
                        "Unknown event type " +
                        text::to_string(event->get_entity_action()));
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void PuppetManager::spawn_puppet(const dbtype::Id &puppet_id)
    {
        if (not puppet_map.count(puppet_id))
        {
            // Puppet doesn't already have an entry.  Spawn off agent
            // and add PID.
            //

            PuppetAgent *agent = new PuppetAgent(player_id, puppet_id, my_pid);
            const executor::PID pid =
                executor::ExecutorAccess::instance()->add_process(
                    dbtype::Id(),
                    player_id,
                    agent);

            if (not pid)
            {
                delete agent;

                LOG(error, "useragent", "spawn_puppet()",
                    "Could not add puppet agent process for " +
                    puppet_id.to_string((true)));
            }
            else
            {
                if (not executor::ExecutorAccess::instance()->start_process(pid))
                {
                    LOG(error, "useragent", "spawn_puppet()",
                        "Could not start puppet agent process for " +
                        puppet_id.to_string((true)));

                    executor::ExecutorAccess::instance()->kill_process(pid);
                }
                else
                {
                    // Successfully started process.  Add to the map.
                    puppet_map[puppet_id] = pid;

                    LOG(debug, "useragent", "spawn_puppet()",
                        "Spawned puppet agent process " +
                        text::to_string(pid) + " for puppet " +
                        puppet_id.to_string((true)));
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void PuppetManager::kill_puppet(const dbtype::Id &puppet_id)
    {
        PuppetMap::const_iterator puppet_iter = puppet_map.find(puppet_id);

        if (puppet_iter != puppet_map.end())
        {
            LOG(debug, "useragent", "kill_puppet()",
                "Killing puppet agent for " +
                puppet_id.to_string(true) + ", PID " +
                text::to_string((puppet_iter->second)));

            executor::ExecutorAccess::instance()->kill_process(
                puppet_iter->second);
            puppet_map.erase(puppet_iter);
        }
    }

    // ----------------------------------------------------------------------
    void PuppetManager::kill_all_puppets(void)
    {
        for (PuppetMap::iterator puppet_iter = puppet_map.begin();
            puppet_iter != puppet_map.end();
            ++puppet_iter)
        {
            LOG(debug, "useragent", "kill_all_puppets()",
                "Killing puppet agent for " +
                    puppet_iter->first.to_string(true) + ", PID " +
                text::to_string((puppet_iter->second)));

            executor::ExecutorAccess::instance()->kill_process(
                puppet_iter->second);
        }
    }

    // ----------------------------------------------------------------------
    void PuppetManager::send_error_message(
        const dbtype::Id &puppet_id,
        const std::string &message)
    {
        PuppetErrorMessage *error_message_ptr =
            new PuppetErrorMessage(puppet_id, message);

        executor::ExecutorAccess::instance()->send_message(
            player_agent_pid,
            error_message_ptr);
    }

    // ----------------------------------------------------------------------
    bool PuppetManager::send_puppet_message(const dbtype::Id &puppet_id,
        const std::string &message)
    {
        bool success = false;
        PuppetMap::const_iterator puppet_iter = puppet_map.find(puppet_id);

        if (puppet_iter != puppet_map.end())
        {
            PuppetCommandMessage * const ping_message_ptr =
                new PuppetCommandMessage(puppet_id, message);

            success = executor::ExecutorAccess::instance()->send_message(
                puppet_iter->second,
                ping_message_ptr);

            if (not success)
            {
                LOG(error, "useragent", "send_puppet_message()",
                    "Unable to send message to puppet " +
                    puppet_id.to_string(true) + ", PID " +
                    text::to_string((puppet_iter->second)));

                send_error_message(
                    puppet_id,
                    "Unable to send command to puppet.");
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool PuppetManager::send_puppet_ping(const dbtype::Id &puppet_id)
    {
        bool success = false;
        PuppetMap::const_iterator puppet_iter = puppet_map.find(puppet_id);

        if (puppet_iter != puppet_map.end())
        {
            PuppetCommandMessage * const ping_message_ptr =
                new PuppetCommandMessage(puppet_id);

            success = executor::ExecutorAccess::instance()->send_message(
                puppet_iter->second,
                ping_message_ptr);

            if (not success)
            {
                LOG(error, "useragent", "send_puppet_ping()",
                    "Unable to send ping to puppet " +
                    puppet_id.to_string(true) + ", PID " +
                    text::to_string((puppet_iter->second)));

                send_error_message(
                    puppet_id,
                    "Unable to send 'ping' to puppet.");
            }
        }

        return success;
    }
}
}