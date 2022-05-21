/*
 * useragent_PuppetAgent.cpp
 */

#include <string>
#include <vector>

#include "useragent/useragent_PuppetAgent.h"

#include "logging/log_Logger.h"

#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_ProcessServices.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "events/events_EventAccess.h"
#include "events/events_EventMatchedMessage.h"
#include "events/events_SubscriptionCallback.h"
#include "channels/events_ClientDataChannel.h"
#include "channels/events_TextChannel.h"
#include "comminterface/comm_CommAccess.h"
#include "comminterface/comm_SessionStats.h"
#include "events/events_EntityChangedEvent.h"
#include "events/events_EntityChangedSubscriptionParams.h"
#include "events/events_EventAccess.h"

#include "security/security_Context.h"
#include "security/security_SecurityAccess.h"

#include "text/text_StringConversion.h"

#include "useragent/useragent_PuppetCommandMessage.h"
#include "useragent/useragent_PuppetStatusMessage.h"


namespace
{
    const std::string CLIENT_DATA_CHANNEL_NAME = "Puppet ClientData";
    const std::string FOREGROUND_PROG_CHAN_NAME = "Puppet foreground_prog";

    const std::vector<mutgos::security::Capability> INHERTED_CAPABILITIES{
         mutgos::security::CAPABILITY_SEND_TEXT_ROOM_UNRESTRICTED,
         mutgos::security::CAPABILITY_SEND_TEXT_ENTITY,
         mutgos::security::CAPABILITY_CHARACTER_FIND_BY_NAME_AFAR,
         mutgos::security::CAPABILITY_CONNECTION_CHECK};
}

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    PuppetAgent::PuppetAgent(
        const dbtype::Id &player,
        const dbtype::Id &puppet,
        const executor::PID manager)
        : Agent(puppet, player),
          manager_pid(manager),
          chown_id(0)
    {
    }

    // ----------------------------------------------------------------------
    PuppetAgent::~PuppetAgent()
    {
    }

    // ----------------------------------------------------------------------
    executor::Process::ProcessStatus
    PuppetAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        first_execute = false;
        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    std::string PuppetAgent::process_get_name(const executor::PID pid)
    {
        return std::string("Puppet Agent for ") + entity_id.to_string(true);
    }

    // ----------------------------------------------------------------------
    void PuppetAgent::setup(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // No setup needed since channels are created on demand
    }

    // ----------------------------------------------------------------------
    executor::Process::ProcessStatus
    PuppetAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        if (activate(services))
        {
            if ((message.message_get_type() ==
                executor::ProcessMessage::MESSAGE_INTERPROCESS) and
                message.message_get_subtype() ==
                PuppetCommandMessage::message_subtype())
            {
                PuppetCommandMessage * const command_message_ptr =
                    dynamic_cast<PuppetCommandMessage *>(&message);

                if (not command_message_ptr)
                {
                    LOG(error, "useragent", "process_execute(msg)",
                        "Expected command message but got something else.");
                }
                else if (command_message_ptr->is_ping())
                {
                    // Ping message. We're now activated if we weren't already.
                    LOG(debug, "useragent", "process_execute(msg)",
                        "Got ping message.");
                }
                else
                {
                    std::string input_line =
                        command_message_ptr->get_input_line();

                    if (not process_user_command(input_line))
                    {
                        // Asked to deactivate.
                        deactivate(services);
                    }
                }

                return PROCESS_STATUS_WAIT_MESSAGE;
            }
            else if ((message.message_get_type() ==
                executor::ProcessMessage::MESSAGE_EVENT))
            {
                events::EventMatchedMessage const * event_matched_ptr =
                    dynamic_cast<events::EventMatchedMessage *>(&message);

                if (event_matched_ptr and (event_matched_ptr->get_event_type() ==
                   events::Event::EVENT_ENTITY_CHANGED))
                {
                    events::EntityChangedEvent *changed_event =
                        dynamic_cast<events::EntityChangedEvent *>(
                            &event_matched_ptr->get_event());

                    if (changed_event and
                        changed_event->get_entity_fields_changed().count(
                            dbtype::ENTITYFIELD_owner))
                    {
                        // Owner field changed.  In this situation, it can
                        // only mean the owner changed AWAY from us.
                        // Let manager know we're shutting down and exit.
                        //
                        send_shutdown_status();
                        deactivate(services);
                        return PROCESS_STATUS_FINISHED;
                    }
                    else
                    {
                        return Agent::process_execute(pid, services, message);
                    }
                }
                else
                {
                    return Agent::process_execute(pid, services, message);
                }
            }
            else
            {
                return Agent::process_execute(pid, services, message);
            }
        }

        // Activation failed due to ownership change.  Finish now.
        return PROCESS_STATUS_FINISHED;
    }


    // ----------------------------------------------------------------------
    executor::Process::ProcessStatus
    PuppetAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        const executor::RID rid,
        executor::ProcessMessage &message)
    {
        // Activate if we're not already, so we have channels.
        if (activate(services))
        {
            return Agent::process_execute(pid, services, rid, message);
        }

        // Activation failed due to ownership change.  Finish now.
        return PROCESS_STATUS_FINISHED;
    }

    // ----------------------------------------------------------------------
    bool PuppetAgent::process_channel_flow(
        executor::ProcessServices &services,
        events::ChannelFlowMessage *const flow_event_ptr)
    {
        if (flow_event_ptr)
        {
            if (flow_event_ptr->get_channel_status() ==
                events::ChannelFlowMessage::CHANNEL_FLOW_CLOSED)
            {
                // Any channel that's closed means we should deactivate
                // as the tab is being closed or something else.
                deactivate(services);
            }
        }

        // Closing channels simply means the agent goes inactive until the next
        // event occurs; don't terminate.
        return false;
    }

    // ----------------------------------------------------------------------
    void PuppetAgent::process_action(
        const dbtype::Id &action_id,
        const std::string &channel_subtype,
        std::string &arguments)
    {
        if (channel_subtype.empty())
        {
            // Adds the puppet name to the channel as a temporary workaround
            // for demo.
            // TODO Need to refactor use of channels with regards to puppets
            dbinterface::EntityRef puppet_entity =
                dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

            if (puppet_entity.valid())
            {
                Agent::process_action(
                    action_id,
                    puppet_entity->get_entity_name(),
                    arguments);
            }
            else
            {
                Agent::process_action(action_id, channel_subtype, arguments);
            }
        }
        else
        {
            Agent::process_action(action_id, channel_subtype, arguments);
        }
    }

    // ----------------------------------------------------------------------
    events::TextChannel *
    PuppetAgent::make_prog_output_channel(const std::string &subtype)
    {
        return new events::TextChannel(
            FOREGROUND_PROG_CHAN_NAME,
            subtype,
            entity_id);
    }

    // ----------------------------------------------------------------------
    bool PuppetAgent::process_quit_command(void)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    bool PuppetAgent::activate(executor::ProcessServices &services)
    {
        if (not output_channel_ptr)
        {
            // Confirm we're still owned by expected player.
            //
            dbinterface::EntityRef puppet_entity =
                dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

            if (not puppet_entity.valid())
            {
                // Entity has been deleted.
                return false;
            }
            else
            {
                // Confirm puppet is still owned by expected player
                //
                if (puppet_entity->get_entity_owner() != session_entity_id)
                {
                    // We no longer own this puppet
                    return false;
                }
            }

            // Create subscription to check for chown change
            //
            events::EntityChangedSubscriptionParams::EntityActions entity_actions;
            dbtype::Entity::IdVector entity;
            dbtype::Entity::EntityFieldSet changed_fields;
            const dbtype::Id::SiteIdType ignored_site = 0;
            const events::EntityChangedSubscriptionParams::EntityTypes ignored_types;
            const dbtype::Entity::FlagSet ignored_flags;
            const dbtype::Entity::IdVector ignored_field_ids;

            entity.push_back(entity_id);
            entity_actions.push_back(events::EntityChangedEvent::ENTITY_UPDATED);
            changed_fields.insert(dbtype::ENTITYFIELD_owner);

            const events::EntityChangedSubscriptionParams chown_sub(
                entity_actions,
                entity,
                false,
                ignored_site,
                ignored_types,
                changed_fields,
                ignored_flags,
                ignored_flags,
                ignored_field_ids,
                ignored_field_ids);
            const events::SubscriptionCallback callback(my_pid);
            chown_id = events::EventAccess::instance()->subscribe(
                chown_sub,
                callback);

            // Make security context
            //
            my_context_ptr = new security::Context(entity_id, dbtype::Id());
            my_context_ptr->set_run_as_requester();
            refresh_context(true);

            // Create the channels and link them up.
            //
            const std::string puppet_name = puppet_entity->get_entity_name();
            output_channel_ptr = new events::TextChannel(
                "Puppet Agent Output",
                puppet_name,
                entity_id);
            input_channel_ptr = new events::TextChannel(
                "Puppet Agent",
                puppet_name,
                entity_id);

            input_channel_ptr->next_resource_add_is_receiver(my_pid);

            if (not services.add_blocking_resource(input_channel_ptr, input_rid))
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to register puppet input channel as resource!");
            }

            if (not services.add_resource(output_channel_ptr, output_rid))
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to register puppet output channel as resource!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                session_entity_id,
                output_channel_ptr,
                true))
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to register puppet output channel on comm!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                session_entity_id,
                input_channel_ptr,
                false))
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to register puppet input channel on comm!");
            }

            if (not output_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to unblock puppet output channel!");
            }

            if (not input_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "make_channels",
                    "Failed to unblock puppet input channel!");
            }

            // If this is a web client, it has addition data channels
            //
            const comm::SessionStats stats = comm::CommAccess::instance()->
                get_session_stats(session_entity_id);

            if ((not stats.get_entity_id().is_default()) and
                stats.is_enhanced())
            {
                // Input from client
                //
                data_input_channel_ptr =
                    new events::ClientDataChannel(
                        CLIENT_DATA_CHANNEL_NAME,
                        puppet_name,
                        entity_id);

                data_input_channel_ptr->next_resource_add_is_receiver(my_pid);

                if (not services.add_blocking_resource(
                    data_input_channel_ptr,
                    data_input_rid))
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to register puppet data input channel as resource!");
                }

                if (not comm::CommAccess::instance()->add_channel(
                    session_entity_id,
                    data_input_channel_ptr,
                    false))
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to register puppet data input channel on comm!");
                }

                if (not data_input_channel_ptr->unblock_channel())
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to unblock puppet data input channel!");
                }

                // Output to client
                //
                data_output_channel_ptr =
                    new events::ClientDataChannel(
                        CLIENT_DATA_CHANNEL_NAME,
                        puppet_name,
                        entity_id);

                if (not services.add_blocking_resource(
                    data_output_channel_ptr,
                    data_output_rid))
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to register puppet data output channel as resource!");
                }

                if (not comm::CommAccess::instance()->add_channel(
                    session_entity_id,
                    data_output_channel_ptr,
                    true))
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to register puppet data output channel on comm!");
                }

                if (not data_output_channel_ptr->unblock_channel())
                {
                    LOG(error, "useragent", "make_channels",
                        "Failed to unblock puppet data output channel!");
                }
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void PuppetAgent::deactivate(executor::ProcessServices &services)
    {
        if (output_channel_ptr)
        {
            // Clean up subscriptions
            //
            events::EventAccess::instance()->unsubscribe(chown_id);
            chown_id = 0;

            // Channels exist; close them.
            //
            services.remove_resource(output_rid);
            services.remove_resource(input_rid);

            output_rid = 0;
            input_rid = 0;
            output_channel_ptr = 0;
            input_channel_ptr = 0;

            if (data_output_channel_ptr)
            {
                services.remove_resource(data_output_rid);
                services.remove_resource(data_input_rid);
                data_output_rid = 0;
                data_input_rid = 0;
                data_output_channel_ptr = 0;
                data_input_channel_ptr = 0;
            }

            // Also clean up context
            //
            delete my_context_ptr;
            my_context_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void PuppetAgent::send_shutdown_status(void)
    {
        PuppetStatusMessage * const status_message_ptr =
            new PuppetStatusMessage(entity_id);

        const bool success = executor::ExecutorAccess::instance()->send_message(
            manager_pid,
            status_message_ptr);

        if (not success)
        {
            LOG(error, "useragent", "send_shutdown_status()",
                "Unable to send status message to manager PID " +
                text::to_string(manager_pid));
        }
    }

    // ----------------------------------------------------------------------
    void PuppetAgent::modify_refreshed_context(void)
    {
        // Get the player's capabilities and add certain ones to the puppet's.
        //
        security::Context player_context(session_entity_id, dbtype::Id());
        player_context.set_run_as_requester();
        security::SecurityAccess::instance()->populate_context_capabilities(
            player_context);

        for (std::vector<mutgos::security::Capability>::const_iterator iter =
                INHERTED_CAPABILITIES.begin();
            iter != INHERTED_CAPABILITIES.end();
            ++iter)
        {
            if (player_context.has_capability(*iter))
            {
                my_context_ptr->add_capability(*iter);
            }
        }
    }

    // ----------------------------------------------------------------------
    void
    PuppetAgent::modify_program_context(security::Context &program_context)
    {
        // Artifically inherits capabilities from our context onto the
        // program's, since we have artificially inherited capabilities
        // from the puppet's player.
        // Assumed modify_refreshed_context() has already been called,
        // which should be true by this point.
        //
        for (std::vector<mutgos::security::Capability>::const_iterator iter =
            INHERTED_CAPABILITIES.begin();
             iter != INHERTED_CAPABILITIES.end();
             ++iter)
        {
            if (my_context_ptr->has_capability(*iter))
            {
                program_context.add_capability(*iter);
            }
        }
    }
}
}
