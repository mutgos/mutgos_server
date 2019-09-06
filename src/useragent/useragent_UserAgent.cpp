/*
 * useragent_UserAgent.cpp
 */

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Exit.h"
#include "dbtypes/dbtype_Command.h"
#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessServices.h"
#include "executor/executor_ProcessMessage.h"

#include "channels/events_TextChannel.h"
#include "channels/events_ChannelFlowMessage.h"
#include "channels/events_ChannelTextMessage.h"
#include "channels/events_ClientDataChannel.h"
#include "channels/events_ChannelClientDataMessage.h"
#include "clientmessages/message_ClientExecuteEntity.h"
#include "clientmessages/message_ClientMatchNameRequest.h"
#include "clientmessages/message_ClientMatchNameResult.h"
#include "clientmessages/message_LocationInfoChange.h"

#include "events/events_EventAccess.h"
#include "events/events_MovementSubscriptionParams.h"
#include "events/events_MovementEvent.h"
#include "events/events_EmitSubscriptionParams.h"
#include "events/events_EmitEvent.h"
#include "events/events_EventMatchedMessage.h"

#include "comminterface/comm_CommAccess.h"
#include "comminterface/comm_SessionStats.h"

#include "dbinterface/dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalFormattedText.h"

#include "security/security_Context.h"
#include "security/security_SecurityAccess.h"

#include "primitives/primitives_DatabasePrims.h"
#include "primitives/primitives_SystemPrims.h"
#include "primitives/primitives_MovementPrims.h"
#include "primitives/primitives_PrimitivesAccess.h"

#include "softcode/softcode_SoftcodeAccess.h"

#include "useragent_TextChannelDocumentWriter.h"
#include "useragent_UserAgent.h"

namespace
{
    const std::string CLIENT_DATA_CHANNEL_NAME = "ClientData";

    const std::string QUIT_COMMAND = "QUIT";
    const std::string LIST_PROG_COMMAND = "@listprog";
    const std::string EDIT_PROG_COMMAND = "@editprog";

    const std::string SAVE_EDIT_STRING = ".save";
    const std::string ABORT_EDIT_STRING = ".abort";
    const std::string PRIVATE_MESSAGE_STRING = "* ";

    const std::string FOREGROUND_PROG_CHAN_NAME = "foreground_prog";

    const std::string REDIRECT_SYM = ">>";

    const MG_LongUnsignedInt MAX_SECONDS_CONTEXT_REFRESH = 180; // 3 minutes
}

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    UserAgent::UserAgent(const dbtype::Id &player)
      : first_execute(true),
        my_context(player, dbtype::Id()),
        my_pid(0),
        program_source_ptr(0),
        location_subscription_id(0),
        emit_subscription_id(0),
        private_subscription_id(0),
        output_channel_ptr(0),
        output_rid(0),
        input_channel_ptr(0),
        input_rid(0),
        data_output_channel_ptr(0),
        data_output_rid(0),
        data_input_channel_ptr(0),
        data_input_rid(0)
    {
    }

    // ----------------------------------------------------------------------
    UserAgent::~UserAgent()
    {
        delete program_source_ptr;
        delete output_channel_ptr;
        delete input_channel_ptr;
        delete data_output_channel_ptr;
        delete data_input_channel_ptr;
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_added(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        my_pid = pid;
        my_context.set_run_as_requester();
        refresh_context(true);

        // Create the channels and link them up.
        //
        output_channel_ptr = new events::TextChannel("Session Agent Output");
        input_channel_ptr = new events::TextChannel("Session Agent");

        input_channel_ptr->next_resource_add_is_receiver(pid);

        if (not services.add_blocking_resource(input_channel_ptr, input_rid))
        {
            LOG(error, "useragent", "process_added",
                "Failed to register input channel as resource!");
        }

        if (not services.add_resource(output_channel_ptr, output_rid))
        {
            LOG(error, "useragent", "process_added",
                "Failed to register output channel as resource!");
        }

        if (not comm::CommAccess::instance()->add_channel(
            my_context.get_requester(),
            output_channel_ptr,
            true))
        {
            LOG(error, "useragent", "process_added",
                "Failed to register output channel on comm!");
        }

        if (not comm::CommAccess::instance()->add_channel(
            my_context.get_requester(),
            input_channel_ptr,
            false))
        {
            LOG(error, "useragent", "process_added",
                "Failed to register input channel on comm!");
        }

        if (not output_channel_ptr->unblock_channel())
        {
            LOG(error, "useragent", "process_added",
                "Failed to unblock output channel!");
        }

        if (not input_channel_ptr->unblock_channel())
        {
            LOG(error, "useragent", "process_added",
                "Failed to unblock input channel!");
        }

        // If this is a web client, it has addition data channels
        //
        const comm::SessionStats stats = comm::CommAccess::instance()->
            get_session_stats(my_context.get_requester());

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
                LOG(error, "useragent", "process_added",
                    "Failed to register data input channel as resource!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                my_context.get_requester(),
                data_input_channel_ptr,
                false))
            {
                LOG(error, "useragent", "process_added",
                    "Failed to register data input channel on comm!");
            }

            if (not data_input_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "process_added",
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
                LOG(error, "useragent", "process_added",
                    "Failed to register data output channel as resource!");
            }

            if (not comm::CommAccess::instance()->add_channel(
                my_context.get_requester(),
                data_output_channel_ptr,
                true))
            {
                LOG(error, "useragent", "process_added",
                    "Failed to register data output channel on comm!");
            }

            if (not data_output_channel_ptr->unblock_channel())
            {
                LOG(error, "useragent", "process_added",
                    "Failed to unblock data output channel!");
            }
        }

        // Do the subscriptions.
        subscribe_events();
    }

    // ----------------------------------------------------------------------
    UserAgent::ProcessStatus UserAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        if (first_execute)
        {
            // We should only get called initially with this method signature,
            // which is used to welcome the player and do the initial look.
            // Otherwise, we are message/event driven only.
            // Note that it is possible we are never called with this method
            // directly by the executor if there are always messages waiting.
            //
            send_plain_text("Welcome!", false);

            if (data_output_channel_ptr)
            {
                // Enhanced client.  Send an initial location update.
                //
                dbtype::Id location_id;

                primitives::PrimitivesAccess::instance()->database_prims().
                        get_entity_location(
                            my_context,
                            my_context.get_requester(),
                            location_id,
                            false);

                send_location_update(location_id);
            }

            force_look();

            first_execute = false;
        }

        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    UserAgent::ProcessStatus UserAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;

        if (first_execute)
        {
            process_execute(pid, services);
        }

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
                        case events::Event::EVENT_MOVEMENT:
                        {
                            process_location_change(
                                dynamic_cast<events::MovementEvent *>(
                                    & event_matched_ptr->get_event()));
                            break;
                        }

                        case events::Event::EVENT_EMIT:
                        {
                            process_emit(
                                event_matched_ptr->get_subscription_id(),
                                dynamic_cast<events::EmitEvent *>(
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
                send_plain_text("Subscription deleted due to a referenced "
                                "Entity being deleted.  Disconnecting...", true);

                comm::CommAccess::instance()->disconnect_session(
                    my_context.get_requester());
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
    UserAgent::ProcessStatus UserAgent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        const executor::RID rid,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;
        const executor::ProcessMessage::ProcessMessageType message_type =
            message.message_get_type();

        if (first_execute)
        {
            process_execute(pid, services);
        }

        if (rid == output_rid)
        {
            if (message_type == executor::ProcessMessage::MESSAGE_CHANNEL_FLOW)
            {
                // Only a closed channel is important right now; all others
                // are considered OK.
                //
                events::ChannelFlowMessage * const flow_message_ptr =
                    dynamic_cast<events::ChannelFlowMessage *>(&message);

                if (not flow_message_ptr)
                {
                    LOG(error, "useragent", "process_execute(rid)",
                        "Expected flow message from output channel but got "
                        "something else.");
                }
                else
                {
                    if (process_channel_flow(flow_message_ptr))
                    {
                        LOG(debug, "useragent", "process_execute(rid)",
                            "Output channel closed; request process to "
                            "terminate for "
                            + my_context.get_requester().to_string(true));
                        status = PROCESS_STATUS_FINISHED;
                    }
                }
            }
            else
            {
                LOG(error, "useragent", "process_execute(rid)",
                    "Unknown message type from output channel.");
            }
        }
        else if (rid == input_rid)
        {
            if (message_type == executor::ProcessMessage::MESSAGE_TEXT_CHANNEL)
            {
                // User input has arrived.
                //
                events::ChannelTextMessage * const text_message_ptr =
                    dynamic_cast<events::ChannelTextMessage *>(&message);

                if (not text_message_ptr)
                {
                    LOG(error, "useragent", "process_execute(rid)",
                        "Expected text message from input channel but got "
                        "something else.");
                }
                else
                {
                    // Convert to plain string and send to processor.
                    //
                    std::string message_plain_text;
                    text::ExternalTextLine line = text_message_ptr->get_item();
                    text_message_ptr->get_item().clear();

                    if ((not line.empty()) and (line.back()) and
                        (line.back()->get_text_type() !=
                        text::ExternalText::TEXT_TYPE_PLAIN_TEXT))
                    {
                        // Insert an empty plain text at the end to reset
                        // the color, if color was used.
                        //
                        line.push_back(new text::ExternalPlainText());
                    }

                    const primitives::Result result =
                        primitives::PrimitivesAccess::instance()->system_prims().
                            from_external_text(
                                my_context,
                                line,
                                message_plain_text,
                                false);

                    text::ExternalText::clear_text_line(line);

                    if (result.is_security_violation())
                    {
                        send_plain_text(
                            "Security violation when converting input.",
                            true);
                    }
                    else if (not result.is_success())
                    {
                        send_plain_text("Failed to convert input.", true);
                    }
                    else
                    {
                        if (not process_user_command(message_plain_text))
                        {
                            // User wants to quit.  Shut down.
                            status = PROCESS_STATUS_FINISHED;
                        }
                    }
                }
            }
            else if (message_type ==
                executor::ProcessMessage::MESSAGE_CHANNEL_FLOW)
            {
                // Only a closed channel is important right now; all others
                // are considered OK.
                //
                events::ChannelFlowMessage * const flow_message_ptr =
                    dynamic_cast<events::ChannelFlowMessage *>(&message);

                if (not flow_message_ptr)
                {
                    LOG(error, "useragent", "process_execute(rid)",
                        "Expected flow message from input channel but got "
                        "something else.");
                }
                else
                {
                    if (process_channel_flow(flow_message_ptr))
                    {
                        LOG(debug, "useragent", "process_execute(rid)",
                            "Input channel closed; request process to "
                            "terminate for "
                            + my_context.get_requester().to_string(true));
                        status = PROCESS_STATUS_FINISHED;
                    }
                }
            }
            else
            {
                LOG(error, "useragent", "process_execute(rid)",
                    "Unknown message type from input channel.");
            }
        }
        else if (rid == data_input_rid)
        {
            if (message_type == executor::ProcessMessage::MESSAGE_CLIENT_DATA_CHANNEL)
            {
                // Input from the enhanced client has arrived
                //
                events::ChannelClientDataMessage * const data_message_ptr =
                    dynamic_cast<events::ChannelClientDataMessage *>(&message);

                if (not data_message_ptr)
                {
                    LOG(error, "useragent", "process_execute(rid)",
                        "Expected client data message from data input channel "
                        "but got something else.");
                }
                else
                {
                    const message::ClientMessage &message =
                        data_message_ptr->get_item();
                    const message::ClientMessageType messageType =
                        message.get_message_type();

                    if (messageType == message::CLIENTMESSAGE_EXECUTE_ENTITY)
                    {
                        const message::ClientExecuteEntity * const
                            execute_entity_ptr = dynamic_cast<
                                const message::ClientExecuteEntity *>(& message);

                        if (execute_entity_ptr)
                        {
                            process_execute_entity(*execute_entity_ptr);
                        }
                        else
                        {
                            LOG(error, "useragent", "process_execute(rid)",
                                "Expected client execute entity message "
                                "but got something else.");
                        }
                    }
                    else if (messageType == message::CLIENTMESSAGE_MATCH_NAME_REQUEST)
                    {
                        const message::ClientMatchNameRequest * const
                            match_ptr = dynamic_cast<
                                const message::ClientMatchNameRequest *>(& message);

                        if (match_ptr)
                        {
                            process_match_name(*match_ptr);
                        }
                        else
                        {
                            LOG(error, "useragent", "process_execute(rid)",
                                "Expected client match name message "
                                "but got something else.");
                        }
                    }
                    else
                    {
                        LOG(error, "useragent", "process_execute(rid)",
                            "Unknown message type from data input channel.");
                    }
                }
            }
        }
        else
        {
            LOG(error, "useragent", "process_execute(rid)", "Unknown RID.");
        }

        return status;
    }

    // ----------------------------------------------------------------------
    std::string UserAgent::process_get_name(const executor::PID pid)
    {
        return std::string("UserAgent for ")
            + my_context.get_requester().to_string(true);
    }

    // ----------------------------------------------------------------------
    bool UserAgent::process_delete_when_finished(const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    UserAgent::ErrorMessageText UserAgent::process_get_error_text(
        const executor::PID pid)
    {
        return ErrorMessageText();
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_killed(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // Nothing to do when killed.
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_finished(const executor::PID pid)
    {
        // Nothing to clean up.  All resources are automatically cleaned up.

        // Just set these to null so the destructor won't delete them.
        output_channel_ptr = 0;
        input_channel_ptr = 0;
        data_output_channel_ptr = 0;
        data_input_channel_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool UserAgent::process_channel_flow(
        events::ChannelFlowMessage * const flow_event_ptr)
    {
        bool end_program = false;

        // If any of our Channels are closed, it's time to terminate.
        //
        if (flow_event_ptr)
        {
            if (flow_event_ptr->get_channel_status() ==
                events::ChannelFlowMessage::CHANNEL_FLOW_CLOSED)
            {
                // Any channel that's closed is a program-ender.
                end_program = true;
            }
        }

        return end_program;
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_location_change(
        events::MovementEvent * const movement_event_ptr)
    {
        if (movement_event_ptr and
            (movement_event_ptr->get_from() != movement_event_ptr->get_to()))
        {
            events::EmitSubscriptionParams params(
                dbtype::Id(),
                movement_event_ptr->get_to(),
                my_context.get_requester());
            events::SubscriptionCallback callback(my_pid);

            // Unsubscribe from current location, and resubscribe to new
            //
            events::EventAccess::instance()->unsubscribe(emit_subscription_id);

            emit_subscription_id = events::EventAccess::instance()->subscribe(
                params,
                callback);

            if (data_output_channel_ptr)
            {
                // Enhanced client.  Send a location update.
                send_location_update(movement_event_ptr->get_to());
            }

            force_look();
        }
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_emit(
        const events::SubscriptionId subscription_id,
        events::EmitEvent * const emit_event_ptr)
    {
        if (emit_event_ptr)
        {
            if (subscription_id == private_subscription_id)
            {
                // Private message
                //
                text::ExternalTextLine line =
                    text::ExternalText::clone_text_line(
                        emit_event_ptr->get_text());

                line.insert(line.begin(), new text::ExternalFormattedText(
                    PRIVATE_MESSAGE_STRING,
                    true,
                    false,
                    false,
                    false,
                    text::ExternalFormattedText::COLOR_CYAN));

                // Output channel is never allowed to be blocked, only closed.
                output_channel_ptr->send_item(line);
                text::ExternalText::clear_text_line(line);
            }
            else if (subscription_id == emit_subscription_id)
            {
                // Message from room.  It's already checked for if we're
                // excluded.
                //
                text::ExternalTextLine line =
                    text::ExternalText::clone_text_line(
                        emit_event_ptr->get_text());

                // Output channel is never allowed to be blocked, only
                // closed.
                output_channel_ptr->send_item(line);
                text::ExternalText::clear_text_line(line);
            }
        }
    }

    // ----------------------------------------------------------------------
    // There is no reason to unsubscribe from these events unless we need to
    // modify them.  The event subsystem will automatically unsubscribe from
    // them when the process ends.
    void UserAgent::subscribe_events(void)
    {
        // Get my Entity so I can use info about it in the subscriptions.
        //
        dbinterface::EntityRef requester_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                my_context.get_requester());

        if (not requester_ref.valid())
        {
            LOG(error, "useragent", "subscribe_events",
                "Could not get my Entity!");
            return;
        }

        dbtype::ContainerPropertyEntity * const requester_entity_ptr =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(requester_ref.get());

        if (not requester_entity_ptr)
        {
            LOG(error, "useragent", "subscribe_events",
                "My Entity is not a container!");
            return;
        }

        events::SubscriptionCallback callback(my_pid);

        // Subscribe to location changes
        //
        events::MovementSubscriptionParams move_params;
        move_params.add_who(my_context.get_requester());
        location_subscription_id = events::EventAccess::instance()->subscribe(
            move_params,
            callback);

        // Subscribe to room emits for our current location.
        //
        events::EmitSubscriptionParams emit_params(
            dbtype::Id(),
            requester_entity_ptr->get_contained_by(),
            my_context.get_requester());
        emit_subscription_id = events::EventAccess::instance()->subscribe(
            emit_params,
            callback);

        // Subscribe to private messages
        //
        events::EmitSubscriptionParams private_message_params(
            dbtype::Id(),
            my_context.get_requester(),
            my_context.get_requester());
        private_subscription_id = events::EventAccess::instance()->subscribe(
            private_message_params,
            callback);
    }

    // ----------------------------------------------------------------------
    bool UserAgent::process_user_command(std::string &command_str)
    {
        bool keep_processing = true;

        refresh_context();

        if (command_str.empty())
        {
            // Do nothing for empty strings.
        }
        else if (command_str == QUIT_COMMAND)
        {
            comm::CommAccess::instance()->disconnect_session(
                my_context.get_requester());
            keep_processing = false;
        }
        else if (program_source_ptr)
        {
            // Currently editing a program.
            process_edit_program(command_str);
        }
        else
        {
            // Separate the first part of the command, used for the built-ins
            // and also in case the entire line does not match an action.
            //
            text::trim(command_str);

            // Insert a space after a :, to allow for easy lookup of pose action.
            if ((command_str.size() >= 2) and command_str[0] == ':')
            {
                command_str.insert(1, " ");
            }

            const size_t separator_pos = command_str.find(' ');
            std::string first_command;
            std::string arguments;

            if (separator_pos == std::string::npos)
            {
                first_command = text::to_lower_copy(command_str);
            }
            else
            {
                first_command =
                    text::to_lower_copy(command_str.substr(0, separator_pos));
                arguments = text::trim_copy(command_str.substr(separator_pos));
            }

            text::trim(first_command);

            if (first_command == LIST_PROG_COMMAND)
            {
                // List program command.
                //
                if (separator_pos == std::string::npos)
                {
                    process_list_program("");
                }
                else
                {
                    process_list_program(command_str.substr(separator_pos));
                }
            }
            else if (first_command == EDIT_PROG_COMMAND)
            {
                // Edit program command.
                //
                if (separator_pos == std::string::npos)
                {
                    process_enter_edit_program("");
                }
                else
                {
                    process_enter_edit_program(command_str.substr(separator_pos));
                }
            }
            else
            {
                // Search for an action
                //
                primitives::DatabasePrims &db_prims =
                    primitives::PrimitivesAccess::instance()->database_prims();
                primitives::Result result;
                dbtype::Id found_id;
                bool is_ambiguous = false;

                // First search for the entire string.
                //
                found_id = db_prims.convert_string_to_id(
                    my_context,
                    command_str);

                if (found_id.is_default())
                {
                    result = db_prims.match_name_to_id(
                        my_context,
                        command_str,
                        true,
                        dbtype::ENTITYTYPE_action,
                        found_id,
                        is_ambiguous,
                        false);
                }

                // If found, use
                //
                if (result.is_success())
                {
                    arguments.clear();
                    process_action(found_id, "", arguments);
                }
                else if (result.is_security_violation())
                {
                    send_plain_text("Permission denied.", true);
                }
                else if (command_str == first_command)
                {
                    // Not found.  Print error.
                    send_plain_text("Command not found.", true);
                }
                else
                {
                    // Not found, use the first portion of the command and
                    // try again.
                    //
                    result = primitives::Result();

                    found_id = db_prims.convert_string_to_id(
                        my_context,
                        first_command);

                    if (found_id.is_default())
                    {
                        result = db_prims.match_name_to_id(
                            my_context,
                            first_command,
                            true,
                            dbtype::ENTITYTYPE_action,
                            found_id,
                            is_ambiguous,
                            false);
                    }

                    if (result.is_success())
                    {
                        process_action(found_id, "", arguments);
                    }
                    else if (result.is_security_violation())
                    {
                        send_plain_text("Permission denied.", true);
                    }
                    else
                    {
                        // Not found at all.  Print error.
                        send_plain_text("Command not found.", true);
                    }
                }
            }
        }

        return keep_processing;
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

        dbtype::Command * const command_entity =
            dynamic_cast<dbtype::Command *>(action_ref.get());

        if (dynamic_cast<dbtype::Exit *>(action_ref.get()))
        {
            // This is an exit, do the move.
            //
            primitives::Result move_result =
                primitives::PrimitivesAccess::instance()->
                    movement_prims().move_requester_with_exit(
                        my_context,
                        action_id,
                        true,
                        true,
                        false);

            if (move_result.is_security_violation())
            {
                send_plain_text("Permission denied.", true);
            }
            else if (not move_result.is_success())
            {
                send_plain_text("Failed to do move.", true);
            }
        }
        else if (command_entity)
        {
            // This is a command (Program).  Execute it if we have permission.
            //
            primitives::Result result;

            result = primitives::PrimitivesAccess::instance()->database_prims().
                can_use_action(
                    my_context,
                    action_id,
                    false);

            if (result.is_security_violation())
            {
                send_plain_text("Permission denied.", true);
                return;
            }
            else if (not result.is_success())
            {
                send_plain_text("Unable to use command.", true);
                return;
            }

            security::Context * const prog_context_ptr =
                new security::Context(
                    my_context.get_requester(),
                    command_entity->get_first_action_target());
            events::TextChannel * const prog_output_ptr =
                new events::TextChannel(
                    FOREGROUND_PROG_CHAN_NAME,
                    channel_subtype);

            dbtype::Id redirect_entity;
            dbinterface::EntityRef redirect_entity_ref;
            std::string redirect_property;
            bool has_property_value = false; // Not used
            std::string redirect_property_value; // Not used
            bool ambiguous = false;
            const size_t redirect_sep = arguments.find(REDIRECT_SYM);

            prog_context_ptr->set_run_as_requester();

            // Determine if we are redirecting to a Document.
            //
            if ((redirect_sep != std::string::npos) and
                (arguments.size() > (redirect_sep + 1)))
            {
                const std::string redirect_info_str =
                    arguments.substr(redirect_sep + REDIRECT_SYM.size());

                // Remove redirection from arguments.
                //
                arguments.erase(redirect_sep);
                text::trim(arguments);

                // Try and process the redirection.
                //
                primitives::DatabasePrims &db_prims =
                    primitives::PrimitivesAccess::instance()->database_prims();
                const primitives::Result convert_result =
                    db_prims.convert_string_to_prop_params(
                            my_context,
                            redirect_info_str,
                            redirect_entity,
                            redirect_property,
                            has_property_value,
                            redirect_property_value,
                            ambiguous,
                            false);

                redirect_entity_ref = dbinterface::DatabaseAccess::instance()->
                    get_entity(redirect_entity);

                if (convert_result.is_security_violation())
                {
                    send_plain_text(
                        "Access denied while finding redirect target.", true);
                    delete prog_context_ptr;
                    delete prog_output_ptr;
                    return;
                }
                else if ((not convert_result.is_success()) or
                    (not redirect_entity_ref.valid()))
                {
                    send_plain_text(
                        "Unable to parse redirect parameters.", true);

                    if (ambiguous)
                    {
                        send_plain_text(
                            "Redirect target name is ambiguous.", true);
                    }

                    delete prog_context_ptr;
                    delete prog_output_ptr;
                    return;
                }
                else
                {
                    // Have a good redirect parse, now do a security and
                    // path check by setting the property.
                    //
                    const primitives::Result set_doc_result =
                        db_prims.set_application_property(
                            my_context,
                            redirect_entity,
                            redirect_property,
                            dbtype::DocumentProperty(),
                            false);

                    if (set_doc_result.is_security_violation())
                    {
                        send_plain_text(
                            "Access denied on redirect target or property.",
                            true);

                        delete prog_context_ptr;
                        delete prog_output_ptr;
                        return;
                    }
                    else if (not set_doc_result.is_success())
                    {
                        send_plain_text(
                            "Unable to set Document on redirect target.",
                            true);

                        delete prog_context_ptr;
                        delete prog_output_ptr;
                        return;
                    }

                    // If we're here, security has passed; we will create
                    // the writer later.
                }
            }

            prog_output_ptr->channel_register_pointer_holder(this);

            // TODO get command name post-demo.
            // TODO Could be possible race conditions or memory leaks here with the Channel

            // Because only the agent can do a disconnect or do a kill process,
            // it should be guaranteed that the process won't be suddenly
            // removed.
            const executor::PID pid =
                softcode::SoftcodeAccess::instance()->make_process(
                    prog_context_ptr,
                    "",
                    arguments,
                    prog_output_ptr,
                    0);

            if (not pid)
            {
                delete prog_output_ptr;
                send_plain_text("Unable to create process.", true);
            }
            else
            {
                // Created the process; now link up the Channel.
                //
                if (redirect_entity.is_default())
                {
                    // Normal linkage to connection.
                    //
                    if (not comm::CommAccess::instance()->add_channel(
                        my_context.get_requester(),
                        prog_output_ptr,
                        true))
                    {
                        executor::ExecutorAccess::instance()->kill_process(pid);
                        prog_output_ptr->close_channel();
                        prog_output_ptr->channel_unregister_pointer_holder(this);

                        send_plain_text(
                            "Unable to start process. "
                              "[can't add Channel to connection]",
                            true);
                        return;
                    }
                }
                else
                {
                    // Use redirect writer linkage.
                    // Once constructed, the writer is ready and will
                    // self manage.
                    //
                    new TextChannelDocumentWriter(
                        redirect_entity_ref,
                        redirect_property,
                        prog_output_ptr);
                }

                prog_output_ptr->unblock_channel();

                // Created process, now start it.
                //
                if (not executor::ExecutorAccess::instance()->start_process(pid))
                {
                    send_plain_text("Unable to start process. [internal error]",
                        true);

                    executor::ExecutorAccess::instance()->kill_process(pid);
                    prog_output_ptr->close_channel();
                }

                prog_output_ptr->channel_unregister_pointer_holder(this);
            }
        }
        else
        {
            send_plain_text("Unknown action type.", true);
        }
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_execute_entity(
        const mutgos::message::ClientExecuteEntity &message)
    {
        if (not message.get_entity_id().is_default())
        {
            std::string argument;

            if (not message.get_program_arguments().empty())
            {
                argument = message.get_program_arguments().front();
            }

            process_action(
                message.get_entity_id(),
                message.get_channel_subtype(),
                argument);
        }
    }

    // ----------------------------------------------------------------------
    void UserAgent::process_match_name(
        const message::ClientMatchNameRequest &message)
    {
        primitives::DatabasePrims &db_prims =
            primitives::PrimitivesAccess::instance()->database_prims();
        dbtype::Id found_id;
        bool is_ambiguous = false;
        primitives::Result result;
        message::ClientMatchNameResult * const result_message_ptr =
            new message::ClientMatchNameResult();

        result_message_ptr->set_message_response_flag(true);
        result_message_ptr->set_message_request_id(
            message.get_message_request_id());

        // Try and convert it to an ID first, and if not then do a normal
        // search.
        //
        found_id = db_prims.convert_string_to_id(
            my_context,
            message.get_search_string());

        if (found_id.is_default())
        {
            result = db_prims.match_name_to_id(
                my_context,
                message.get_search_string(),
                message.get_exact_match_flag(),
                message.get_entity_type(),
                found_id,
                is_ambiguous,
                false);
        }

        // Put result into message.
        //
        if (result.is_success() and (not found_id.is_default()))
        {
            result_message_ptr->add_matching_id(found_id);
        }
        else if (result.is_security_violation())
        {
            result_message_ptr->set_security_violation_flag(true);
        }
        else
        {
            result_message_ptr->set_ambiguous_flag(is_ambiguous);
        }

        // Send message back to client.
        //
        if (data_output_channel_ptr)
        {
            if (not data_output_channel_ptr->send_item(result_message_ptr))
            {
                delete result_message_ptr;
            }
        }
    }

    // ----------------------------------------------------------------------
    // This operation does not exist in the primitives, so temporarily doing
    // it here for now.
    void UserAgent::process_list_program(const std::string &program_id_str)
    {
        primitives::DatabasePrims &db_prims =
            primitives::PrimitivesAccess::instance()->database_prims();
        const dbtype::Id program_id =
            db_prims.convert_string_to_id(my_context, program_id_str);

        if (program_id.is_default())
        {
            // Not a valid ID
            send_plain_text("Invalid program ID: " + program_id_str, true);
            return;
        }

        // Get EntityRef
        //
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);

        if (not program_ref.valid())
        {
            // Not a valid Entity
            send_plain_text("ID does not exist: " + program_id_str, true);
            return;
        }

        // Check permissions
        //
        if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            my_context,
            program_ref,
            dbtype::ENTITYFIELD_program_source_code,
            false))
        {
            send_plain_text("Permission denied.", true);
            return;
        }

        // Check if program
        //
        dbtype::Program * const program_ptr =
            dynamic_cast<dbtype::Program *>(program_ref.get());

        if (not program_ptr)
        {
            send_plain_text("Entity is not a Program.", true);
            return;
        }

        // Get Document
        //
        dbtype::DocumentProperty source_code = program_ptr->get_source_code();

        //  Use system call to convert to multiline
        //
        text::ExternalTextMultiline multiline;

        const primitives::Result result =
            primitives::PrimitivesAccess::instance()->system_prims().
                to_external_text_multiline_unformatted(
                    my_context,
                    source_code.get_as_string(),
                    multiline,
                    false);

        if (result.is_security_violation())
        {
            send_plain_text("Permission denied while formatting lines.", true);
            text::ExternalText::clear_text_lines(multiline);
            return;
        }

        if (not result.is_success())
        {
            send_plain_text("Failed to format lines.", true);
            text::ExternalText::clear_text_lines(multiline);
            return;
        }

        // Send.
        //
        send_plain_text("Program listing:", false);

        for (text::ExternalTextMultiline::iterator line_iter =
                multiline.begin();
            line_iter != multiline.end();
            ++line_iter)
        {
            if (not output_channel_ptr->send_item(*line_iter))
            {
                LOG(error, "useragent", "process_list_program",
                    "Could not send text on channel "
                    + output_channel_ptr->get_channel_name() + ", requester "
                    + my_context.get_requester().to_string(true));

                break;
            }
        }

        text::ExternalText::clear_text_lines(multiline);
    }

    // ----------------------------------------------------------------------
    // This operation does not exist in the primitives, so temporarily doing
    // it here for now.
    void UserAgent::process_enter_edit_program(
        const std::string &program_id_str)
    {
        if (program_source_ptr or (not program_source_id.is_default()))
        {
            send_plain_text("Already editing a program! [internal error]", true);
            return;
        }

        primitives::DatabasePrims &db_prims =
            primitives::PrimitivesAccess::instance()->database_prims();
        const dbtype::Id program_id =
            db_prims.convert_string_to_id(my_context, program_id_str);

        if (program_id.is_default())
        {
            // Not a valid ID
            send_plain_text("Invalid program ID: " + program_id_str, true);
            return;
        }

        // Get EntityRef
        //
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);

        if (not program_ref.valid())
        {
            // Not a valid Entity
            send_plain_text("ID does not exist: " + program_id_str, true);
            return;
        }

        // Check permissions
        //
        if ((not security::SecurityAccess::instance()->security_check(
              security::OPERATION_GET_ENTITY_FIELD,
              my_context,
              program_ref,
              dbtype::ENTITYFIELD_program_source_code,
              false)) or (not security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                my_context,
                program_ref,
                dbtype::ENTITYFIELD_program_source_code,
                false)))
        {
            send_plain_text("Permission denied.", true);
            return;
        }

        // Check if program
        //
        dbtype::Program * const program_ptr =
            dynamic_cast<dbtype::Program *>(program_ref.get());

        if (not program_ptr)
        {
            send_plain_text("Entity is not a Program.", true);
            return;
        }

        // Save off program info to enter edit mode.
        //
        program_source_ptr = new dbtype::DocumentProperty();
        program_source_id = program_id;

        // Display simple help
        //
        send_plain_text("Editing program " + program_id_str, false);
        send_plain_text("Existing program cleared.  Enter new lines.", false);
        send_plain_text("Type " + ABORT_EDIT_STRING
            + " to exit without saving, and " + SAVE_EDIT_STRING + " to save.",
            false);
        send_plain_text("", false);
    }

    // ----------------------------------------------------------------------
    // This operation does not exist in the primitives, so temporarily doing
    // it here for now.
    void UserAgent::process_edit_program(const std::string &input)
    {
        if (not program_source_ptr)
        {
            send_plain_text("No program is being edited!", true);
            return;
        }

        if (input == ABORT_EDIT_STRING)
        {
            // Abort the edits
            //
            delete program_source_ptr;
            program_source_ptr = 0;
            program_source_id = dbtype::Id();

            send_plain_text("Editing aborted.", false);
        }
        else if (input != SAVE_EDIT_STRING)
        {
            // Add a new line
            //
            if (not program_source_ptr->append_line(input))
            {
                send_plain_text(
                    "Too many lines.  New entry is ignored.",
                    true);
            }
        }
        else
        {
            // Save it.

            // Get EntityRef
            //
            dbinterface::EntityRef program_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(program_source_id);

            if (not program_ref.valid())
            {
                // Not a valid Entity
                send_plain_text("Program no longer exists: "
                    + program_source_id.to_string(true), true);

                delete program_source_ptr;
                program_source_ptr = 0;
                program_source_id = dbtype::Id();
                return;
            }

            // Check permissions
            //
            if (not security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                my_context,
                program_ref,
                dbtype::ENTITYFIELD_program_source_code,
                false))
            {
                send_plain_text("Permission denied.", true);
                delete program_source_ptr;
                program_source_ptr = 0;
                program_source_id = dbtype::Id();
                return;
            }

            // Check if program
            //
            dbtype::Program * const program_ptr =
                dynamic_cast<dbtype::Program *>(program_ref.get());

            if (not program_ptr)
            {
                send_plain_text("Entity is no longer a Program.", true);
                delete program_source_ptr;
                program_source_ptr = 0;
                program_source_id = dbtype::Id();
                return;
            }

            if (not program_ptr->set_source_code(*program_source_ptr))
            {
                send_plain_text("Unable to save prorgam.  Aborted.", true);
            }
            else
            {
                // Since we updated the source code, we need to clear the
                // compiled version.
                //
                if (not softcode::SoftcodeAccess::instance()->uncompile(
                    program_source_id))
                {
                    send_plain_text(
                        "Program saved, but unable to uncompile.",
                        true);
                }

                send_plain_text("Done.", false);
            }

            delete program_source_ptr;
            program_source_ptr = 0;
            program_source_id = dbtype::Id();
        }
    }

    // ----------------------------------------------------------------------
    // TODO will need to be enhanced to call predetermined program.
    void UserAgent::force_look(void)
    {
        // For now, just feed a 'look' into the Channel as if the user sent
        // it.
        text::ExternalTextLine line;
        line.push_back(new text::ExternalPlainText("look"));

        input_channel_ptr->send_item(line);
        text::ExternalText::clear_text_line(line);
    }

    // ----------------------------------------------------------------------
    void UserAgent::send_location_update(const dbtype::Id &new_container)
    {
        if (data_output_channel_ptr)
        {
            std::string new_name;
            message::LocationInfoChange *location_message_ptr =
                new message::LocationInfoChange();

            if (not primitives::PrimitivesAccess::instance()->database_prims().
                convert_id_to_name(
                    my_context,
                    new_container,
                    false,
                    new_name,
                    false).is_success())
            {
                new_name = "**UNKNOWN LOCATION NAME**";
            }

            location_message_ptr->set_room_id(new_container);
            location_message_ptr->set_room_name(new_name);

            if (not data_output_channel_ptr->send_item(location_message_ptr))
            {
                delete location_message_ptr;
            }
        }
    }

    // ----------------------------------------------------------------------
    void UserAgent::send_plain_text(
        const std::string &text,
        const bool text_is_error)
    {
        text::ExternalTextLine line;

        if (text_is_error)
        {
            line.push_back(new text::ExternalFormattedText(
                text,
                false,
                false,
                false,
                false,
                text::ExternalFormattedText::COLOR_RED));
        }
        else
        {
            line.push_back(new text::ExternalPlainText(text));
        }

        if (output_channel_ptr)
        {
            if (not output_channel_ptr->send_item(line))
            {
                LOG(error, "useragent", "send_plain_text",
                    "Could not send text on channel "
                    + output_channel_ptr->get_channel_name() + ", requester "
                    + my_context.get_requester().to_string(true));
            }
        }

        text::ExternalText::clear_text_line(line);
    }

    // ----------------------------------------------------------------------
    void UserAgent::refresh_context(const bool force)
    {
        const MG_LongUnsignedInt last_update_seconds =
            last_context_refresh.get_relative_seconds();

        if (force or (last_update_seconds > MAX_SECONDS_CONTEXT_REFRESH))
        {
            my_context.reset_capabilities();
            security::SecurityAccess::instance()->
                populate_context_capabilities(my_context);
            last_context_refresh.set_to_now();
        }
    }
}
}
