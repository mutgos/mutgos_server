/*
 * comm_ClientSession.cpp
 */

#include <string>
#include <unistd.h>
#include <map>
#include <algorithm>
#include "text/text_StringConversion.h"
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_TimeStamp.h"

#include "concurrency/concurrency_WriterLockToken.h"

#include "channels/events_TextChannel.h"
#include "channels/events_ClientDataChannel.h"

#include "comminterface/comm_ClientSession.h"
#include "comminterface/comm_ClientConnection.h"
#include "comminterface/comm_RouterSessionManager.h"
#include "comminterface/comm_RouterEvent.h"
#include "comminterface/comm_ClientChannelInfo.h"
#include "comminterface/comm_SessionStats.h"

#include "clientmessages/message_ChannelStatusChange.h"
#include "clientmessages/message_ChannelStatus.h"

namespace
{
    static const mutgos::comm::ChannelId MAX_CHANNELS =
        std::numeric_limits<mutgos::comm::ChannelId>::max() - 1;
}

// TODO Error checking for window size?
// TODO Might be a bug where if client never ACKs anything we send, the queue will build up, eventually blocking forever if the client reconnects

namespace mutgos
{
namespace comm
{
    // ----------------------------------------------------------------------
    ClientSession::ClientSession(
        const SessionId id,
        RouterSessionManager *router,
        ClientConnection *client)
      : outgoing_ser_ack(0),
        incoming_ser_ack(0),
        needs_incoming_ser_ack_sent(false),
        has_requested_service(false),
        need_handle_reconnect(false),
        need_disconnect(false),
        wait_reconnect_response(false),
        client_is_blocked(false),
        client_is_connected(true),
        client_is_enhanced(client->client_is_enhanced()),
        client_type(client->get_client_type()),
        client_source(client->client_get_source()),
        client_entity_id(client->client_get_entity_id()),
        last_used_channel_id(0),
        last_used_message_ser_id(0),
        session_id(id),
        client_window_size(client->get_client_window_size()),
        client_ptr(client),
        router_ptr(router)
    {
        if (not router)
        {
            LOG(fatal, "comm", "ClientSession", "router is null!");
        }
    }

    // ----------------------------------------------------------------------
    ClientSession::~ClientSession()
    {
        Channels channels_to_remove;

        // Scope for write_lock, to avoid deadlock when calling into Channel.
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
            channels_to_remove = active_channels;

            active_channels.clear();
            blocked_channel_queues.clear();
        }

        // Unregister as a listener to every channel, which will cause
        // them to be removed during callbacks.
        //
        for (Channels::iterator channel_iter = channels_to_remove.begin();
             channel_iter != channels_to_remove.end();
             ++channel_iter)
        {
            unregister_channel(*channel_iter);
        }

        channels_to_remove.clear();

        // If we still have an active client, close everything up.
        //
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        if (client_ptr)
        {
            client_ptr->client_disconnect();
            router_ptr->release_connection(client_ptr);
            client_ptr = 0;
        }

        client_is_connected = false;
    }

    // ----------------------------------------------------------------------
    void ClientSession::set_client_connection(ClientConnection *connection_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        if (client_ptr)
        {
            client_ptr->client_disconnect();
            router_ptr->release_connection(client_ptr);
        }

        client_ptr = connection_ptr;

        if (client_ptr)
        {
            client_window_size = client_ptr->get_client_window_size();
            client_is_enhanced = client_ptr->client_is_enhanced();
            client_type = client_ptr->get_client_type();
            client_source = client_ptr->client_get_source();
        }

        client_is_blocked = false;
        client_is_connected = true;
        need_disconnect = false;
        need_handle_reconnect = true;
        wait_reconnect_response = true;
        needs_incoming_ser_ack_sent = false;

        request_service();
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_disconnected(void)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        client_is_connected = false;
    }

    // ----------------------------------------------------------------------
    SessionStats ClientSession::get_stats(void)
    {
        boost::lock_guard<boost::recursive_mutex> read_lock(client_lock);

        return SessionStats(
            client_entity_id,
            client_is_connected,
            session_established_time,
            last_activity_time,
            client_is_enhanced,
            client_source,
            client_type);
    }

    // ----------------------------------------------------------------------
    dbtype::TimeStamp ClientSession::get_session_activity_time(void)
    {
        boost::lock_guard<boost::recursive_mutex> read_lock(client_lock);

        return last_activity_time;
    }

    // ----------------------------------------------------------------------
    void ClientSession::set_activity_time_to_now(void)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        last_activity_time.set_to_now();
    }

    // ----------------------------------------------------------------------
    bool ClientSession::is_connected(void)
    {
        boost::lock_guard<boost::recursive_mutex> read_lock(client_lock);

        return client_is_connected;
    }

    // ----------------------------------------------------------------------
    void ClientSession::request_disconnection(void)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
        need_disconnect = true;
        request_service();
    }

    // ----------------------------------------------------------------------
    void ClientSession::process_pending(void)
    {
        bool process_channel_close = false;
        bool do_work = true;

        // Scope for lock
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

            has_requested_service = false;

            if (need_disconnect)
            {
                client_ptr->client_disconnect();
                do_work = false;
                client_is_connected = false;
            }
            else
            {
                process_channel_close = not pending_channels_delete.empty();
            }
        }

        if (do_work)
        {
            if (process_channel_close)
            {
                process_pending_channel_deletes();
            }

            process_pending_unblocked_channels();

            // Scope for lock
            {
                boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

                // TODO Later, fix this to allow switching between different ClientConnection types, by sending current open channels up front, etc.

                if (need_handle_reconnect)
                {
                    // Handle reconnect - send out ACK to client with last
                    // message we got from them, then wait for their response
                    // with the last message they got from us before sending
                    // anything else.
                    //
                    client_ptr->client_send_acknowledge_data_reconnect(
                        incoming_ser_ack);

                    needs_incoming_ser_ack_sent = false;
                    need_handle_reconnect = false;
                    wait_reconnect_response = true;
                }
                else if ((not wait_reconnect_response) and (not client_is_blocked))
                {
                    // Normal situation (not reconnecting).
                    ClientConnection::SendReturnCode status =
                        ClientConnection::SEND_OK;

                    // If an ACK needs to be sent out, do it.
                    //
                    if (needs_incoming_ser_ack_sent)
                    {
                        status = client_ptr->client_send_acknowledge_data(
                            incoming_ser_ack);

                        needs_incoming_ser_ack_sent = false;
                        process_send_return_code(status);
                    }

                    // Then, if still not blocked, send pending messages until
                    // we block, nothing is left, or we hit the window limit.
                    //
                    bool sent_success = false;

                    while ((not client_is_blocked) and
                        (not outgoing_events.empty()) and
                        (sent_events.size() < client_window_size))
                    {
                        RouterEvent &event = outgoing_events.front();

                        switch (event.get_event_type())
                        {
                            case RouterEvent::EVENT_CHANNEL_STATUS_DATA:
                            {
                                sent_success = process_send_return_code(
                                    client_ptr->client_channel_status_changed(
                                        event.get_serial_id(),
                                        *event.get_channel_status_data()));
                                break;
                            }

                            case RouterEvent::EVENT_TEXT_DATA:
                            {
                                sent_success = process_send_return_code(
                                    client_ptr->client_send_data(
                                        event.get_channel_id(),
                                        event.get_serial_id(),
                                        *event.get_text_data()));
                                break;
                            }

                            case RouterEvent::EVENT_ENHANCED_DATA:
                            {
                                sent_success = process_send_return_code(
                                    client_ptr->client_send_data(
                                        event.get_channel_id(),
                                        event.get_serial_id(),
                                        *event.get_enhanced_data()));
                                break;
                            }

                            default:
                            {
                                LOG(error, "comm", "process_pending",
                                    "Unknown event type: "
                                    + text::to_string(
                                        event.get_event_type()));

                                need_disconnect = true;
                                sent_success = false;
                                request_service();

                                break;
                            }
                        }

                        // If we were able to send it, put message in send
                        // queue to wait for ACK
                        //
                        if (sent_success)
                        {
                            sent_events.insert(sent_events.end(), RouterEvent());
                            sent_events.back().transfer(event);
                            outgoing_events.pop_front();
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_data_acknowledge(const MessageSerialId ser_id)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        if (not ser_id)
        {
            // This is an error and could indicate a badly coded or malicious
            // client.
            //
            LOG(error, "comm", "client_data_acknowledge",
                "Client sent invalid ser_id! Source: "
                  + client_ptr->client_get_source());

            need_disconnect = true;
            request_service();
        }
        else
        {
            // Simply locate the ID in the queue of sent messages.  Everything
            // prior to the message is assumed to also have been received and
            // can be safely deleted.
            //

            EventQueue::iterator found_message_iter = sent_events.end();

            for (EventQueue::iterator event_iter = sent_events.begin();
                event_iter != sent_events.end();
                ++event_iter)
            {
                if (event_iter->get_serial_id() == ser_id)
                {
                    // Found event
                    found_message_iter = event_iter;
                    outgoing_ser_ack = ser_id;
                    break;
                }
            }

            if (found_message_iter == sent_events.end())
            {
                // Did not find the event.  This is an error and could indicate
                // a badly coded or malicious client.
                //
                LOG(error, "comm", "client_data_acknowledge",
                    "Client sent ser_id that did not match any event! Source: "
                    + client_ptr->client_get_source());

                need_disconnect = true;
                request_service();
            }
            else
            {
                // Jump it one past to make sure all messages acknowledged
                // are deleted.
                ++found_message_iter;
                sent_events.erase(sent_events.begin(), found_message_iter);

                // Now that there's possibly room in the window to send more
                // messages, try and do it the next time around.
                //
                if (not outgoing_events.empty())
                {
                    request_service();
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_data_acknowledge_reconnect(
        const MessageSerialId ser_id)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        if (not wait_reconnect_response)
        {
            // This is not an expected response.  Disconnect.
            //
            LOG(error, "comm", "client_data_acknowledge_reconnect",
                "Client sent reconnect ACK but is not reconnecting!  Source: "
                + client_ptr->client_get_source());
            need_disconnect = true;
        }
        else
        {
            wait_reconnect_response = false;

            // We allow a 0 ser_id here to indicate "I don't maintain state
            // between reconnects, so send me everything you currently have".
            if (not ser_id)
            {
                outgoing_ser_ack = 0;
            }
            else if (ser_id != outgoing_ser_ack)
            {
                // Client was not up to date.  Call the usual ACK routine to
                // clear out old data before moving the remainder back into
                // the outgoing queue.
                //
                client_data_acknowledge(ser_id);
            }
        }

        if (not need_disconnect)
        {
            // No errors processing the ACK.  Move events back into outgoing
            // queue and request to send them.
            //
            while (not sent_events.empty())
            {
                outgoing_events.insert(outgoing_events.begin(), RouterEvent());

                outgoing_events.front().transfer(sent_events.back());
                sent_events.pop_back();
            }
        }

        request_service();
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_unblocked()
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        client_is_blocked = false;

        if (not outgoing_events.empty())
        {
            // We have more data to go out; let the router know.
            request_service();
        }
    }

    // ----------------------------------------------------------------------
    ClientChannelInfoVector ClientSession::get_current_channel_info(void)
    {
        ClientChannelInfoVector channels;

        boost::lock_guard<boost::recursive_mutex> read_lock(client_lock);

        for (Channels::iterator channel_iter = active_channels.begin();
            channel_iter != active_channels.end();
            ++channel_iter)
        {
            if (channel_iter->valid())
            {
                events::Channel * const channel_ptr = channel_iter->channel_ptr;

                channels.push_back(ClientChannelInfo(
                    channel_iter->id,
                    channel_ptr->get_channel_name(),
                    channel_ptr->get_channel_type(),
                    channel_ptr->get_channel_subtype(),
                    channel_iter->out,
                    channel_iter->blocked));
            }
        }

        return channels;
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_data(
        const ChannelId channel_id,
        const MessageSerialId ser_id,
        text::ExternalTextLine *text_line_ptr)
    {
        // Find the channel and determine if it's blocked.  If blocked, queue
        // it up (unless we've exceeded the window size, which means the client
        // is ignoring Channel status and may be malicious or noncompliant),
        // otherwise try and send the data on the channel.
        // Note that due to deadlock issues, the mutexes have to be used very
        // carefully and in a specific order.
        //
        ChannelInfo *channel_info_ptr = 0;

        // Scope for write_lock to avoid future deadlock
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
            // This can only be cleaned up in process_pending(), so we're safe.
            channel_info_ptr = &get_channel_info(channel_id);
        }

        // Now that we have the channel data, see if the channel is valid,
        // blocked, etc.
        //

        if (not text_line_ptr)
        {
            LOG(error, "comm", "client_data (text)",
                "Channel ID " + text::to_string(channel_id)
                + " got null pointer for text data for message "
                + text::to_string(ser_id));
        }
        else if (not channel_info_ptr->valid())
        {
            LOG(warning, "comm", "client_data (text)",
                "Channel ID " + text::to_string(channel_id)
                + " not found.  Ignoring message "
                + text::to_string(ser_id));
        }
        else if (channel_info_ptr->out)
        {
            LOG(error, "comm", "client_data (text)",
                "Channel ID " + text::to_string(channel_id)
                + " is going the wrong direction.  Client error.");

            // Scope for write_lock to avoid future deadlock
            {
                boost::lock_guard<boost::recursive_mutex> write_lock(
                    client_lock);
                need_disconnect = true;
                request_service();
            }
        }
        else
        {
            LOG(debug, "comm", "client_data (text)",
                "Sending message " + text::to_string(ser_id)
                  + " on Channel ID "
                  + text::to_string(channel_id) + "...");

            // First lock the channel, then our lock to avoid deadlocks.
            //
            events::Channel * const channel_ptr = channel_info_ptr->channel_ptr;

            concurrency::WriterLockToken token(*channel_ptr);
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

            last_activity_time.set_to_now();

            incoming_ser_ack = ser_id;
            needs_incoming_ser_ack_sent = true;

            if (not client_is_blocked)
            {
                request_service();
            }

            if (channel_info_ptr->blocked)
            {
                // Already know it's blocked.  Just queue it up.  No need to
                // check channel itself as it will call us back when it's
                // unblocked.
                //
                LOG(debug, "comm", "client_data (text)",
                    "Message " + text::to_string(ser_id)
                    + " blocked (known) on Channel ID "
                    + text::to_string(channel_id));

                EventQueue &blocked_events = get_blocked_queue(channel_id);

                RouterEvent event(text_line_ptr, ser_id, 0);
                blocked_events.push_back(RouterEvent());
                blocked_events.back().transfer(event);
                text_line_ptr = 0;

                if ((blocked_events.size() + 1) > client_window_size)
                {
                    LOG(error, "comm", "client_data (text)",
                        "Channel ID "
                        + text::to_string(channel_id)
                        + " blocked but client still sending.");

                    need_disconnect = true;
                    request_service();
                }
            }
            else if (channel_info_ptr->closed)
            {
                LOG(warning, "comm", "client_data (text)",
                    "Channel ID " + text::to_string(channel_id)
                    + " closed.  Ignoring message "
                    + text::to_string(ser_id));
            }
            else
            {
                // Try and send.  If blocked, change our status and queue it.
                // If closed, change our status and ignore message.
                //

                if (channel_ptr->get_channel_type() !=
                    events::Channel::CHANNEL_TYPE_TEXT)
                {
                    // Wrong type of channel.
                    //
                    LOG(error, "comm", "client_data (text)",
                        "Channel ID "
                        + text::to_string(channel_id)
                        + " tried to send non-text data!");

                    need_disconnect = true;
                    request_service();
                }
                else if (not static_cast<events::TextChannel *>(channel_ptr)->
                    send_item(*text_line_ptr))
                {
                    // Failed to send, determine why and queue up if blocked.
                    //
                    if (channel_ptr->channel_is_closed())
                    {
                        // Channel is closed.  Ignore.  Eventually we'll
                        // get a callback about it and will clean it up there.
                        //
                        LOG(debug, "comm", "client_data (text)",
                            "Channel ID "
                            + text::to_string(channel_id)
                            + " closed (not yet notified us).  Ignoring message.");
                    }
                    else if (channel_ptr->channel_is_blocked())
                    {
                        LOG(debug, "comm", "client_data (text)",
                            "Message " + text::to_string(ser_id)
                            + " blocked (new) on Channel ID "
                            + text::to_string(channel_id));

                        channel_info_ptr->blocked = true;

                        EventQueue &blocked_events =
                            get_blocked_queue(channel_id);

                        RouterEvent event(text_line_ptr, ser_id, 0);
                        blocked_events.push_back(RouterEvent());
                        blocked_events.back().transfer(event);
                        text_line_ptr = 0;

                        if ((blocked_events.size() + 1) > client_window_size)
                        {
                            LOG(error, "comm", "client_data (text)",
                                "Channel ID "
                                + text::to_string(channel_id)
                                + " blocked but client still sending.");

                            need_disconnect = true;
                            request_service();
                        }
                    }
                    else
                    {
                        LOG(error, "comm", "client_data (text)",
                            "Channel ID "
                            + text::to_string(channel_id)
                            + ", name " + channel_ptr->get_channel_name()
                            + " did not accept message for unknown reason!");
                    }
                }
            }
        }

        if (text_line_ptr)
        {
            text::ExternalText::clear_text_line(*text_line_ptr);
            delete text_line_ptr;
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_data(
        const ChannelId channel_id,
        const MessageSerialId ser_id,
        message::ClientMessage *client_message_ptr)
    {
        // Find the channel and determine if it's blocked.  If blocked, queue
        // it up (unless we've exceeded the window size, which means the client
        // is ignoring Channel status and may be malicious or noncompliant),
        // otherwise try and send the data on the channel.
        // Note that due to deadlock issues, the mutexes have to be used very
        // carefully and in a specific order.
        //
        ChannelInfo *channel_info_ptr = 0;

        // Scope for write_lock to avoid future deadlock
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
            // This can only be cleaned up in process_pending(), so we're safe.
            channel_info_ptr = &get_channel_info(channel_id);
        }

        // Now that we have the channel data, see if the channel is valid,
        // blocked, etc.
        //

        if (not client_message_ptr)
        {
            LOG(error, "comm", "client_data (ClientMessage)",
                "Channel ID " + text::to_string(channel_id)
                + " got null pointer for ClientMessage data for message "
                + text::to_string(ser_id));
        }
        else if (not channel_info_ptr->valid())
        {
            LOG(warning, "comm", "client_data (ClientMessage)",
                "Channel ID " + text::to_string(channel_id)
                + " not found.  Ignoring message "
                + text::to_string(ser_id));
        }
        else if (channel_info_ptr->out)
        {
            LOG(error, "comm", "client_data (ClientMessage)",
                "Channel ID " + text::to_string(channel_id)
                + " is going the wrong direction.  Client error.");

            // Scope for write_lock to avoid future deadlock
            {
                boost::lock_guard<boost::recursive_mutex> write_lock(
                    client_lock);
                need_disconnect = true;
                request_service();
            }
        }
        else
        {
            LOG(debug, "comm", "client_data (ClientMessage)",
                "Sending message " + text::to_string(ser_id)
                + " on Channel ID "
                + text::to_string(channel_id) + "...");

            // First lock the channel, then our lock to avoid deadlocks.
            //
            events::Channel * const channel_ptr = channel_info_ptr->channel_ptr;

            concurrency::WriterLockToken token(*channel_ptr);
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

            last_activity_time.set_to_now();

            incoming_ser_ack = ser_id;
            needs_incoming_ser_ack_sent = true;

            if (not client_is_blocked)
            {
                request_service();
            }

            if (channel_info_ptr->blocked)
            {
                // Already know it's blocked.  Just queue it up.  No need to
                // check channel itself as it will call us back when it's
                // unblocked.
                //
                LOG(debug, "comm", "client_data (ClientMessage)",
                    "Message " + text::to_string(ser_id)
                    + " blocked (known) on Channel ID "
                    + text::to_string(channel_id));

                EventQueue &blocked_events = get_blocked_queue(channel_id);

                RouterEvent event(client_message_ptr, ser_id, 0);
                blocked_events.push_back(RouterEvent());
                blocked_events.back().transfer(event);
                client_message_ptr = 0;

                if ((blocked_events.size() + 1) > client_window_size)
                {
                    LOG(error, "comm", "client_data (ClientMessage)",
                        "Channel ID "
                        + text::to_string(channel_id)
                        + " blocked but client still sending.");

                    need_disconnect = true;
                }
            }
            else if (channel_info_ptr->closed)
            {
                LOG(warning, "comm", "client_data (ClientMessage)",
                    "Channel ID " + text::to_string(channel_id)
                    + " closed.  Ignoring message "
                    + text::to_string(ser_id));
            }
            else
            {
                // Try and send.  If blocked, change our status and queue it.
                // If closed, change our status and ignore message.
                //
                if (channel_ptr->get_channel_type() !=
                    events::Channel::CHANNEL_TYPE_CLIENT_DATA)
                {
                    // Wrong type of channel.
                    //
                    LOG(error, "comm", "client_data (ClientMessage)",
                        "Channel ID "
                        + text::to_string(channel_id)
                        + " tried to send non-client data!");

                    need_disconnect = true;
                    request_service();
                }
                else if (static_cast<events::ClientDataChannel *>(channel_ptr)->
                        send_item(client_message_ptr))
                {
                    // Success.  Don't delete the mesage here since we no
                    // longer own it.
                    client_message_ptr = 0;
                }
                else
                {
                    // Failed to send, determine why and queue up if blocked.
                    //
                    if (channel_ptr->channel_is_closed())
                    {
                        // Channel is closed.  Ignore.  Eventually we'll
                        // get a callback about it and will clean it up there.
                        //
                        LOG(debug, "comm", "client_data (ClientMessage)",
                            "Channel ID "
                            + text::to_string(channel_id)
                            + " closed (not yet notified us).  Ignoring message.");
                    }
                    else if (channel_ptr->channel_is_blocked())
                    {
                        LOG(debug, "comm", "client_data (ClientMessage)",
                            "Message " + text::to_string(ser_id)
                            + " blocked (new) on Channel ID "
                            + text::to_string(channel_id));

                        channel_info_ptr->blocked = true;

                        EventQueue &blocked_events =
                            get_blocked_queue(channel_id);

                        RouterEvent event(client_message_ptr, ser_id, 0);
                        blocked_events.push_back(RouterEvent());
                        blocked_events.back().transfer(event);
                        client_message_ptr = 0;

                        if ((blocked_events.size() + 1) > client_window_size)
                        {
                            LOG(error, "comm", "client_data (ClientMessage)",
                                "Channel ID "
                                + text::to_string(channel_id)
                                + " blocked but client still sending.");

                            need_disconnect = true;
                        }
                    }
                    else
                    {
                        LOG(error, "comm", "client_data (ClientMessage)",
                            "Channel ID "
                            + text::to_string(channel_id)
                            + ", name " + channel_ptr->get_channel_name()
                            + " did not accept message for unknown reason!");
                    }
                }
            }
        }

        delete client_message_ptr;
    }

    // ----------------------------------------------------------------------
    ChannelId ClientSession::channel_added(
        events::Channel *const channel_ptr,
        const bool to_client)
    {
        ChannelId channel_id = 0;

        if (channel_ptr)
        {
            concurrency::WriterLockToken token(*channel_ptr);
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

            channel_id = get_next_channel_id();

            active_channels.push_back(
                ChannelInfo(channel_id, channel_ptr, to_client));

            ChannelInfo &channel_info = active_channels.back();

            // Let client know of the new channel
            //
            RouterEvent event(make_channel_status_change(
                channel_info,
                message::CHANNEL_STATUS_open),
                get_next_message_id());

            outgoing_events.push_back(RouterEvent());
            outgoing_events.back().transfer(event);

            if (not client_is_blocked)
            {
                request_service();
            }

            // Determine if channel is currently blocked.
            //
            if (channel_ptr->channel_is_blocked())
            {
                channel_info.blocked = true;

                RouterEvent block_event(make_channel_status_change(
                    channel_info,
                    message::CHANNEL_STATUS_block),
                    get_next_message_id());

                outgoing_events.push_back(RouterEvent());
                outgoing_events.back().transfer(block_event);
            }

            // Ensures channel is not destructed until we are 100% done with it.
            channel_ptr->channel_register_pointer_holder(this);

            // Register ourselves as a listener depending on the type of channel
            //
            channel_ptr->channel_register_control_listener(this);

            if (to_client)
            {
                // Since we have to listen for data as well, register
                // as the specific listener depending on the type.
                //
                switch (channel_ptr->get_channel_type())
                {
                    case events::Channel::CHANNEL_TYPE_TEXT:
                    {
                        (static_cast<events::TextChannel *>(channel_ptr))->
                            register_receiver_callback(this);
                        break;
                    }

                    case events::Channel::CHANNEL_TYPE_CLIENT_DATA:
                    {
                        (static_cast<events::ClientDataChannel *>(channel_ptr))->
                            register_receiver_callback(this);
                        break;
                    }

                    default:
                    {
                        LOG(fatal, "comm", "channel_added",
                            "Unknown Channel type: "
                            + text::to_string(
                                channel_ptr->get_channel_type()));
                    }
                }
            }
        }

        return channel_id;
    }

    // ----------------------------------------------------------------------
    void ClientSession::channel_flow_blocked(
        const std::string &channel_name,
        events::Channel *const channel_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (channel_info.valid() and (not channel_info.out))
        {
            // Only care about channels taking data FROM client right now.
            //
            remove_pending_unblocked_channel(channel_info.id);

            if (not channel_info.blocked)
            {
                // Update state and add a router event.
                //
                channel_info.blocked = true;

                // This two step process is used to avoid copying the event
                // contents.
                //
                RouterEvent event(make_channel_status_change(
                    channel_info,
                    message::CHANNEL_STATUS_block),
                    get_next_message_id());

                outgoing_events.push_back(RouterEvent());
                outgoing_events.back().transfer(event);

                if (not client_is_blocked)
                {
                    request_service();
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::channel_flow_open(
        const std::string &channel_name,
        events::Channel *const channel_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (not channel_info.valid())
        {
            LOG(error, "comm", "channel_flow_open",
                "Channel " + channel_name + " not found!");
        }
        else if (not channel_info.out)
        {
            add_pending_unblocked_channel(channel_info.id);

            if (not client_is_blocked)
            {
                request_service();
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::channel_flow_closed(
        const std::string &channel_name,
        events::Channel *const channel_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (not channel_info.valid())
        {
            LOG(error, "comm", "channel_flow_closed",
                "Channel " + channel_name + " not found!");
        }
        else
        {
            // Put an event on the queue about it, then queue up the channel
            // to be safely removed from our data structures.
            // This two step process is used to avoid copying the event
            // contents.
            //
            RouterEvent event(make_channel_status_change(
                  channel_info,
                  message::CHANNEL_STATUS_close),
                get_next_message_id());

            outgoing_events.push_back(RouterEvent());
            outgoing_events.back().transfer(event);

            pending_channels_delete.push_back(channel_info.id);
            channel_info.closed = true;

            if (not client_is_blocked)
            {
                request_service();
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::channel_destructed(
        const std::string &channel_name,
        events::Channel *const channel_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        // Normally the channel should be deleted when closed.
        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (channel_info.valid())
        {
            // Should have already been deleted.
            LOG(error, "comm", "channel_destructed",
                "Channel " + channel_name + " destructed but never closed.");
            channel_flow_closed(channel_name, channel_ptr);
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::client_channel_data(
        const std::string &channel_name,
        events::ClientDataChannel *channel_ptr,
        message::ClientMessage *client_message_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (not channel_info.valid())
        {
            LOG(error, "comm", "client_channel_data",
                "Unrecognized channel " + channel_name + " sent data to us.");
        }
        else
        {
            // Put client data on the queue.
            // This two step process is used to avoid copying the event
            // contents.
            //
            RouterEvent event(
                client_message_ptr,
                get_next_message_id(),
                channel_info.id);
            outgoing_events.push_back(RouterEvent());
            outgoing_events.back().transfer(event);

            if (not client_is_blocked)
            {
                request_service();
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::text_channel_data(
        const std::string &channel_name,
        events::TextChannel *channel_ptr,
        text::ExternalTextLine &text_line)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        ChannelInfo &channel_info = get_channel_info(channel_ptr);

        if (not channel_info.valid())
        {
            LOG(error, "comm", "text_channel_data",
                "Unrecognized channel " + channel_name + " sent data to us.");
        }
        else
        {
            // Put text data on the queue.
            // This two step process is used to avoid copying the event
            // contents.
            //
            RouterEvent event(
                new text::ExternalTextLine(text_line),
                get_next_message_id(),
                channel_info.id);
            outgoing_events.push_back(RouterEvent());
            outgoing_events.back().transfer(event);

            text_line.clear();

            if (not client_is_blocked)
            {
                request_service();
            }
        }
    }

    // ----------------------------------------------------------------------
    bool ClientSession::process_send_return_code(
        const ClientConnection::SendReturnCode code)
    {
        bool sent = false;

        // Update session status booleans based on return code
        //
        switch (code)
        {
            case ClientConnection::SEND_OK:
            {
                sent = true;
                break;
            }

            case ClientConnection::SEND_OK_BLOCKED:
            {
                sent = true;
                client_is_blocked = true;
                break;
            }

            case ClientConnection::SEND_BLOCKED:
            {
                client_is_blocked = true;
                break;
            }

            case ClientConnection::SEND_DISCONNECTED:
            {
                client_is_blocked = true;
                client_is_connected = false;
                break;
            }

            case ClientConnection::SEND_NOT_SUPPORTED:
            {
                client_is_blocked = true;
                need_disconnect = true;
                request_service();
                break;
            }

            default:
            {
                // No need to do anything with the rest.
                break;
            }
        }

        return sent;
    }

    // ----------------------------------------------------------------------
    events::Channel *ClientSession::get_channel_by_id(
        const ChannelId channel_id)
    {
        return get_channel_info(channel_id).channel_ptr;
    }

    // ----------------------------------------------------------------------
    ChannelId ClientSession::get_channel_id(const events::Channel *channel_ptr)
    {
        return get_channel_info(channel_ptr).id;
    }

    // ----------------------------------------------------------------------
    ClientSession::ChannelInfo& ClientSession::get_channel_info(
        const events::Channel *channel_ptr)
    {
        static ChannelInfo invalid_channel_info;
        ChannelInfo *result_ptr = &invalid_channel_info;

        for (Channels::iterator iter = active_channels.begin();
             iter != active_channels.end();
             ++iter)
        {
            if (iter->channel_ptr == channel_ptr)
            {
                result_ptr = &(*iter);
                break;
            }
        }

        return *result_ptr;
    }

    // ----------------------------------------------------------------------
    ClientSession::ChannelInfo& ClientSession::get_channel_info(
        const ChannelId channel_id)
    {
        static ChannelInfo invalid_channel_info;
        ChannelInfo *result_ptr = &invalid_channel_info;

        for (Channels::iterator iter = active_channels.begin();
             iter != active_channels.end();
             ++iter)
        {
            if (iter->id == channel_id)
            {
                result_ptr = &(*iter);
                break;
            }
        }

        return *result_ptr;
    }

    // ----------------------------------------------------------------------
    message::ChannelStatusChange *ClientSession::make_channel_status_change(
        ChannelInfo &channel_info,
        const message::ChannelStatus channel_status)
    {
        return new message::ChannelStatusChange(
            channel_status,
            channel_info.out,
            channel_info.id,
            channel_info.channel_ptr->get_channel_name(),
            channel_info.channel_ptr->get_channel_type(),
            channel_info.channel_ptr->get_channel_subtype());
    }

    // ----------------------------------------------------------------------
    // This method illustrates the design may not be optimal, due to the
    // excessive lock/unlock cycles to get around how Channels work.
    //
    void ClientSession::process_pending_unblocked_channels(void)
    {
        ChannelIds unblocked_channels;
        ChannelIds invalid_channels;
        ChannelId channel_id = 0;
        events::Channel *channel_ptr = 0;
        ChannelInfo *channel_info_ptr = 0;
        bool send_success = false;

        // Scope for write_lock
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
            unblocked_channels = pending_channels_unblock;
            pending_channels_unblock.clear();
        }

        // For each Channel that is now unblocked...
        //
        for (ChannelIds::const_iterator unblock_iter =
                unblocked_channels.begin();
            unblock_iter != unblocked_channels.end();
            ++unblock_iter)
        {
            channel_id = *unblock_iter;

            // Scope for write_lock
            {
                boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
                channel_info_ptr = &get_channel_info(channel_id);
            }

            if (not channel_info_ptr->valid())
            {
                // Already deleted??  Ignore.
                invalid_channels.push_back(channel_id);
            }
            else
            {
                channel_ptr = channel_info_ptr->channel_ptr;

                // Must lock in this order to prevent deadlock
                concurrency::WriterLockToken token(*channel_ptr);
                boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

                // Only try and process if not closed.  Closed channels will
                // be cleaned up later.
                //
                if (not channel_ptr->channel_is_closed())
                {
                    // Try and send everything in the queue.
                    //
                    EventQueue &queue = get_blocked_queue(channel_id);
                    send_success = false;

                    // For each message in an unblocked Channel...
                    //
                    while (not queue.empty())
                    {
                        RouterEvent &event = queue.front();

                        switch (event.get_event_type())
                        {
                            case RouterEvent::EVENT_TEXT_DATA:
                            {
                                // We know the type, no need to check.
                                send_success =
                                    reinterpret_cast<events::TextChannel *>(
                                      channel_ptr)->send_item(
                                        *event.get_text_data());
                                break;
                            }

                            case RouterEvent::EVENT_ENHANCED_DATA:
                            {
                                // We know the type, no need to check.
                                send_success =
                                    reinterpret_cast<events::ClientDataChannel *>(
                                        channel_ptr)->send_item(
                                          event.get_enhanced_data());

                                if (send_success)
                                {
                                    // Pointer is owned by other end of channel
                                    event.transfer();
                                }

                                break;
                            }

                            default:
                            {
                                // This is a fatal condition and should only
                                // happen during development.
                                LOG(fatal, "comm",
                                    "process_pending_unblocked_channels",
                                    "Unrecognized event type!");

                                send_success = false;
                                invalid_channels.push_back(channel_id);

                                break;
                            }
                        }

                        if (send_success)
                        {
                            queue.pop_front();
                            // event is now invalid.
                        }
                        else if (channel_ptr->channel_is_blocked())
                        {
                            // Need to wait for channel to become unblocked again.
                            break;
                        }
                        else if (channel_ptr->channel_is_closed())
                        {
                            // Closed channel will be cleaned up later.
                            break;
                        }
                        else
                        {
                            // Something else is wrong.
                            LOG(error, "comm",
                                "process_pending_unblocked_channels",
                                "Channel "
                                + text::to_string(*unblock_iter)
                                + " in unknown state!");

                            invalid_channels.push_back(channel_id);
                            break;
                        }
                    }

                    // Update blocked status if everything really did go
                    // through.
                    //
                    if (queue.empty())
                    {
                        channel_info_ptr->blocked =
                            channel_ptr->channel_is_blocked();

                        if (not channel_info_ptr->blocked)
                        {
                            // Let client know channel did finally unblock
                            // This two step process is used to avoid copying
                            // the event contents.
                            //
                            RouterEvent event(make_channel_status_change(
                                *channel_info_ptr,
                                message::CHANNEL_STATUS_unblock),
                                get_next_message_id());

                            outgoing_events.push_back(RouterEvent());
                            outgoing_events.back().transfer(event);

                            // Don't need to request service because this is
                            // called within process_pending().
                        }
                    }
                }
            }
        }

        boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);

        // If any invalid channels were found, just remove their queues.
        // The remaining cleanup is handled in other parts of this class.
        //
        if (not invalid_channels.empty())
        {
            for (ChannelIds::const_iterator invalid_iter =
                    invalid_channels.begin();
                invalid_iter != invalid_channels.end();
                ++invalid_iter)
            {
                remove_blocked_queue(*invalid_iter);
            }
        }
    }

    // ----------------------------------------------------------------------
    void ClientSession::process_pending_channel_deletes(void)
    {
        Channels channels_to_delete;

        // Scope for write_lock.  Delete the channels from the data structures.
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(client_lock);
            ChannelInfo temp_channel;

            for (ChannelIds::const_iterator channel_iter =
                    pending_channels_delete.begin();
                channel_iter != pending_channels_delete.end();
                ++channel_iter)
            {
                temp_channel = delete_channel(*channel_iter);

                if (temp_channel.valid())
                {
                    channels_to_delete.push_back(temp_channel);
                }
            }

            pending_channels_delete.clear();
        }

        // Now unregister.  If somehow they call us back in the meantime,
        // other methods will be unable to look the channel up and safely fail.
        //
        for (Channels::iterator delete_iter = channels_to_delete.begin();
            delete_iter != channels_to_delete.end();
            ++delete_iter)
        {
            unregister_channel(*delete_iter);
        }
    }

    // ----------------------------------------------------------------------
    ClientSession::ChannelInfo ClientSession::delete_channel(
        const ChannelId channel_id)
    {
        ChannelInfo channel_info;

        if (channel_id)
        {
            remove_pending_unblocked_channel(channel_id);

            for (Channels::iterator iter = active_channels.begin();
                 iter != active_channels.end();
                 ++iter)
            {
                if (iter->id == channel_id)
                {
                    remove_blocked_queue(channel_id);

                    // Found it.  See if this is the last one or if we need
                    // to swap this entry for the last entry manually.
                    //
                    channel_info = *iter;

                    if (active_channels.back().channel_ptr == iter->channel_ptr)
                    {
                        // We're at the end.  Just erase it.
                        active_channels.erase(iter);
                    }
                    else
                    {
                        // We're not at the end.  Put the last element
                        // into this one's position, to avoid shifting
                        // stuff around.
                        (*iter) = active_channels.back();
                        active_channels.pop_back();
                    }

                    break;
                }
            }
        }

        return channel_info;
    }

    // ----------------------------------------------------------------------
    void ClientSession::unregister_channel(ChannelInfo &channel_info)
    {
        if (not channel_info.valid())
        {
            LOG(error, "comm", "unregister_channel",
                "Channel is not valid!");
        }
        else
        {
            events::Channel * const channel_ptr = channel_info.channel_ptr;

            // Keeps it from calling us back due to unregistering as a receiver.
            channel_info.closed = true;
            channel_ptr->channel_unregister_control_listener(this);
            channel_ptr->close_channel();

            // Unregister the actual listener if we send data from this channel
            // to the client.
            //
            if (channel_info.out)
            {
                switch (channel_ptr->get_channel_type())
                {
                    case events::Channel::CHANNEL_TYPE_TEXT:
                    {
                        (static_cast<events::TextChannel *>(channel_ptr))->
                            unregister_receiver_callback(this);
                        break;
                    }

                    case events::Channel::CHANNEL_TYPE_CLIENT_DATA:
                    {
                        (static_cast<events::ClientDataChannel *>(channel_ptr))->
                            unregister_receiver_callback(this);
                        break;
                    }

                    default:
                    {
                        LOG(fatal, "comm", "unregister_channel",
                            "Unknown Channel type: "
                            + text::to_string(
                                channel_ptr->get_channel_type()));
                    }
                }
            }

            channel_ptr->channel_unregister_pointer_holder(this);

            // channel_ptr may be invalid at this point
        }
    }

    // ----------------------------------------------------------------------
    ClientSession::EventQueue& ClientSession::get_blocked_queue(
        const ChannelId channel_id)
    {
        EventQueue *queue = 0;

        // Try and find an existing queue first
        //
        for (BlockedChannelQueues::iterator find_iter =
            blocked_channel_queues.begin();
             find_iter != blocked_channel_queues.end();
             ++find_iter)
        {
            if (find_iter->first == channel_id)
            {
                queue = &find_iter->second;
                break;
            }
        }

        if (not queue)
        {
            // Need to make a new one.
            //
            blocked_channel_queues.push_back(
                std::make_pair(channel_id, EventQueue()));

            queue = &blocked_channel_queues.back().second;
        }

        return *queue;
    }

    // ----------------------------------------------------------------------
    void ClientSession::remove_blocked_queue(const ChannelId channel_id)
    {
        // TODO While rare, it is in theory possible for the list of blocked queues to get really big of nothing but unblocked queues plus one blocked queue at the end.  Consider a more solid approach later.  It would require a very specialized attack to utilize this to run out of memory.

        bool found = false;

        // Find and clear out everything in the queue.
        //
        for (BlockedChannelQueues::iterator find_iter =
              blocked_channel_queues.begin();
            find_iter != blocked_channel_queues.end();
            ++find_iter)
        {
            if (find_iter->first == channel_id)
            {
                // Found it.  Clear out the blocked queue.
                find_iter->second.clear();
                found = true;
                break;
            }
        }

        // Determine if there are any queues at the end we can remove
        //
        if (found)
        {
            BlockedChannelQueues::iterator erase_iter =
                blocked_channel_queues.end();
            --erase_iter;

            while (erase_iter->second.empty())
            {
                --erase_iter;
            }

            ++erase_iter;

            if (erase_iter != blocked_channel_queues.end())
            {
                // Found a range of queues to remove.
                //
                blocked_channel_queues.erase(
                    erase_iter,
                    blocked_channel_queues.end());
            }
        }
    }

    // ----------------------------------------------------------------------
    bool ClientSession::add_pending_unblocked_channel(
        const ChannelId channel_id)
    {
        bool added = false;

        if (channel_id)
        {
            // Make sure channel isn't already in the list
            //
            if (std::find(
                pending_channels_unblock.begin(),
                pending_channels_unblock.end(),
                channel_id) == pending_channels_unblock.end())
            {
                pending_channels_unblock.push_back(channel_id);
                added = true;
            }
        }

        return added;
    }

    // ----------------------------------------------------------------------
    bool ClientSession::remove_pending_unblocked_channel(
        const ChannelId channel_id)
    {
        bool removed = false;

        if (channel_id)
        {
            ChannelIds::iterator channel_iter = std::find(
                pending_channels_unblock.begin(),
                pending_channels_unblock.end(),
                channel_id);

            if (channel_iter != pending_channels_unblock.end())
            {
                // Found channel to remove.  Since there should be so few
                // of these, right now just use the standard inefficient
                // vector removal.
                pending_channels_unblock.erase(channel_iter);
                removed = true;
            }
        }

        return removed;
    }

    // ----------------------------------------------------------------------
    void ClientSession::request_service()
    {
        if (not has_requested_service)
        {
            router_ptr->session_has_pending_actions(this);
            has_requested_service = true;
        }
    }

    // ----------------------------------------------------------------------
    MessageSerialId ClientSession::get_next_message_id(void)
    {
        // Since messages are constantly being put on and acknowledged,
        // there's no such thing as a long lasting message.  Therefore,
        // assuming the window size is reasonable, there's no reason we
        // have to check for duplicates.
        //
        ++last_used_message_ser_id;

        if (last_used_message_ser_id == 0)
        {
            ++last_used_message_ser_id;
        }

        return last_used_message_ser_id;
    }

    // ----------------------------------------------------------------------
    ChannelId ClientSession::get_next_channel_id(void)
    {
        ChannelId id = 0;

        if (active_channels.size() >= MAX_CHANNELS)
        {
            LOG(fatal, "comm", "get_next_channel_id",
                "No more Channel IDs available!");
        }
        else
        {
            while (not id)
            {
                ++last_used_channel_id;

                if (last_used_channel_id >= MAX_CHANNELS)
                {
                    last_used_channel_id = 1;
                }

                if (not get_channel_by_id(last_used_channel_id))
                {
                    // Found next available ID
                    //
                    id = last_used_channel_id;
                }
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    ClientSession::ChannelInfo::ChannelInfo(
        const ChannelId channel_id,
        events::Channel *channel,
        const bool channel_out)
      : id(channel_id),
        channel_ptr(channel),
        out(channel_out),
        closed(false),
        blocked(false)
    {
    }

    // ----------------------------------------------------------------------
    ClientSession::ChannelInfo::ChannelInfo(void)
        : id(0),
          channel_ptr(0),
          out(false),
          closed(false),
          blocked(false)
    {
    }

    // ----------------------------------------------------------------------
    bool ClientSession::ChannelInfo::valid(void) const
    {
        return channel_ptr;
    }
}
}
