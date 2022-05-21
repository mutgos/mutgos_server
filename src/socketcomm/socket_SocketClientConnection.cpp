/*
 * socket_SocketClientConnection.cpp
 */

#include <string>
#include <string.h>
#include <stddef.h>
#include <vector>

#include <boost/algorithm/string/replace.hpp>

#include "osinterface/osinterface_OsTypes.h"

#include "text/text_StringConversion.h"

#include "osinterface/osinterface_OsTypes.h"

#include "comminterface/comm_ClientSession.h"

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "dbtypes/dbtype_Id.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "comminterface/comm_RouterSessionManager.h"

#include "socket_SocketDriver.h"
#include "socket_RawSocketConnection.h"
#include "socket_SocketClientConnection.h"

#include "text/text_Utf8Tools.h"
#include "text/text_ExternalTextConverter.h"
#include "text/text_AnsiConverter.h"
#include "text/text_ExternalText.h"

#define TARGET_PENDING_MESSAGE_BYTES 4096
#define CHANNEL_STACK_INITIAL_SIZE 4

#define INCOMING_LINES_ACK 5

#define TELNET_LF '\n'

namespace
{
    // TODO Update if name changes
    const std::string SESSION_AGENT_CHANNEL_NAME = "Session Agent";
    const std::string TELNET_CR(1, '\r');

    const unsigned int MAX_INACTIVE_PUPPET_TIME = 600;
}

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    SocketClientConnection::SocketClientConnection(
        mutgos::socket::SocketDriver *driver,
        boost::shared_ptr<mutgos::socket::RawSocketConnection> connection,
        const std::string &source)
      : client_window_size(0),
        max_pending_data_size(0),
        client_type(comm::ClientConnection::CLIENT_TYPE_INTERACTIVE),
        client_source(source),
        client_blocked(false),
        client_connected(false),
        client_do_reconnect(false),
        requested_service(false),
        config_ansi_enabled(true),
        pending_ids_message_size(0),
        ack_lines_received_from_client(0),
        next_input_ser_id(1),
        channel_main_input_id(0),
        client_session_ptr(0),
        driver_ptr(driver),
        raw_connection(connection),
        command_processor(this)
    {
        if (not driver)
        {
            LOG(fatal, "socket", "SocketClientConnection",
                "driver pointer is null!  Crash will likely follow...");
        }

        if (not raw_connection)
        {
            LOG(fatal, "socket", "SocketClientConnection",
                "raw_connection pointer is null!  Crash will follow...");
        }

        if (not connection.get())
        {
            LOG(fatal, "socket", "SocketClientConnection",
                "Raw connection pointer is null!  Crash will likely follow...");
        }

        if (source.empty())
        {
            client_source = "UNKNOWN";
        }

        raw_connection->set_client(this);
        raw_connection->set_timer(config::comm::auth_time());

        channel_input_stack.reserve(CHANNEL_STACK_INITIAL_SIZE);
        channel_output_stack.reserve(CHANNEL_STACK_INITIAL_SIZE);

        client_window_size = raw_connection->get_socket_send_buffer_size();
        max_pending_data_size = client_window_size;

        last_puppet_check_time.set_to_now();

        if (not client_window_size)
        {
            LOG(fatal, "socket", "SocketClientConnection",
                "client_window_size is 0!  Crash will follow...");
        }

        LOG(debug, "socket", "SocketClientConnection",
            "Got a connection to " + source);
    }

    // ----------------------------------------------------------------------
    SocketClientConnection::~SocketClientConnection()
    {
        // This will not cause a double-delete because the Driver should
        // already know it is in the middle of deleting this.
        raw_connection->client_released();

        client_disconnect();
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::stop(void)
    {
        disconnect_socket();
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::do_work(void)
    {
        requested_service = false;

        if (client_connected)
        {
            if (client_do_reconnect)
            {
                // Tell client session that we are ready to get pending data
                // after a reconnect
                client_session_ptr->client_data_acknowledge_reconnect(0);
                client_do_reconnect = false;
            }
            else if (ack_lines_received_from_client > INCOMING_LINES_ACK)
            {
                // ACK any pending lines with the client session.
                // If we've gotten enough lines from the client, try and lower
                // how many lines we haven't ACKed yet because the client has
                // likely seen them because we're getting data from them.
                //
                ack_outgoing_data(true);
                ack_lines_received_from_client = 0;

                // Every 15 minutes, close any puppet channels that haven't
                // had any recent activity.
                //
                if (last_puppet_check_time.get_relative_seconds() > MAX_INACTIVE_PUPPET_TIME)
                {
                    std::vector<comm::ChannelId> channels_to_close;

                    for (std::map<comm::ChannelId, PuppetNameTimestamp>::iterator
                            puppet_iter = puppet_channel_info.begin();
                        puppet_iter != puppet_channel_info.end();
                        ++puppet_iter)
                    {
                        if (puppet_iter->second.second.get_relative_seconds() >
                            MAX_INACTIVE_PUPPET_TIME)
                        {
                            channels_to_close.push_back(puppet_iter->first);
                        }
                    }

                    while (not channels_to_close.empty())
                    {
                        client_session_ptr->client_request_channel_close(
                            channels_to_close.back());
                        channels_to_close.pop_back();
                    }

                    last_puppet_check_time.set_to_now();
                }
            }
            else if (not client_blocked)
            {
                ack_outgoing_data(false);
            }

            if (not client_blocked)
            {
                if (not outgoing_control_buffer.empty())
                {
                    // Add the control buffer to what's going out so we can
                    // send it all at once.
                    //
                    outgoing_text_buffer.append(outgoing_control_buffer);
                    outgoing_control_buffer.clear();
                    outgoing_control_buffer.shrink_to_fit();
                }

                if (not outgoing_text_buffer.empty())
                {
                    if (raw_connection->raw_send(
                        outgoing_text_buffer.c_str(),
                        outgoing_text_buffer.size()))
                    {
                        // Wait for it to confirm sending.
                        client_blocked = true;
                    }
                    else
                    {
                        // Error condition.  We should always know the state
                        // of the connection.  It may have disconnected
                        // and not yet notified us, which is an expected
                        // condition.  If we are still connected, however,
                        // that is an error condition.
                        if (raw_connection->raw_is_connected())
                        {
                            LOG(error, "socket", "do_work",
                                "Unable to send buffer to source "
                                  + client_source + ".  Disconnecting.");
                        }

                        raw_disconnected();
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    osinterface::OsTypes::UnsignedInt
    SocketClientConnection::get_client_window_size(void)
    {
        return client_window_size;
    }

    // ----------------------------------------------------------------------
    bool SocketClientConnection::client_is_enhanced(void)
    {
        return false;
    }

    // ----------------------------------------------------------------------
    SocketClientConnection::ClientType SocketClientConnection::get_client_type(void)
    {
        return client_type;
    }

    // ----------------------------------------------------------------------
    bool SocketClientConnection::client_is_send_blocked(void)
    {
        return client_blocked;
    }

    // ----------------------------------------------------------------------
    bool SocketClientConnection::client_is_connected(void)
    {
        return client_connected;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::client_disconnect(void)
    {
        if (client_connected)
        {
            client_connected = false;
            client_blocked = true;

            raw_connection->raw_disconnect();
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdType SocketClientConnection::client_get_site_id(void)
    {
        return client_entity_id.get_site_id();
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::client_set_site_id(
        const dbtype::Id::SiteIdType site_id)
    {
        if (client_entity_id.is_entity_default())
        {
            client_entity_id = dbtype::Id(site_id, 0);
        }
    }

    // ----------------------------------------------------------------------
    const std::string& SocketClientConnection::client_get_source(void)
    {
        return client_source;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::client_set_entity_id(const dbtype::Id &entity_id)
    {
        client_entity_id = entity_id;
    }

    // ----------------------------------------------------------------------
    dbtype::Id SocketClientConnection::client_get_entity_id(void)
    {
        return client_entity_id;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::client_set_session(
        comm::ClientSession *session_ptr)
    {
        if (session_ptr and (not client_session_ptr))
        {
            client_session_ptr = session_ptr;
            driver_ptr->add_reference(this);
            raw_connection->cancel_timer();
        }
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::send_to_input_channel(
        text::ExternalTextLine *line_ptr,
        const bool to_agent)
    {
        if (line_ptr)
        {
            if (channel_input_stack.empty() or (not client_session_ptr))
            {
                send_control_text_raw("WARNING: No channels active.  "
                                      "Text has been lost.\n");
                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else if (to_agent and (not channel_main_input_id))
            {
                send_control_text_raw("WARNING: No agent channel found.  "
                                      "Text has been lost.\n");
                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else
            {
                client_session_ptr->client_data(
                    (to_agent ?
                        channel_main_input_id :
                        channel_input_stack.back().first),
                    get_next_incoming_ser_id(),
                    line_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    // Simulate proper functioning; not actually used for raw sockets since
    // the client has no concept of this functionality.
    SocketClientConnection::SendReturnCode
    SocketClientConnection::client_send_acknowledge_data(
        const mutgos::comm::MessageSerialId ser_id)
    {
        if (not client_connected)
        {
            return SEND_DISCONNECTED;
        }
        else if (client_blocked)
        {
            return SEND_BLOCKED;
        }

        return SEND_OK;
    }

    // ----------------------------------------------------------------------
    // Simulate proper functioning; not actually used for raw sockets since
    // the client has no concept of this functionality.
    SocketClientConnection::SendReturnCode
    SocketClientConnection::client_send_acknowledge_data_reconnect(
        const mutgos::comm::MessageSerialId ser_id)
    {
        client_do_reconnect = true;

        // Add known channels, so we know what they are when the pending data
        // starts flowing.
        //
        const comm::ClientChannelInfoVector channel_info =
            client_session_ptr->get_current_channel_info();

        for (comm::ClientChannelInfoVector::const_iterator
                iter = channel_info.begin();
            iter != channel_info.end();
            ++iter)
        {
            ChannelStack &stack = iter->channel_is_outgoing() ?
                channel_output_stack : channel_input_stack;

            stack.push_back(std::make_pair(
                iter->get_channel_id(),
                iter->channel_is_blocked()));

            // See if channel is our 'main input' and update if so.
            //
            if (iter->get_channel_name() == SESSION_AGENT_CHANNEL_NAME)
            {
                channel_main_input_id = iter->get_channel_id();
            }
        }

        request_service();

        return SEND_OK;
    }

    // ----------------------------------------------------------------------
    // Note that currently, output channels are maintained solely for
    // informational purposes; they have no impact on how data is output
    // to the client.
    SocketClientConnection::SendReturnCode
      SocketClientConnection::client_channel_status_changed(
        const comm::MessageSerialId ser_id,
        const message::ChannelStatusChange &channel_status)
    {
        pending_ser_ack(ser_id);
        const bool out = channel_status.get_channel_out();
        const comm::ChannelId channel_id = channel_status.get_channel_id();
        ChannelStack &stack = out ? channel_output_stack : channel_input_stack;
        const bool is_puppet =
            channel_status.get_channel_name().find("Puppet ") == 0;

        // TODO To make it in time for a gamedev demo, puppet channel
        //      support is not fully complete.  Input channels are ignored
        //      and channels remain open forever unless the server side
        //      closes them, meaning puppets won't deactivate on their own.
        //      Block/unblock is ignored for puppets

        switch (channel_status.get_channel_status())
        {
            case message::ChannelStatus::CHANNEL_STATUS_open:
            {
                if (is_puppet)
                {
                    if (out)
                    {
                        puppet_channel_info[channel_status.get_channel_id()] =
                            PuppetNameTimestamp(
                                channel_status.get_channel_subtype(),
                                dbtype::TimeStamp());
                    }
                }
                else
                {
                    bool need_add = true;

                    // Do to reconnect, we may get channel twice.  Check for
                    // existance before adding.
                    for (ChannelStack::iterator stack_iter = stack.begin();
                         stack_iter != stack.end();
                         ++stack_iter)
                    {
                        if (stack_iter->first == channel_id)
                        {
                            // Found it. Skip the add.
                            need_add = false;
                            break;
                        }
                    }

                    if (need_add)
                    {
                        stack.push_back(std::make_pair(channel_id, false));

                        // See if channel is our 'main input' and update if so.
                        //
                        if (channel_status.get_channel_name() ==
                            SESSION_AGENT_CHANNEL_NAME)
                        {
                            channel_main_input_id = channel_id;
                        }
                    }
                }

                break;
            }

            case message::ChannelStatus::CHANNEL_STATUS_close:
            {
                if (is_puppet)
                {
                    if (out)
                    {
                        puppet_channel_info.erase(
                            channel_status.get_channel_id());
                    }
                }
                else
                {
                    // Channel has been removed.
                    //
                    for (ChannelStack::iterator stack_iter = stack.begin();
                         stack_iter != stack.end();
                         ++stack_iter)
                    {
                        if (stack_iter->first == channel_id)
                        {
                            // Found it. Delete.
                            //

                            if (channel_id == channel_main_input_id)
                            {
                                channel_main_input_id = 0;
                            }

                            stack.erase(stack_iter);
                            break;
                        }
                    }
                }

                break;
            }

            case message::ChannelStatus::CHANNEL_STATUS_block:
            {
                // Channel is now blocked.
                //
                for (ChannelStack::iterator stack_iter = stack.begin();
                     stack_iter != stack.end();
                     ++stack_iter)
                {
                    if (stack_iter->first == channel_id)
                    {
                        // Found it. Update.
                        //
                        stack_iter->second = true;
                        break;
                    }
                }

                break;
            }

            case message::ChannelStatus::CHANNEL_STATUS_unblock:
            {
                // Channel is now unblocked.
                //
                for (ChannelStack::iterator stack_iter = stack.begin();
                     stack_iter != stack.end();
                     ++stack_iter)
                {
                    if (stack_iter->first == channel_id)
                    {
                        // Found it. Update.
                        //
                        stack_iter->second = false;
                        break;
                    }
                }

                break;
            }

            default:
            {
                LOG(error, "socket", "client_channel_status_changed",
                    "Unknown status: " + message::channel_status_to_string(
                        channel_status.get_channel_status()));
                break;
            }
        }

        // Since this doesn't go to the client, no need to block.
        return SocketClientConnection::SEND_OK;
    }

    // ----------------------------------------------------------------------
    SocketClientConnection::SendReturnCode SocketClientConnection::client_send_data(
        const mutgos::comm::ChannelId channel_id,
        const mutgos::comm::MessageSerialId ser_id,
        const mutgos::text::ExternalTextLine &text_line)
    {
        SendReturnCode status = SEND_OK;

        if (not client_connected)
        {
            status = SEND_DISCONNECTED;
        }
        else if (client_blocked)
        {
            status = SEND_BLOCKED;
        }
        else
        {
            std::map<comm::ChannelId, PuppetNameTimestamp>::iterator puppet_iter =
                puppet_channel_info.find(channel_id);

            // Not known to be disconnected or blocked, so try queue up to
            // send.  This involves coding it for sockets, which means
            // converting to extended ASCII and ANSI color.
            //
            std::string formatted_output;

            if (puppet_iter != puppet_channel_info.end())
            {
                formatted_output = puppet_iter->second.first + "> ";
                // Update timestamp to show recent use.
                puppet_iter->second.second.set_to_now();
            }

            formatted_output += (config_ansi_enabled ?
                text::to_ansi(text_line) :
                text::ExternalText::to_string(text_line));

            formatted_output = text::convert_utf8_to_extended(formatted_output);

            // Add to outgoing text
            //
            status = send_text_line(formatted_output);
            pending_ser_ack(ser_id, formatted_output.size() + 1);
        }

        return status;
    }

    // ----------------------------------------------------------------------
    // Not supported for text-only connections.
    SocketClientConnection::SendReturnCode SocketClientConnection::client_send_data(
        const mutgos::comm::ChannelId channel_id,
        const mutgos::comm::MessageSerialId ser_id,
        const mutgos::message::ClientMessage &client_message)
    {
        return SEND_NOT_SUPPORTED;
    }

    // ----------------------------------------------------------------------
    bool SocketClientConnection::send_control_text_raw(const std::string &text)
    {
        const bool result = client_connected and
                            outgoing_control_buffer.empty();

        if (result and (not text.empty()))
        {
            outgoing_control_buffer = text;
            request_service();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::raw_ready(void)
    {
        client_connected = true;

        command_processor.show_login_screen();
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::raw_send_complete(void)
    {
        outgoing_text_buffer.clear();

        if (client_connected and client_blocked)
        {
            client_blocked = false;

            if (client_session_ptr)
            {
                client_session_ptr->client_unblocked();
            }

            request_service();
        }
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::raw_disconnected(void)
    {
        if (client_connected)
        {
            LOG(debug, "socket", "raw_disconnected",
                "Client disconnected.  Source " + client_source + ", entity "
                + client_entity_id.to_string(true));

            client_connected = false;
            client_blocked = true;

            if (client_session_ptr)
            {
                client_session_ptr->client_disconnected();
            }
        }

        // At this point we will never be connected again.
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::raw_data(
        const char *data_ptr,
        const size_t data_size)
    {
        if (data_size)
        {
            std::string data(data_ptr, data_size);
            process_raw_incoming_data(data);
        }
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::raw_timer_expired(void)
    {
        LOG(warning, "socket", "raw_timer_expired",
            "Client " + client_source + " never successfully authenticated."
                                        "  Disconnecting.");

        disconnect_socket();
    }

    // ----------------------------------------------------------------------
    SocketClientConnection::SendReturnCode SocketClientConnection::send_text_line(
        const std::string &text)
    {
        SendReturnCode status = SEND_OK;

        if (not client_connected)
        {
            status = SEND_DISCONNECTED;
        }
        else if (client_blocked)
        {
            status = SEND_BLOCKED;
        }
        else
        {
            // Not known to be disconnected or blocked, so try queue up to
            // send.
            //

            // Add to outgoing text
            //
            outgoing_text_buffer += text;
            outgoing_text_buffer += TELNET_LF;

            // Determine if we can send more.
            //
            if (outgoing_text_buffer.size() >= max_pending_data_size)
            {
                // We're done for now.
                //
                client_blocked = true;
                status = SEND_OK_BLOCKED;
            }

            request_service();
        }

        return status;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::disconnect_socket(void)
    {
        if (client_connected)
        {
            client_connected = false;
            client_blocked = true;

            raw_connection->raw_disconnect();

            if (client_session_ptr)
            {
                client_session_ptr->client_disconnected();
            }
        }
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::pending_ser_ack(
        const comm::MessageSerialId ser_id,
        const MG_UnsignedInt size)
    {
        pending_serial_ids.push_back(std::make_pair(ser_id, size));
        pending_ids_message_size += size;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::ack_outgoing_data(
        const bool from_client_input)
    {
        // Determine how many pending mesages we want left after this call
        // (in bytes).
        //
        MG_LongUnsignedInt target_size = TARGET_PENDING_MESSAGE_BYTES;

        if (from_client_input)
        {
            const MG_LongUnsignedInt buffer_half = target_size / 2;
            const MG_LongUnsignedInt used_buffer_half =
                pending_ids_message_size / 2;

            target_size = (buffer_half > used_buffer_half ?
                used_buffer_half : buffer_half);
        }

        // ACK messages until the size is low enough.
        //
        if ((pending_ids_message_size > target_size) and
            (not pending_serial_ids.empty()))
        {
            PendingSerialIds::iterator pending_ids_iter =
                pending_serial_ids.begin();

            while ((pending_ids_message_size > target_size) and
                 (pending_ids_iter != pending_serial_ids.end()))
            {
                pending_ids_message_size -= pending_ids_iter->second;
                ++pending_ids_iter;
            }

            if (pending_ids_iter == pending_serial_ids.end())
            {
                // Jump back one so we can get the ID.
                --pending_ids_iter;
                client_session_ptr->client_data_acknowledge(
                    pending_ids_iter->first);
                ++pending_ids_iter;
                // Nothing will be pending, so we must be at 0 bytes.
                pending_ids_message_size = 0;
            }
            else
            {
                client_session_ptr->client_data_acknowledge(
                    pending_ids_iter->first);
            }

            pending_serial_ids.erase(pending_serial_ids.begin(), pending_ids_iter);
        }
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::process_raw_incoming_data(std::string &data)
    {
        if (not data.empty())
        {
            size_t line_start_index = 0;
            size_t line_end_index = 0;
            std::string line;

            // Remove any CRs, since those are not supposed to be there.
            boost::replace_all(data, TELNET_CR, std::string());
            incoming_text_buffer.append(data);
            const size_t string_end_index = data.size() - 1;

            while (line_start_index <= string_end_index)
            {
                line_end_index = incoming_text_buffer.find(
                    TELNET_LF,
                    line_start_index);

                if (line_end_index == std::string::npos)
                {
                    // No more full lines left.
                    // Confirm the line is not too long.
                    //
                    if ((data.size() - line_start_index) >
                        config::comm::so_input_line_length())
                    {
                        LOG(warning, "socket", "process_raw_incoming_data",
                            "Client " + client_source + " sent too long a line."
                            "  Disconnecting.");

                        line_start_index =  std::string::npos;
                        line_end_index = std::string::npos;

                        disconnect_socket();
                    }

                    break;
                }
                else
                {
                    // Found a full line.
                    line = incoming_text_buffer.substr(
                        line_start_index,
                        line_end_index - line_start_index);

                    // Convert into ExternalTextLine
                    //
                    line = text::convert_extended_to_utf8(line);
                    text::ExternalTextLine *external_text_line_ptr =
                        new text::ExternalTextLine(
                            text::ExternalTextConverter::to_external(line));

                    command_processor.process_input(external_text_line_ptr);
                    ++ack_lines_received_from_client;

                    line_start_index = line_end_index + 1;
                }
            }

            // Processed as many full lines as available.  Shift the used
            // characters out of the buffer.
            //
            if (line_start_index > string_end_index)
            {
                // Used all characters.
                incoming_text_buffer.clear();
            }
            else if (line_start_index and
                (line_end_index == std::string::npos))
            {
                // Used only some characters.
                incoming_text_buffer.erase(0, line_start_index);
            }
        }
    }

    // ----------------------------------------------------------------------
    comm::MessageSerialId SocketClientConnection::get_next_incoming_ser_id(void)
    {
        if (not next_input_ser_id)
        {
            next_input_ser_id = 1;
        }

        return next_input_ser_id++;
    }

    // ----------------------------------------------------------------------
    void SocketClientConnection::request_service(void)
    {
        if (not requested_service)
        {
            driver_ptr->connection_has_pending_actions(this);
            requested_service = true;
        }
    }
}
}