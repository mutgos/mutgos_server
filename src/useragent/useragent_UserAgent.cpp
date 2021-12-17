/*
 * useragent_UserAgent.cpp
 */

#include <string>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Command.h"

#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_ProcessServices.h"

#include "channels/events_TextChannel.h"
#include "channels/events_ClientDataChannel.h"

#include "events/events_MovementEvent.h"
#include "events/events_EmitEvent.h"

#include "comminterface/comm_CommAccess.h"
#include "comminterface/comm_SessionStats.h"

#include "dbinterface/dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "security/security_Context.h"

#include "primitives/primitives_SystemPrims.h"

#include "softcode/softcode_SoftcodeAccess.h"

#include "useragent/useragent_PuppetManager.h"
#include "useragent/useragent_PuppetCommandMessage.h"
#include "useragent/useragent_PuppetErrorMessage.h"
#include "useragent/useragent_UserAgent.h"

namespace
{
    const std::string CLIENT_DATA_CHANNEL_NAME = "ClientData";
}

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    UserAgent::UserAgent(const dbtype::Id &player)
      : Agent(player),
        puppet_manager_pid(0)
    {
    }

    // ----------------------------------------------------------------------
    UserAgent::~UserAgent()
    {
    }

    // ----------------------------------------------------------------------
    Agent::ProcessStatus UserAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;

        if ((message.message_get_type() ==
             executor::ProcessMessage::MESSAGE_INTERPROCESS) and
            message.message_get_subtype() ==
            PuppetErrorMessage::message_subtype())
        {
            // Got an error message from a puppet.  Print it out.
            //
            PuppetErrorMessage * const error_message_ptr =
                dynamic_cast<PuppetErrorMessage *>(&message);

            if (not error_message_ptr)
            {
                LOG(error, "useragent", "process_execute(msg)",
                    "Expected puppet error message but got something else.");
            }
            else
            {
                const dbinterface::EntityMetadata metadata =
                    dbinterface::DatabaseAccess::instance()->get_entity_metadata(
                        error_message_ptr->get_puppet_id());
                std::string name;

                if (metadata.valid())
                {
                    name = metadata.get_name();
                }
                else
                {
                    name = "UNKNOWN PUPPET";
                }

                send_plain_text("Puppet " + name + " (" +
                    error_message_ptr->get_puppet_id().to_string(false) +
                    ") error: " + error_message_ptr->get_error_message(),
                    true);
            }
        }
        else
        {
            status = Agent::process_execute(pid, services, message);
        }

        return status;
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_finished(const executor::PID pid)
    {
        // Kill puppet manager
        //
        if (puppet_manager_pid)
        {
            executor::ExecutorAccess::instance()->kill_process(
                puppet_manager_pid);
        }

        Agent::process_finished(pid);
    }

    // ----------------------------------------------------------------------
    void UserAgent::setup(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        my_context_ptr = new security::Context(entity_id, dbtype::Id());
        my_context_ptr->set_run_as_requester();
        refresh_context(true);

        // Create the channels and link them up.
        //
        output_channel_ptr = new events::TextChannel("Session Agent Output");
        input_channel_ptr = new events::TextChannel("Session Agent");

        input_channel_ptr->next_resource_add_is_receiver(pid);

        if (not services.add_blocking_resource(input_channel_ptr, input_rid))
        {
            LOG(error, "useragent", "setup",
                "Failed to register input channel as resource!");
        }

        if (not services.add_resource(output_channel_ptr, output_rid))
        {
            LOG(error, "useragent", "setup",
                "Failed to register output channel as resource!");
        }

        if (not comm::CommAccess::instance()->add_channel(
            entity_id,
            output_channel_ptr,
            true))
        {
            LOG(error, "useragent", "setup",
                "Failed to register output channel on comm!");
        }

        if (not comm::CommAccess::instance()->add_channel(
            entity_id,
            input_channel_ptr,
            false))
        {
            LOG(error, "useragent", "setup",
                "Failed to register input channel on comm!");
        }

        if (not output_channel_ptr->unblock_channel())
        {
            LOG(error, "useragent", "setup",
                "Failed to unblock output channel!");
        }

        if (not input_channel_ptr->unblock_channel())
        {
            LOG(error, "useragent", "setup",
                "Failed to unblock input channel!");
        }

        // If this is a web client, it has addition data channels
        //
        const comm::SessionStats stats = comm::CommAccess::instance()->
            get_session_stats(entity_id);

        if ((not stats.get_entity_id().is_default()) and
            stats.is_enhanced())
        {
            // Input from client
            //
            data_input_channel_ptr =
                new events::ClientDataChannel(CLIENT_DATA_CHANNEL_NAME);

            data_input_channel_ptr->next_resource_add_is_receiver(pid);

            if (not services.add_blocking_resource(
                data_input_channel_ptr,
                data_input_rid))
            {
                LOG(error, "useragent", "setup",
                    "Failed to register data input channel as resource!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                entity_id,
                data_input_channel_ptr,
                false))
            {
                LOG(error, "useragent", "setup",
                    "Failed to register data input channel on comm!");
            }

            if (not data_input_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "setup",
                    "Failed to unblock data input channel!");
            }

            // Output to client
            //
            data_output_channel_ptr =
                new events::ClientDataChannel(CLIENT_DATA_CHANNEL_NAME);

            if (not services.add_blocking_resource(
                data_output_channel_ptr,
                data_output_rid))
            {
                LOG(error, "useragent", "setup",
                    "Failed to register data output channel as resource!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                entity_id,
                data_output_channel_ptr,
                true))
            {
                LOG(error, "useragent", "setup",
                    "Failed to register data output channel on comm!");
            }

            if (not data_output_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "setup",
                    "Failed to unblock data output channel!");
            }
        }

        // Launch puppet manager
        //
        PuppetManager *puppet_manager = new PuppetManager(entity_id, pid);

        puppet_manager_pid = executor::ExecutorAccess::instance()->add_process(
            dbtype::Id(),
            entity_id,
            puppet_manager);

        if (not puppet_manager_pid)
        {
            LOG(error, "useragent", "setup",
                "Failed to add puppet manager process for player " +
                entity_id.to_string(true));
            delete puppet_manager;
        }
        else
        {
            if (executor::ExecutorAccess::instance()->start_process(
                puppet_manager_pid))
            {
                LOG(debug, "useragent", "setup",
                    "Launched puppet manager for player " +
                    entity_id.to_string(true));
            }
            else
            {
                LOG(error, "useragent", "setup",
                    "Unable to launch puppet manager for player " +
                    entity_id.to_string(true));
            }
        }
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_action(
        const dbtype::Id &action_id,
        const std::string &channel_subtype,
        std::string &arguments)
    {
        // Get EntityRef
        //
        dbinterface::EntityRef action_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(action_id);

        if (not action_ref.valid())
        {
            send_plain_text("Action no longer valid.", true);
            return;
        }

        // TODO For puppets:
        // TODO Update socket comms
        // TODO update Web UI
        // TODO Rename UserAgent to something better, per Sin

        dbtype::Command * const command_entity =
            dynamic_cast<dbtype::Command *>(action_ref.get());

        if (command_entity)
        {
            const dbtype::Id target_id =
                command_entity->get_first_action_target();

            if (not target_id.is_default())
            {
                // See if target is a puppet we own.
                // Right now a puppet can only be controlled by a single Player
                // due to the streams, so there's no point allowing multiple to
                // control it quite yet.
                //
                dbinterface::EntityRef target_ref =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        target_id);

                if (target_ref.valid() and
                    (target_ref->get_entity_type() == dbtype::ENTITYTYPE_puppet) and
                    (target_ref->get_entity_owner() == entity_id))
                {
                    // It's a puppet.  Send the command to the puppet
                    // manager.
                    //
                    PuppetCommandMessage * const message_ptr =
                        new PuppetCommandMessage(target_id, arguments);

                    if (not executor::ExecutorAccess::instance()->send_message(
                        puppet_manager_pid,
                        message_ptr))
                    {
                        LOG(error, "useragent", "process_action()",
                            "Unable to send message to puppet " +
                                target_id.to_string(true) + ", PID " +
                            text::to_string(puppet_manager_pid));

                        send_plain_text("Unable to send command to puppet.", true);
                    }

                    return;
                }
            }
        }

        Agent::process_action(action_id, channel_subtype, arguments);
    }

    // ----------------------------------------------------------------------
    bool UserAgent::process_quit_command(void)
    {
        comm::CommAccess::instance()->disconnect_session(
            entity_id);
        return false;
    }
}
}
