/*
 * useragent_Agent.cpp
 */

#include <string>
#include <set>

#include "osinterface/osinterface_OsTypes.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Puppet.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_Capability.h"
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
#include "clientmessages/message_ClientFindEntityRequest.h"
#include "clientmessages/message_ClientFindEntityResult.h"
#include "clientmessages/message_LocationInfoChange.h"

#include "events/events_EventAccess.h"
#include "events/events_MovementSubscriptionParams.h"
#include "events/events_MovementEvent.h"
#include "events/events_EmitSubscriptionParams.h"
#include "events/events_EmitEvent.h"
#include "events/events_EventMatchedMessage.h"

#include "comminterface/comm_CommAccess.h"

#include "dbinterface/dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalFormattedText.h"
#include "text/text_StringConversion.h"
#include "text/text_StringParsing.h"

#include "security/security_Context.h"
#include "security/security_SecurityAccess.h"

#include "primitives/primitives_DatabasePrims.h"
#include "primitives/primitives_MovementPrims.h"
#include "primitives/primitives_PrimitivesAccess.h"

#include "softcode/softcode_SoftcodeAccess.h"

#include "useragent_TextChannelDocumentWriter.h"
#include "useragent_Agent.h"

namespace
{
    const std::string CLIENT_DATA_CHANNEL_NAME = "ClientData";

    const std::string QUIT_COMMAND = "QUIT";
    const std::string LIST_PROG_COMMAND = "/listprog";
    const std::string EDIT_PROG_COMMAND = "/editprog";

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
    Agent::Agent(const dbtype::Id &entity)
      : first_execute(true),
        is_player((dbinterface::DatabaseAccess::instance()->
            get_entity_type(entity) == dbtype::ENTITYTYPE_player)),
        entity_id(entity),
        session_entity_id(entity),
        my_context_ptr(0),
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
    Agent::Agent(const dbtype::Id &entity, const dbtype::Id &session_entity)
        : first_execute(true),
          is_player((dbinterface::DatabaseAccess::instance()->
              get_entity_type(entity) == dbtype::ENTITYTYPE_player)),
          entity_id(entity),
          session_entity_id(session_entity),
          my_context_ptr(0),
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
    Agent::~Agent()
    {
        delete my_context_ptr;
        delete program_source_ptr;
        delete output_channel_ptr;
        delete input_channel_ptr;
        delete data_output_channel_ptr;
        delete data_input_channel_ptr;
    }

    // ----------------------------------------------------------------------
    void Agent::process_added(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        my_pid = pid;
        setup(pid, services);
        subscribe_events();
    }

    // ----------------------------------------------------------------------
    Agent::ProcessStatus Agent::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        if (first_execute)
        {
            // We should only get called initially with this method signature,
            // which is used to welcome the player and do the initial look.
            // Otherwise, we are message/event driven only.
            // Note that it is possible we are never called with this method
            // directly by the executor if there are already messages waiting.
            //
            force_look();

            first_execute = false;

            // Send initial location update to update UI
            //
            if (data_output_channel_ptr)
            {
                dbinterface::EntityRef entity =
                    dbinterface::DatabaseAccess::instance()
                        ->get_entity(entity_id);
                dbtype::ContainerPropertyEntity *cpe =
                    dynamic_cast<dbtype::ContainerPropertyEntity *>(entity
                        .get());

                if (entity.valid() and cpe)
                {
                    send_location_update(cpe->get_contained_by());
                }
                else
                {
                    LOG(error, "useragent", "process_execute()",
                        "Could not get location of entity " +
                        entity_id.to_string(true));
                }
            }
        }

        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    Agent::ProcessStatus Agent::process_execute(
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
                                "Entity being deleted.  Terminating...", true);

                if (is_player)
                {
                    comm::CommAccess::instance()->disconnect_session(entity_id);
                }

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
    Agent::ProcessStatus Agent::process_execute(
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
                    if (process_channel_flow(services, flow_message_ptr))
                    {
                        LOG(debug, "useragent", "process_execute(rid)",
                            "Output channel closed; request process to "
                            "terminate for " + entity_id.to_string(true));
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
                                *my_context_ptr,
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
                    if (process_channel_flow(services, flow_message_ptr))
                    {
                        LOG(debug, "useragent", "process_execute(rid)",
                            "Input channel closed; request process to "
                            "terminate for "
                            + entity_id.to_string(true));
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
                    else if (messageType == message::CLIENTMESSAGE_FIND_ENTITY_REQUEST)
                    {
                        const message::ClientFindEntityRequest * const
                            match_ptr = dynamic_cast<
                                const message::ClientFindEntityRequest *>(& message);

                        if (match_ptr)
                        {
                            if (match_ptr->get_search_string().empty())
                            {
                                // Search by type only
                                process_match_type(*match_ptr);
                            }
                            else
                            {
                                process_match_name(*match_ptr);
                            }
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
                        process_data_channel_message(message);
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
    std::string Agent::process_get_name(const executor::PID pid)
    {
        return std::string("Agent for ") + entity_id.to_string(true);
    }

    // ----------------------------------------------------------------------
    bool Agent::process_delete_when_finished(const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    Agent::ErrorMessageText Agent::process_get_error_text(
        const executor::PID pid)
    {
        return ErrorMessageText();
    }

    // ----------------------------------------------------------------------
    void Agent::process_killed(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // Nothing to do when killed.
    }

    // ----------------------------------------------------------------------
    void Agent::process_finished(const executor::PID pid)
    {
        // Nothing to clean up.  All resources are automatically cleaned up.

        // Just set these to null so the destructor won't delete them.
        output_channel_ptr = 0;
        input_channel_ptr = 0;
        data_output_channel_ptr = 0;
        data_input_channel_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool Agent::process_channel_flow(
        executor::ProcessServices &services,
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
    void Agent::process_data_channel_message(
        const message::ClientMessage &message)
    {
        LOG(error, "useragent", "process_data_channel_message()",
            "Unknown message type from data input channel.");
    }

    // ----------------------------------------------------------------------
    void Agent::process_location_change(
        events::MovementEvent * const movement_event_ptr)
    {
        if (movement_event_ptr and
            (movement_event_ptr->get_from() != movement_event_ptr->get_to()))
        {
            events::EmitSubscriptionParams params(
                dbtype::Id(),
                movement_event_ptr->get_to(),
                entity_id);
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
    void Agent::process_emit(
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
    void Agent::subscribe_events(void)
    {
        // Get my Entity so I can use info about it in the subscriptions.
        //
        dbinterface::EntityRef requester_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

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
        move_params.add_who(entity_id);
        location_subscription_id = events::EventAccess::instance()->subscribe(
            move_params,
            callback);

        // Subscribe to room emits for our current location.
        //
        events::EmitSubscriptionParams emit_params(
            dbtype::Id(),
            requester_entity_ptr->get_contained_by(),
            entity_id);
        emit_subscription_id = events::EventAccess::instance()->subscribe(
            emit_params,
            callback);

        // Subscribe to private messages
        //
        events::EmitSubscriptionParams private_message_params(
            dbtype::Id(),
            entity_id,
            entity_id);
        private_subscription_id = events::EventAccess::instance()->subscribe(
            private_message_params,
            callback);
    }

    // ----------------------------------------------------------------------
    bool Agent::process_user_command(std::string &command_str)
    {
        bool keep_processing = true;

        refresh_context();

        if (command_str.empty())
        {
            // Do nothing for empty strings.
        }
        else if (command_str == QUIT_COMMAND)
        {
            keep_processing = process_quit_command();
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
            else if (first_command == "/makeplayer")
            {
                // Temporary make player command.
                //
                if (separator_pos == std::string::npos)
                {
                    create_player("");
                }
                else
                {
                    create_player(command_str.substr(separator_pos));
                }
            }
            else if (first_command == "/makepuppet")
            {
                // Temporary make puppet command.
                //
                if (separator_pos == std::string::npos)
                {
                    create_puppet("");
                }
                else
                {
                    create_puppet(command_str.substr(separator_pos));
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
                    *my_context_ptr,
                    command_str);

                if (found_id.is_default())
                {
                    result = db_prims.match_name_to_id(
                        *my_context_ptr,
                        command_str,
                        true,
                        dbtype::ENTITYTYPE_action,
                        found_id,
                        is_ambiguous,
                        false);
                }

                // If found, use
                //
                if (result.is_success() and (not found_id.is_default()))
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
                        *my_context_ptr,
                        first_command);

                    if (found_id.is_default())
                    {
                        result = db_prims.match_name_to_id(
                            *my_context_ptr,
                            first_command,
                            true,
                            dbtype::ENTITYTYPE_action,
                            found_id,
                            is_ambiguous,
                            false);
                    }

                    if (result.is_success() and (not found_id.is_default()))
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
    void Agent::process_action(
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

        if ((not command_entity) and
            dynamic_cast<dbtype::Exit *>(action_ref.get()))
        {
            process_exit(action_ref, arguments);
        }
        else if (command_entity)
        {
            // This is a command (Program).  Execute it if we have permission.
            //
            primitives::Result result;

            result = primitives::PrimitivesAccess::instance()->database_prims().
                can_use_action(
                    *my_context_ptr,
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
                    my_context_ptr->get_requester(),
                    command_entity->get_first_action_target());
            events::TextChannel * const prog_output_ptr =
                make_prog_output_channel(channel_subtype);
            dbtype::Id redirect_entity;
            dbinterface::EntityRef redirect_entity_ref;
            std::string redirect_property;
            bool has_property_value = false; // Not used
            std::string redirect_property_value; // Not used
            bool ambiguous = false;
            const size_t redirect_sep = arguments.find(REDIRECT_SYM);

            prog_context_ptr->set_run_as_requester();
            modify_program_context(*prog_context_ptr);

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
                            *my_context_ptr,
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
                            *my_context_ptr,
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
                        session_entity_id,
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
    void Agent::process_exit(
        dbinterface::EntityRef &exit_ref,
        std::string &arguments)
    {
        // This is an exit, do the move.
        //
        primitives::Result move_result =
            primitives::PrimitivesAccess::instance()->
                movement_prims().move_requester_with_exit(
                *my_context_ptr,
                exit_ref.id(),
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

    // ----------------------------------------------------------------------
    void Agent::process_execute_entity(
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
    void Agent::process_match_name(
        const message::ClientFindEntityRequest &message)
    {
        if (data_output_channel_ptr)
        {
            primitives::DatabasePrims &db_prims =
                primitives::PrimitivesAccess::instance()->database_prims();
            dbtype::Id found_id;
            bool is_ambiguous = false;
            primitives::Result result;
            message::ClientFindEntityResult * const result_message_ptr =
                new message::ClientFindEntityResult();

            result_message_ptr->set_message_response_flag(true);
            result_message_ptr->set_message_request_id(
                message.get_message_request_id());

            // Try and convert it to an ID first, and if not then do a normal
            // search.
            //
            found_id = db_prims.convert_string_to_id(
            *my_context_ptr,
                message.get_search_string());

            if (found_id.is_default())
            {
                result = db_prims.match_name_to_id(
                    *my_context_ptr,
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
                dbinterface::EntityMetadata metadata =
                    dbinterface::DatabaseAccess::instance()->get_entity_metadata(
                        found_id);

                if (metadata.valid())
                {
                    result_message_ptr->add_entity(
                        metadata.get_id(),
                        metadata.get_name(),
                        metadata.get_type());
                }
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
            if (not data_output_channel_ptr->send_item(result_message_ptr))
            {
                delete result_message_ptr;
            }
        }
    }

    // ----------------------------------------------------------------------
    void Agent::process_match_type(
        const message::ClientFindEntityRequest &message)
    {
        // Only search on Entities we own.  Guaranteed no security issues.
        //
        if (data_output_channel_ptr)
        {
            dbinterface::DatabaseAccess *const db =
                dbinterface::DatabaseAccess::instance();
            message::ClientFindEntityResult *const result_message_ptr =
                new message::ClientFindEntityResult();

            result_message_ptr->set_message_response_flag(true);
            result_message_ptr->set_message_request_id(
                message.get_message_request_id());

            const dbtype::Entity::IdVector result =
                db->find(entity_id.get_site_id(),
                    message.get_entity_type(),
                    entity_id.get_entity_id(),
                    std::string());

            if (not result.empty())
            {
                const dbinterface::MetadataVector metadata =
                    dbinterface::DatabaseAccess::instance()->get_entity_metadata(
                        result);

                for (dbinterface::MetadataVector::const_iterator iter =
                        metadata.begin();
                    iter != metadata.end();
                    ++iter)
                {
                    result_message_ptr->add_entity(
                        iter->get_id(),
                        iter->get_name(),
                        iter->get_type());
                }
            }

            // Send message back to client.
            //
            if (not data_output_channel_ptr->send_item(result_message_ptr))
            {
                delete result_message_ptr;
            }
        }
    }

    // ----------------------------------------------------------------------
    // This operation does not exist in the primitives, so temporarily doing
    // it here for now.
    void Agent::process_list_program(const std::string &program_id_str)
    {
        primitives::DatabasePrims &db_prims =
            primitives::PrimitivesAccess::instance()->database_prims();
        const dbtype::Id program_id =
            db_prims.convert_string_to_id(*my_context_ptr, program_id_str);

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
            *my_context_ptr,
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
                    *my_context_ptr,
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
                    + my_context_ptr->get_requester().to_string(true));

                break;
            }
        }

        text::ExternalText::clear_text_lines(multiline);
    }

    // ----------------------------------------------------------------------
    // This operation does not exist in the primitives, so temporarily doing
    // it here for now.
    void Agent::process_enter_edit_program(
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
            db_prims.convert_string_to_id(*my_context_ptr, program_id_str);

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
              *my_context_ptr,
              program_ref,
              dbtype::ENTITYFIELD_program_source_code,
              false)) or (not security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                *my_context_ptr,
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
    void Agent::process_edit_program(const std::string &input)
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
                *my_context_ptr,
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
    void Agent::force_look(void)
    {
        // For now, just feed a 'look' into the Channel as if the user sent
        // it.
        text::ExternalTextLine line;
        line.push_back(new text::ExternalPlainText("look"));

        input_channel_ptr->send_item(line);
        text::ExternalText::clear_text_line(line);
    }

    // ----------------------------------------------------------------------
    events::TextChannel *
    Agent::make_prog_output_channel(const std::string &subtype)
    {
        return new events::TextChannel(
            FOREGROUND_PROG_CHAN_NAME,
            subtype);
    }

    // ----------------------------------------------------------------------
    void Agent::send_location_update(const dbtype::Id &new_container)
    {
        if (data_output_channel_ptr)
        {
            std::string new_name;
            message::LocationInfoChange *location_message_ptr =
                new message::LocationInfoChange();

            if (not primitives::PrimitivesAccess::instance()->database_prims().
                convert_id_to_name(
                    *my_context_ptr,
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
    void Agent::send_plain_text(
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
                    + my_context_ptr->get_requester().to_string(true));
            }
        }

        text::ExternalText::clear_text_line(line);
    }

    // ----------------------------------------------------------------------
    void Agent::refresh_context(const bool force)
    {
        if (not my_context_ptr)
        {
            my_context_ptr = new security::Context(entity_id, dbtype::Id());
        }

        const MG_LongUnsignedInt last_update_seconds =
            last_context_refresh.get_relative_seconds();

        if (force or (last_update_seconds > MAX_SECONDS_CONTEXT_REFRESH))
        {
            my_context_ptr->reset_capabilities();
            security::SecurityAccess::instance()->
                populate_context_capabilities(*my_context_ptr);
            modify_refreshed_context();
            last_context_refresh.set_to_now();
        }
    }

    // ----------------------------------------------------------------------
    void Agent::modify_refreshed_context(void)
    {
    }

    // ----------------------------------------------------------------------
    void Agent::modify_program_context(security::Context &program_context)
    {
    }

    // ----------------------------------------------------------------------
    // TODO Temporary code for demo purposes.
    void Agent::create_player(const std::string &params)
    {
        if (not my_context_ptr->has_capability(security::CAPABILITY_CREATE_PLAYER))
        {
            send_plain_text("Permission denied.", true);
            return;
        }

        const text::StringParsing::SplitStrings args =
            text::StringParsing::split_string(text::trim_copy(params), " ", true);


        if (args.size() != 2)
        {
            send_plain_text("How to use: <player_name> <player_password>", false);
            return;
        }

        dbinterface::DatabaseAccess *const db =
            dbinterface::DatabaseAccess::instance();

        dbinterface::EntityRef entity = db->get_entity(entity_id);
        dbinterface::EntityRef player_entity;
        dbtype::Player *player_ptr = 0;
        dbtype::Id my_location = dynamic_cast<dbtype::Player *>(entity.get())->
            get_contained_by();

        // create player.  Assign owner to itself.
        db->new_entity(
            dbtype::ENTITYTYPE_player,
            entity_id.get_site_id(),
            dbtype::Id(entity_id.get_site_id(), 1),
            args[0],
            player_entity);

        if (not player_entity.valid())
        {
            send_plain_text(
                "Could not create player " + args[0] + ".  Maybe a duplicate?", true);
            return;
        }

        player_ptr = dynamic_cast<dbtype::Player *>(player_entity.get());
        player_ptr->set_entity_owner(player_entity.id());

        // set display name to player name
        // set password
        player_ptr->set_display_name(args[0]);
        player_ptr->set_password(args[1]);

        // set home and contained by to current location
        player_ptr->set_player_home(my_location);
        player_ptr->set_contained_by(my_location);

        // create look application properties
        //  set default /look/shortdesc string
        dbtype::PropertySecurity security;
        security.set_other_security_flag(dbtype::SECURITYFLAG_read, true);
        player_ptr->add_application("look", player_entity.id(), security);
        dbtype::StringProperty string_prop;
        string_prop.set("Please change my description");
        player_ptr->set_property("/look/shortdesc", string_prop);

        // Look up capabilities and add to them
        //
        static const std::set<std::string> CAPABILITIES_TO_ADD =
            {security::capability_to_string(security::CAPABILITY_BUILDER),
             security::capability_to_string(security::CAPABILITY_SEND_TEXT_ROOM_UNRESTRICTED),
             security::capability_to_string(security::CAPABILITY_SEND_TEXT_ENTITY),
             security::capability_to_string(security::CAPABILITY_CHARACTER_FIND_BY_NAME_AFAR),
             security::capability_to_string(security::CAPABILITY_ANY_ID_TO_NAME),
             security::capability_to_string(security::CAPABILITY_CONNECTION_CHECK)};

        for (std::set<std::string>::const_iterator iter = CAPABILITIES_TO_ADD.begin();
            iter != CAPABILITIES_TO_ADD.end();
            ++iter)
        {
            const dbtype::Entity::IdVector search_result = db->find(
                entity_id.get_site_id(),
                dbtype::ENTITYTYPE_capability,
                0,
                *iter,
                true);

            dbinterface::EntityRef cap_entity = db->get_entity(search_result.front());
            dbtype::Capability *const cap_ptr =
                dynamic_cast<dbtype::Capability *>(cap_entity.get());

            if (CAPABILITIES_TO_ADD.count(cap_ptr->get_entity_name()))
            {
                cap_ptr->add_to_group(player_entity.id());
            }
        }

        send_plain_text("Player " + args[0] + " created.", false);
    }

    // ----------------------------------------------------------------------
    // TODO Temporary code for demo purposes.
    void Agent::create_puppet(const std::string &params)
    {
        // Check security / put out usage
        //
        if (not my_context_ptr->has_capability(security::CAPABILITY_CREATE_PLAYER))
        {
            send_plain_text("Permission denied.", true);
            return;
        }

        const text::StringParsing::SplitStrings args =
            text::StringParsing::split_string(text::trim_copy(params), " ", true);

        if (args.size() != 3)
        {
            send_plain_text("How to use: <player_name> <puppet_name> <command_name>", false);
            return;
        }

        dbinterface::DatabaseAccess * const db =
            dbinterface::DatabaseAccess::instance();

        // Get player
        //
        const dbtype::Entity::IdVector players = db->find(
            entity_id.get_site_id(),
            dbtype::ENTITYTYPE_player,
            0,
            args[0],
            true);

        if (players.empty())
        {
            send_plain_text("Cannot find player.", true);
            return;
        }

        if (not db->find(
            entity_id.get_site_id(),
            dbtype::ENTITYTYPE_command,
            players.front().get_entity_id(),
            args[2],
            true).empty())
        {
            send_plain_text("Action name already exists on player.", true);
            return;
        }

        dbinterface::EntityRef entity;
        dbtype::Puppet *puppet_ptr = 0;

        // Make puppet
        db->new_entity(
            dbtype::ENTITYTYPE_puppet,
            entity_id.get_site_id(),
            players.front(),
            args[1],
            entity);

        if (not entity.valid())
        {
            send_plain_text(
                "Could not create puppet " + args[1], true);
            return;
        }

        // Set home/location to player
        // add short description
        puppet_ptr = dynamic_cast<dbtype::Puppet *>(entity.get());
        puppet_ptr->set_thing_home(players.front());
        puppet_ptr->set_contained_by(players.front());

        dbtype::PropertySecurity security;
        security.set_other_security_flag(dbtype::SECURITYFLAG_read, true);
        puppet_ptr->add_application("look", entity.id(), security);
        dbtype::StringProperty string_prop;
        string_prop.set("Please change my puppet description");
        puppet_ptr->set_property("/look/shortdesc", string_prop);

        // Create command on player that links to puppet
        //
        dbinterface::EntityRef command;
        dbtype::Command *command_ptr = 0;

        db->new_entity(
            dbtype::ENTITYTYPE_command,
            entity_id.get_site_id(),
            players.front(),
            args[2],
            command);

        if (not command.valid())
        {
            send_plain_text(
                "Could not create command " + args[2], true);
            return;
        }

        command_ptr = dynamic_cast<dbtype::Command *>(command.get());
        command_ptr->set_action_contained_by(players.front());
        command_ptr->add_action_target(entity.id());
        dbtype::Command::CommandList commands;
        commands.push_back(args[2]);
        command_ptr->set_action_commands(commands);

        send_plain_text(
            "Puppet " + args[1] + " created with command " + args[2],
            false);
    }
}
}
