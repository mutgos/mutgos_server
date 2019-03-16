/*
 * websocket_WSClientConnection.cpp
 */

#include <string>
#include <string.h>
#include <stddef.h>
#include "text/text_StringConversion.h"

#include "osinterface/osinterface_OsTypes.h"

#include "comminterface/comm_ClientSession.h"

#include "utilities/mutgos_config.h"
#include "logging/log_Logger.h"
#include "dbtypes/dbtype_Id.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "utilities/json_JsonParsedObject.h"
#include "utilities/json_JsonUtilities.h"

#include "comminterface/comm_RouterSessionManager.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_MessageFactory.h"
#include "clientmessages/message_ChannelData.h"
#include "clientmessages/message_ClientTextData.h"
#include "clientmessages/message_ClientSiteList.h"
#include "clientmessages/message_ClientAuthenticationResult.h"
#include "clientmessages/message_ClientDataAcknowledge.h"
#include "clientmessages/message_ClientDataAcknowledgeReconnect.h"
#include "clientmessages/message_ClientDisconnect.h"

#include "websocket_WebsocketDriver.h"
#include "websocket_RawWSConnection.h"
#include "websocket_WSClientConnection.h"

// TODO May want to work on ChannelData, ClientTextData to have a 'no copy' mode for performance

namespace mutgos
{
namespace websocket
{
    // ----------------------------------------------------------------------
    WSClientConnection::WSClientConnection(
        WebsocketDriver *driver,
        std::shared_ptr<RawWSConnection> connection,
        const std::string &source)
      : client_window_size(0),
        client_type(comm::ClientConnection::CLIENT_TYPE_INTERACTIVE),
        client_source(source),
        client_blocked(false),
        client_connected(true),
        client_error(false),
        client_disconnect_state(WSClientConnection::DISCONNECT_STATE_NOT_REQUESTED),
        requested_service(false),
        outgoing_size(0),
        outgoing_json_node(JSON_MAKE_ARRAY_ROOT()),
        auth_attempts(0),
        client_session_ptr(0),
        driver_ptr(driver),
        raw_connection(connection)
    {
        if (not driver)
        {
            LOG(fatal, "websocket", "WSClientConnection",
                "driver pointer is null!  Crash will likely follow...");
        }

        if (not raw_connection)
        {
            LOG(fatal, "websocket", "WSClientConnection",
                "raw_connection pointer is null!  Crash will follow...");
        }

        if (not connection.get())
        {
            LOG(fatal, "websocket", "WSClientConnection",
                "Raw connection pointer is null!  Crash will likely follow...");
        }

        if (source.empty())
        {
            client_source = "UNKNOWN";
        }

        raw_connection->set_client(this);
        raw_connection->set_timer(config::comm::auth_time());

        LOG(debug, "websocket", "WSClientConnection",
            "Got a connection to " + source);
    }

    // ----------------------------------------------------------------------
    WSClientConnection::~WSClientConnection()
    {
        // This will not cause a double-delete because the Driver should
        // already know it is in the middle of deleting this.
        raw_connection->client_released();

        raw_connection->raw_disconnect();
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::stop(void)
    {
        // Unlike other disconnection requests, this one WILL notify
        // the session because it's a disconnection that it didn't request.
        //
        if (client_connected)
        {
            disconnect_socket();

            if (client_session_ptr)
            {
                client_session_ptr->client_disconnected();
            }
        }
    }

    // ----------------------------------------------------------------------
    osinterface::OsTypes::UnsignedInt WSClientConnection::
        get_client_window_size(void)
    {
        return client_window_size;
    }

    // ----------------------------------------------------------------------
    bool WSClientConnection::client_is_enhanced(void)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::ClientType WSClientConnection::get_client_type(void)
    {
        // TODO Fix so not hardcoded later.
        return comm::ClientConnection::CLIENT_TYPE_INTERACTIVE;
    }

    // ----------------------------------------------------------------------
    bool WSClientConnection::client_is_send_blocked(void)
    {
        return client_blocked;
    }

    // ----------------------------------------------------------------------
    bool WSClientConnection::client_is_connected(void)
    {
        return client_connected;
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::client_disconnect(void)
    {
        // Do not notify session of the disconnect, since this method is
        // used by them to initiate a disconnect.
        //
        if (client_disconnect_state == DISCONNECT_STATE_NOT_REQUESTED)
        {
            LOG(debug, "websocket", "client_disconnect",
                "Requested a disconnect from " + client_source + ", entity "
                + client_entity_id.to_string(true));

            client_disconnect_state = DISCONNECT_STATE_REQUESTED;
            request_service();
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdType WSClientConnection::client_get_site_id(void)
    {
        // Before Entity ID is set, the set ID portion may be set.
        // This avoids using two variables which could get out of sync.
        return client_entity_id.get_site_id();
    }

    // ----------------------------------------------------------------------
    const std::string& WSClientConnection::client_get_source(void)
    {
        return client_source;
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::client_set_entity_id(const dbtype::Id &entity_id)
    {
        client_entity_id = entity_id;
    }

    // ----------------------------------------------------------------------
    dbtype::Id WSClientConnection::client_get_entity_id(void)
    {
        return client_entity_id;
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode
    WSClientConnection::client_send_acknowledge_data(
        const comm::MessageSerialId ser_id)
    {
        message::ClientDataAcknowledge ack_message(ser_id);
        return send_message_raw(ack_message);
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode
    WSClientConnection::client_send_acknowledge_data_reconnect(
        const comm::MessageSerialId ser_id)
    {
        message::ClientDataAcknowledgeReconnect ack_message(ser_id);
        return send_message_raw(ack_message);
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode
    WSClientConnection::client_channel_status_changed(
        const comm::MessageSerialId ser_id,
        const message::ChannelStatusChange &channel_status)
    {
        message::ChannelData channel_data(0, ser_id, channel_status.clone());
        return send_message_raw(channel_data);
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode
    WSClientConnection::client_send_data(
        const comm::ChannelId channel_id,
        const comm::MessageSerialId ser_id,
        const text::ExternalTextLine &text_line)
    {
        text::ExternalTextLine cloned_line =
            text::ExternalText::clone_text_line(text_line);
        message::ChannelData channel_data(
            channel_id,
            ser_id,
            new message::ClientTextData(cloned_line));
        return send_message_raw(channel_data);
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode
    WSClientConnection::client_send_data(
        const comm::ChannelId channel_id,
        const comm::MessageSerialId ser_id,
        const message::ClientMessage &client_message)
    {
        message::ChannelData channel_data(
            channel_id,
            ser_id,
            client_message.clone());
        return send_message_raw(channel_data);
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::do_work(void)
    {
        requested_service = false;

        if (client_connected)
        {
            if (client_error)
            {
                // Let the session know of the disconnect, then
                // forcibly disconnect the client right now.
                //
                if (client_session_ptr)
                {
                    client_session_ptr->request_disconnection();
                }

                client_error = false;
                disconnect_socket();
            }
            else if ((client_disconnect_state == DISCONNECT_STATE_SENT) and
                (not raw_connection->raw_is_blocked()))
            {
                // Disconnect message sent; it is now safe to completely
                // disconnect.
                disconnect_socket();
            }
            else if (not raw_connection->raw_is_blocked())
            {
                if (client_disconnect_state == DISCONNECT_STATE_REQUESTED)
                {
                    // Append disconnect message to outgoing queue now that
                    // we're no longer blocked.
                    //
                    message::ClientDisconnect disconnect_message;
                    queue_message_to_send(disconnect_message);
                    client_disconnect_state = DISCONNECT_STATE_SENT;
                    client_blocked = true;
                }

                if (not json::array_empty(outgoing_json_node))
                {
                    // The socket can accept more data going out and there
                    // is stuff to send.  Send it.
                    //
                    const std::string outgoing_data =
                        json::write_json(outgoing_json_node);
                    raw_connection->raw_send(
                        outgoing_data.c_str(),
                        outgoing_data.size());

                    client_blocked = true;
                    json::json_clear(outgoing_json_node);
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::raw_send_complete(void)
    {
        LOG(debug, "websocket", "raw_send_complete",
            "Send complete for " + client_source + ", entity "
            + client_entity_id.to_string(true));

        // Reset outgoing data string to completely empty.  This is to prevent
        // unexpectedly large strings from hanging around after use.
        //
        if (client_connected)
        {
            if (client_disconnect_state != DISCONNECT_STATE_NOT_REQUESTED)
            {
                // Ready to send disconnect message or do final disconnection.
                request_service();
            }
            else
            {
                client_blocked = false;

                if (client_session_ptr)
                {

                    client_session_ptr->client_unblocked();
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::raw_disconnected(void)
    {
        LOG(debug, "websocket", "raw_disconnected",
            "Client disconnected.  Source " + client_source + ", entity "
            + client_entity_id.to_string(true));

        // Only process if we didn't initiate the disconnection or it was
        // disconnected due to an error condition.
        //
        if (client_connected or client_error)
        {
            client_connected = false;
            client_blocked = true;
            client_disconnect_state = DISCONNECT_STATE_NOT_REQUESTED;

            if (client_session_ptr)
            {
                client_session_ptr->client_disconnected();
            }
        }

        // At this point we will never be connected again.
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::raw_data(char *data_ptr, const size_t data_size)
    {
        // The RawWSConnection has already done length checks to ensure this
        // is not excessively long.
        //
        if (not data_ptr)
        {
            LOG(error, "websocket", "raw_data", "Got a null data pointer!");
        }
        else
        {
            LOG(debug, "websocket", "raw_data",
                "Client sent "
                + text::to_string(strlen(data_ptr))
                + " bytes.  Source " + client_source + ", entity "
                + client_entity_id.to_string(true));

            json::JsonParsedObject *json_ptr = json::parse_json(data_ptr);
            data_ptr = 0;

            if (not json_ptr)
            {
                LOG(error, "websocket", "raw_data",
                    "Client sent invalid/incomplete JSON data!  "
                    "Source " + client_source + ", entity "
                    + client_entity_id.to_string(true));

                client_error = true;
                request_service();
            }
            else
            {
                // We now have a valid, parsed JSON.  Split it out into
                // the various messages and parse each one.
                //
                if (json::is_map(json_ptr->get()))
                {
                    // Single message not sent as array.  Process directly.
                    process_message(restore_message(json_ptr));
                }
                else if (json::is_array(json_ptr->get()))
                {
                    // One or more messages sent as array.  Process one at a
                    // time.
                    //
                    const MG_UnsignedInt message_count =
                        json::array_size(json_ptr->get());
                    char *raw_json_ptr = 0;
                    size_t raw_json_size = 0;
                    json::JsonParsedObject *indexed_json_ptr = 0;

                    for (MG_UnsignedInt index = 0; index < message_count; ++index)
                    {
                        json::array_get_value(
                            json_ptr->get(),
                            index,
                            raw_json_ptr,
                            raw_json_size);

                        if (not raw_json_ptr)
                        {
                            LOG(error, "websocket", "raw_data",
                                "Empty JSON found in array, or wrong type.");
                            client_error = true;
                            request_service();
                        }
                        else
                        {
                            indexed_json_ptr = json::parse_json(raw_json_ptr);

                            if (not indexed_json_ptr)
                            {
                                LOG(error, "websocket", "raw_data",
                                    "Client sent invalid/incomplete JSON data "
                                    "in array!  Source " + client_source
                                    + ", entity "
                                    + client_entity_id.to_string(true));

                                client_error = true;
                                request_service();
                            }
                            else
                            {
                                process_message(
                                    restore_message(indexed_json_ptr));
                            }
                        }
                    }
                }
                else
                {
                    LOG(error, "websocket", "raw_data",
                        "Client sent unknown JSON data!  "
                            "Source " + client_source + ", entity "
                        + client_entity_id.to_string(true));

                    delete json_ptr;
                    client_error = true;
                    request_service();
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::raw_timer_expired(void)
    {
        // TODO In the future, this could be used for pings or other connectivity checks

        // Failed to authenticate in time.  Disconnect.
        raw_connection->raw_disconnect();
    }

    // ----------------------------------------------------------------------
    comm::ClientConnection::SendReturnCode WSClientConnection::send_message_raw(
        const message::ClientMessage &message)
    {
        comm::ClientConnection::SendReturnCode status =
            comm::ClientConnection::SEND_NOT_SUPPORTED;

        if (not client_connected)
        {
            status = comm::ClientConnection::SEND_DISCONNECTED;
        }
        else if (client_blocked)
        {
            status = comm::ClientConnection::SEND_BLOCKED;
        }
        else if (queue_message_to_send(message))
        {
            // Determine if we need to block (too big a message, too
            // many messages, etc).
            //
            if ((json::array_size(outgoing_json_node) >=
                     client_window_size) or
                (client_window_size > config::comm::ws_max_window()))
            {
                // We shouldn't take any more messages.
                status = comm::ClientConnection::SEND_OK_BLOCKED;
                client_blocked = true;
            }
            else
            {
                status = comm::ClientConnection::SEND_OK;
            }
        }

        return status;
    }

    // ----------------------------------------------------------------------
    bool WSClientConnection::queue_message_to_send(
        const mutgos::message::ClientMessage &message)
    {
        bool success = true;

        JSON_MAKE_MAP_ROOT(message_json_node);

        if (not message.save(outgoing_json_node, message_json_node))
        {
            LOG(error, "websocket", "queue_message_to_send",
                "Failed to save message of type "
                + message::client_message_type_to_string(message.get_message_type())
                + ".  Source " + client_source + ", entity "
                + client_entity_id.to_string(true));

            client_error = true;
            success = false;
        }
        else
        {
            const std::string message_json = json::write_json(
                message_json_node);

            json::array_add_value(
                message_json,
                outgoing_json_node,
                outgoing_json_node);

            client_window_size += message_json.size();
        }

        request_service();

        return success;
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::disconnect_socket(void)
    {
        if (client_connected)
        {
            LOG(debug, "websocket", "client_disconnect",
                "Disconnecting socket to " + client_source + ", entity "
                + client_entity_id.to_string(true));

            client_connected = false;
            raw_connection->raw_disconnect();
            client_blocked = true;
            client_disconnect_state = DISCONNECT_STATE_NOT_REQUESTED;
        }
    }

    // ----------------------------------------------------------------------
    message::ClientMessage *WSClientConnection::restore_message(
        json::JsonParsedObject *json_ptr)
    {
        message::ClientMessage *message_ptr = 0;

        if (not json_ptr)
        {
            LOG(error, "websocket", "restore_message",
                "json_ptr is null!");
        }
        else
        {
            const message::ClientMessageType message_type =
                message::ClientMessage::get_message_type(json_ptr->get());

            if (message_type == message::CLIENTMESSAGE_END_INVALID)
            {
                LOG(error, "websocket", "restore_message",
                    "Invalid message to restore (unknown type).");
            }
            else
            {
                LOG(debug, "websocket", "restore_message",
                    "Restoring message of type " +
                    message::client_message_type_to_string(message_type));

                message_ptr =
                    message::MessageFactory::create_message(message_type);

                if (not message_ptr)
                {
                    LOG(error, "websocket", "restore_message",
                        "Message type is not registered: " +
                        message::client_message_type_to_string(message_type));
                }
                else if (not message_ptr->restore(json_ptr->get()))
                {
                    LOG(error, "websocket", "restore_message",
                        "Failed to restore message of type: " +
                        message::client_message_type_to_string(message_type));

                    delete message_ptr;
                    message_ptr = 0;
                }
            }

            delete json_ptr;
        }

        return message_ptr;
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::process_message(
        message::ClientMessage *message_ptr)
    {
        if (message_ptr)
        {
            switch (message_ptr->get_message_type())
            {
                case message::CLIENTMESSAGE_DATA_ACKNOWLEDGE:
                {
                    if (not client_session_ptr)
                    {
                        LOG(error, "websocket", "process_message",
                            "Got data ACK message before authenticated!");
                        client_error = true;
                        request_service();
                    }
                    else
                    {
                        client_session_ptr->client_data_acknowledge(
                            static_cast<message::ClientDataAcknowledge *>(
                                message_ptr)->get_serial_id());
                    }

                    break;
                };

                case message::CLIENTMESSAGE_DATA_ACKNOWLEDGE_RECONNECT:
                {
                    if (not client_session_ptr)
                    {
                        LOG(error, "websocket", "process_message",
                            "Got data reconnect ACK message before "
                            "authenticated!");
                        client_error = true;
                        request_service();
                    }
                    else
                    {
                        client_session_ptr->client_data_acknowledge_reconnect(
                            static_cast<message::ClientDataAcknowledgeReconnect *>(
                                message_ptr)->get_serial_id());
                    }

                    break;
                };

                case message::CLIENTMESSAGE_REQUEST_SITE_LIST:
                {
                    process_request_site_list();
                    break;
                };

                case message::CLIENTMESSAGE_AUTHENTICATION_REQUEST:
                {
                    process_authentication_request(
                        *static_cast<message::AuthenticationRequest *>(message_ptr));
                    break;
                };

                case message::CLIENTMESSAGE_DISCONNECT:
                {
                    if (client_session_ptr)
                    {
                        client_session_ptr->request_disconnection();
                    }
                    else
                    {
                        client_disconnect();
                    }

                    break;
                }

                case message::CLIENTMESSAGE_CHANNEL_DATA:
                {
                    if (not client_session_ptr)
                    {
                        LOG(error, "websocket", "process_message",
                            "Got channel data message before authenticated!");
                        client_error = true;
                        request_service();
                    }
                    else
                    {
                        process_channel_data(
                            *static_cast<message::ChannelData *>(message_ptr));
                    }

                    break;
                };

                default:
                {
                    LOG(error, "websocket", "process_message",
                        "Got message we do not process!");

                    client_error = true;
                    request_service();
                    break;
                }
            }

            delete message_ptr;
            message_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::process_request_site_list(void)
    {
        // TODO later, add site name and description

        const dbtype::Id::SiteIdVector db_sites =
            dbinterface::DatabaseAccess::instance()->get_all_site_ids();
        comm::RouterSessionManager * const router_ptr =
            driver_ptr->get_router();
        message::ClientSiteList site_message;

        // Got all the site IDs, now populate the site message.
        //
        for (dbtype::Id::SiteIdVector::const_iterator site_iter =
            db_sites.begin();
            site_iter != db_sites.end();
            ++site_iter)
        {
            site_message.add_site(
                *site_iter,
                "NOT IMPLEMENTED",
                "NOT IMPLEMENTED",
                router_ptr->get_session_online_count(*site_iter));
        }

        // Send the response back,
        //
        const SendReturnCode rc = send_message_raw(site_message);

        if ((rc != SEND_OK) and (rc != SEND_OK_BLOCKED))
        {
            LOG(error, "websocket", "process_request_site_list",
                "Client is sending too many non-channel requests.");
            client_error = true;
            request_service();
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::process_authentication_request(
        message::AuthenticationRequest &request)
    {
        // TODO Need to limit how big the window size can be, to avoid malicious windows

        message::ClientAuthenticationResult result_message;

        result_message.set_negotiation_result(true);

        if (client_session_ptr)
        {
            LOG(error, "websocket", "process_authentication_request",
                "Client attempted to authenticate after being authenticated.");
            client_error = true;
            request_service();
        }
        else
        {
            comm::ClientSession *session_ptr = 0;

            client_window_size = request.get_window_size();
            client_entity_id = dbtype::Id(request.get_player_site_id(), 0);

            if (auth_attempts <= 6)
            {
                if (request.get_reconnect_flag())
                {
                    // Reconnecting.
                    session_ptr = driver_ptr->get_router()->reauthorize_client(
                        request.get_player_name(),
                        request.get_player_password(),
                        driver_ptr,
                        this);
                }
                else
                {
                    // New connection.
                    session_ptr = driver_ptr->get_router()->authorize_client(
                        request.get_player_name(),
                        request.get_player_password(),
                        driver_ptr,
                        this);
                }
            }

            if (not session_ptr)
            {
                if (auth_attempts < 500)
                {
                    ++auth_attempts;
                }
            }
            else
            {
                driver_ptr->add_reference(this);
                result_message.set_authentication_result(true);
                client_session_ptr = session_ptr;
                raw_connection->cancel_timer();
            }
        }

        // Send the response back,
        //
        const SendReturnCode rc = send_message_raw(result_message);

        if ((rc != SEND_OK) and (rc != SEND_OK_BLOCKED))
        {
            LOG(error, "websocket", "process_authentication_request",
                "Client is sending too many non-channel requests.");
            client_error = true;
            request_service();
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::process_channel_data(
        message::ChannelData &channel_data)
    {
        if (channel_data.has_message())
        {
            message::ClientMessage * const content_ptr =
                channel_data.transfer_message();

            // We have a message to process.  Determine type and hand it off
            // to the channel.
            //
            if (content_ptr->get_message_type() ==
                message::CLIENTMESSAGE_TEXT_DATA)
            {
                // Text data.
                //
                client_session_ptr->client_data(
                    channel_data.get_channel_id(),
                    channel_data.get_serial_id(),
                    static_cast<message::ClientTextData *>(content_ptr)->
                        transfer_text_line());
            }
            else
            {
                // Everything else (client enhanced data).
                //
                client_session_ptr->client_data(
                    channel_data.get_channel_id(),
                    channel_data.get_serial_id(),
                    content_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    void WSClientConnection::request_service(void)
    {
        if (not requested_service)
        {
            driver_ptr->connection_has_pending_actions(this);
            requested_service = true;
        }
    }
}
}
