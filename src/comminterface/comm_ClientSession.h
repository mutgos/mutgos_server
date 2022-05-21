/*
 * comm_ClientSession.h
 */

#ifndef MUTGOS_COMM_CLIENTSESSION_H
#define MUTGOS_COMM_CLIENTSESSION_H

#include <deque>
#include <vector>
#include <map>
#include <boost/thread/recursive_mutex.hpp>

#include "channels/events_ChannelControlListener.h"
#include "channels/events_TextChannelReceiver.h"
#include "channels/events_ClientDataReceiver.h"

#include "dbtypes/dbtype_TimeStamp.h"
#include "dbtypes/dbtype_Id.h"

#include "text/text_ExternalText.h"

#include "comminterface/comm_CommonTypes.h"
#include "comminterface/comm_RouterEvent.h"
#include "comminterface/comm_ClientConnection.h"
#include "comminterface/comm_SessionStats.h"
#include "comminterface/comm_ClientChannelInfo.h"

#include "clientmessages/message_ChannelStatus.h"

namespace mutgos
{
// Forward declarations
//
namespace events
{
    class Channel;
}

namespace message
{
    class ChannelStatusChange;
}

namespace comm
{
    // Forward declarations.
    //
    class RouterSessionManager;

    /**
     * Manages everything related to comms for a single client's session.
     * This performs work that generally considered to be common to every
     * comm Driver such as resending data after a reconnect.  This is also the
     * interface point between Channels and the client.
     *
     * Because this class is thread safe, having a getter for the
     * ClientConnection is not allowed because only the router should have
     * access to it (and it already does).  In theory, the pointer could
     * be freed soon after calling the getter, making the pointer invalid
     * and causing a crash.
     *
     * Also, while this class is considered thread safe, it has rules on when
     * methods can be called.  When in process_pending(), only Channel
     * listener methods can be called, and any getter methods.  When not in
     * process_pending(), any of the other methods can be called at the same
     * time without issue.  If this is not followed, a deadlock or unexpected
     * behavior can result.  From the perspective of the comm module, this
     * is single threaded.  From a Channel perspective, it is always safe to
     * call.
     *
     * // TODO Add rigorous window size checks for incoming data to prevent maliciously sending large amounts of data
     * // TODO May require redesign to better deal with channels without excessive lock/unlocks  Messaging passing/queue??
     */
    class ClientSession :
        public events::ChannelControlListener,
        public events::TextChannelReceiver,
        public events::ClientDataReceiver
    {
    public:
        /**
         * Creates a new ClientSession.  This is used only when a client
         * connects and there is no existing ClientSession for them.
         * @param id[in] The unique session ID for this instance.
         * @param router[in] The router in use.
         * @param client[in] Pointer to the client connection for this session.
         * Ths pointer will be owned by this class instance.
         */
        ClientSession(
            const SessionId id,
            RouterSessionManager *router,
            ClientConnection *client);

        /**
         * Destructor. Unregister from any known channels and disconnects
         * the client.
         */
        virtual ~ClientSession();

        /**
         * Sets a new client connection for this session (such as during a
         * reconnect).
         * If there is already a ClientConnection, disconnect will be called on
         * the original connection, the original connection will be
         * released (freed), and the client will be notified as to the
         * last
         * @param connection_ptr[in] The new ClientConnection to set.
         */
        void set_client_connection(ClientConnection *connection_ptr);

        /**
         * Informs the ClientSession the client has disconnected.  No pointers
         * are freed or changed.
         */
        void client_disconnected(void);

        /**
         * @return A ConnectionStats object populated with data about the
         * session.
         */
        SessionStats get_stats(void);

        /**
         * @return The Entity ID associated with this session.
         */
        const dbtype::Id &get_entity_id(void) const
          { return client_entity_id; }

        /**
         * @return The unique session ID associated with this session.
         */
        const SessionId get_session_id(void) const
          { return session_id; }

        /**
         * @return When the session was created.
         */
        const dbtype::TimeStamp &get_session_creation_time(void) const
          { return session_established_time; }

        /**
         * @return The last time data (not connection or acknowledgement
         * related) was received from the client.  This is a copy.
         */
        dbtype::TimeStamp get_session_activity_time(void);

        /**
         * Sets the 'last activity' time to now.  Normally this is not
         * needed, since it is done automatically when activity is presented
         * to this class.
         */
        void set_activity_time_to_now(void);

        /**
         * @return True if there is currently a connection to a client.
         */
        bool is_connected(void);

        /**
         * Requests the server initiate a disconnection.  No reconnect
         * will be allowed.
         */
        void request_disconnection(void);

        /**
         * Called as needed to handle any pending data or actions this class
         * instance needs to perform.  Harmless to call if nothing is pending.
         *
         * When in this method, no other methods shall be called except for
         * Channel listeners.
         */
        void process_pending(void);

        /**
         * Called when a client has acknowledged receipt of data.
         * Must be guaranteed not to be called when in process_pending().
         * @param ser_id[in] The highest serial number of the data being
         * acknowledged.
         */
        void client_data_acknowledge(const MessageSerialId ser_id);

        /**
         * Called when a client has reconnected and wants to indicate
         * the last message it received.
         * Must be guaranteed not to be called when in process_pending().
         * @param ser_id[in] The highest serial number of the data being
         * acknowledged.
         */
        void client_data_acknowledge_reconnect(const MessageSerialId ser_id);

        /**
         * Called when a client's connection is now able to send more data
         * out.
         * Must be guaranteed not to be called when in process_pending().
         */
        void client_unblocked();

        /**
         * @return Information on all currently open channels for this
         * client session.
         */
        ClientChannelInfoVector get_current_channel_info(void);

        /**
         * Called when text data has been received from the client.
         * Must be guaranteed not to be called when in process_pending().
         * @param channel_id[in] The ID of the channel the data has been
         * received on.
         * @param ser_id[in] The serial number of the text data, used for
         * ACK of receipt.
         * @param text_line_ptr[in] The data itself.  Control of the pointer
         * will pass to this method.
         */
        void client_data(
            const ChannelId channel_id,
            const MessageSerialId ser_id,
            text::ExternalTextLine *text_line_ptr);

        /**
         * Called when enhanced data has been received from the client.
         * Must be guaranteed not to be called when in process_pending().
         * @param channel_id[in] The ID of the channel the data has been
         * received on.
         * @param ser_id[in] The serial number of the enhanced data, used for
         * ACK of receipt.
         * @param client_message_ptr[in] The data itself.  Control of the
         * pointer will pass to this method.
         */
        void client_data(
            const ChannelId channel_id,
            const MessageSerialId ser_id,
            message::ClientMessage *client_message_ptr);

        /**
         * Called when client is requesting for a Channel to be closed.
         * Must be guaranteed not to be called when in process_pending().
         * @param channel_id[in] The ID of the Channel to close.
         */
        void client_request_channel_close(const ChannelId channel_id);

        /**
         * Adds a new channel to this client session.
         * Assumes the channel has not been added before.
         * @param channel_ptr[in] Pointer to the channel.  This class will
         * register itself as a listener and hold onto the pointer until
         * the channel is closed or this class is destructed.
         * @return The ID for the channel.
         */
        ChannelId channel_added(
            events::Channel *const channel_ptr,
            const bool to_client);

        /**
         * Called when a channel is blocked.
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.
         */
        virtual void channel_flow_blocked(
            const std::string &channel_name,
            events::Channel *const channel_ptr);

        /**
         * Called when a channel is now unblocked.
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.
         */
        virtual void channel_flow_open(
            const std::string &channel_name,
            events::Channel *const channel_ptr);

        /**
         * Called when a channel is permanently closed (will not reopen).
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.
         */
        virtual void channel_flow_closed(
            const std::string &channel_name,
            events::Channel *const channel_ptr);

        /**
         * Called just before a channel's class instance is to be destructed
         * (freed).
         * @deprecated This is deprecated under the new Channel design.
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.   This will be invalid
         * after the method returns.
         */
        virtual void channel_destructed(
            const std::string &channel_name,
            events::Channel *const channel_ptr);

        /**
         * Called when a channel has enhanced client data on a channel
         * destined to the client.
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.
         * @param client_message_ptr[in] The client message.  Ownership of the
         * pointer will be transferred to this method.
         */
        virtual void client_channel_data(
            const std::string &channel_name,
            events::ClientDataChannel *channel_ptr,
            message::ClientMessage *client_message_ptr);

        /**
         * Called when a channel has text data on a channel
         * destined to the client.
         * @param channel_name[in] The name of the channel.
         * @param channel_ptr[in] Pointer to the channel.
         * @param text_line[in] The text data.  Ownership of pointers within
         * the line will be owned by this class.
         */
        virtual void text_channel_data(
            const std::string &channel_name,
            events::TextChannel *channel_ptr,
            text::ExternalTextLine &text_line);

    private:

        typedef std::deque<RouterEvent> EventQueue; ///< Queue of events

        /**
         * Simple container class that holds information about a channel.
         * It has no intelligence (rare, huh?).
         */
        struct ChannelInfo
        {
            /**
             * Creates a ChannelInfo and fills in the provided values.
             */
            ChannelInfo(
                const ChannelId channel_id,
                events::Channel *channel,
                const bool channel_out);

            /**
             * Creates an invalid ChannelInfo.
             */
            ChannelInfo(void);

            /**
             * @return True if this is an instance with valid data.
             */
            bool valid(void) const;

            ChannelId id; ///< ID of the channel for this session.
            events::Channel *channel_ptr; ///< Pointer to Channel itself
            bool out; ///< True if this channel sends towards the client
            bool closed; ///< True if channel is closed (will never reopen)
            bool blocked; ///< True if channel is blocked or new data must still be queued
        };

        /**
         * Write locking is assumed to have already been performed.
         * This will update class attributes concerning current connection
         * status (such as blocked).
         * @param code[in] The return code to process
         * @return True if message successfully sent.
         */
        bool process_send_return_code(
            const ClientConnection::SendReturnCode code);

        /**
         * Read locking is assumed to have already been performed.
         * @param channel_id[in] The channel ID to search for.
         * @return Pointer to the channel if found, or null if not.  Do not
         * delete the pointer!
         */
        events::Channel *get_channel_by_id(const ChannelId channel_id);

        /**
         * Read locking is assumed to have already been performed.
         * @param channel_ptr[in] The channel to search for.
         * @return Info about channel if found, or 0 if not.
         */
        ChannelId get_channel_id(const events::Channel *channel_ptr);

        /**
         * Gets the full channel information via a Channel pointer.
         * Read locking is assumed to have already been performed.
         * @param channel_ptr[in] The pointer of the Channel whose data is to
         * be retrieved.
         * @return The ChannelInfo for the channel, or invalid (all zeros) if
         * not found.
         */
        ChannelInfo &get_channel_info(const events::Channel *channel_ptr);

        /**
         * Gets the full channel information via a Channel ID.
         * Read locking is assumed to have already been performed.
         * @param channel_id[in] The ID of the Channel whose data is to
         * be retrieved.
         * @return The ChannelInfo for the channel, or invalid (all zeros) if
         * not found.
         */
        ChannelInfo &get_channel_info(const ChannelId channel_id);

        /**
         * Creates a ChannelStatusChange instance, fully populated.
         * @param channel_info[in] The channel info for the Channel.
         * @param channel_status[in] The status for the channel.
         * @return A new, populated ChannelStatusChange instance.  Caller
         * owns the pointer.
         */
        message::ChannelStatusChange *make_channel_status_change(
            ChannelInfo &channel_info,
            const message::ChannelStatus channel_status);

        /**
         * Processes all pending channel unblocks, if any.
         * No locking on channels or the class must be performed prior
         * to calling.
         */
        void process_pending_unblocked_channels(void);

        /**
         * Processes all pending channel deletes, if any.
         * No locking on channels or the class must be performed prior
         * to calling.
         */
        void process_pending_channel_deletes(void);

        /**
         * Write locking is assumed to have already been performed.
         * Deletes a channel from the data structures.
         * @param channel_id[in] The ID of the channel to delete.
         * @return ChannelInfo of the channel removed, or invalid if not found.
         */
        ChannelInfo delete_channel(const ChannelId channel_id);

        /**
         * Used when we're done with a channel, this will close it and
         * unregister us as a listener.
         * No locking on the channel or class must be performed prior to
         * calling, or a deadlock may result.
         * The channel pointer should be considered invalid when this
         * returns.
         * @param channel_info[in] The channel to unregister from.
         */
        void unregister_channel(ChannelInfo &channel_info);

        /**
         * Write locking is assumed to have already been performed.
         * @param channel_id[in] The channel ID of the blocked queue to
         * make or retrieve.
         * @return A reference to the new or existing blocked queue.
         */
        EventQueue &get_blocked_queue(const ChannelId channel_id);

        /**
         * Write locking is assumed to have already been performed.
         * Attempts to remove the given channel from the blocked queues.
         * Assumes all pending data in the queue has been sent and that
         * anything remaining is safe to delete.  If the container allows
         * removal of the queue without disturbing other queues, it will be
         * removed, otherwise no action will occur.
         * @param channel_id[in] The channel ID whose blocked queue is to be
         * removed.
         */
        void remove_blocked_queue(const ChannelId channel_id);

        /**
         * Write locking is assumed to have already been performed.
         * Adds a recently unblocked channel to the pending unblocked list,
         * so it can be handled later.  It is safe to call this more than
         * once for the same channel.
         * @param channel_id[in] The channel ID now unblocked.
         * @return True if channel added, false if already there.
         */
        bool add_pending_unblocked_channel(const ChannelId channel_id);

        /**
         * Write locking is assumed to have already been performed.
         * Removed a channel from the pending unblocked list.  This may happen
         * if a channel was unblocked but then suddenly became blocked again.
         * It is safe to call this more than once for the same channel.
         * @param channel_id[in] The channel ID.
         * @return True if channel removed, false if not found.
         */
        bool remove_pending_unblocked_channel(const ChannelId channel_id);

        /**
         * Write locking is assumed to have already been performed.
         * If not already done, informs the router that this ClientSession
         * needs service (process_pending() needs to be called).
         */
        void request_service(void);

        /**
         * Write locking is assumed to have already been performed.
         * @return The next message serial ID.
         */
        MessageSerialId get_next_message_id(void);

        /**
         * Write locking is assumed to have already been performed.
         * @return The next channel ID not in use.
         */
        ChannelId get_next_channel_id(void);

        // No copying
        ClientSession &operator=(const ClientSession &rhs);
        ClientSession(const ClientSession &rhs);

        typedef std::vector<ChannelId> ChannelIds; ///< Vector of Channel IDs
        typedef std::pair<ChannelId, EventQueue> ChannelToQueue; ///< Maps Channel ID to a specific EventQueue for it

        typedef std::vector<ChannelInfo> Channels; ///< Vector of active channels
        typedef std::vector<ChannelToQueue> BlockedChannelQueues; ///< Vector of blocked channel pointer to queue

        EventQueue outgoing_events;  ///< Events that need to go to client, oldest first
        EventQueue sent_events;  ///< Events that have already gone to client but not ACKed, oldest first.

        MessageSerialId outgoing_ser_ack; ///< Highest ACKed serial ID of messages TO client.
        MessageSerialId incoming_ser_ack; ///< Highest pending ACKed serial ID of messages FROM client.
        bool needs_incoming_ser_ack_sent; ///< True if incoming_ser_ack needs to be sent to client.

        bool has_requested_service; ///< True if requested service from Router
        bool need_handle_reconnect; ///< Need to send reconnect data and/or preprocessing before anything else
        bool need_disconnect; ///< A condition arose that requires a deferred disconnect
        bool wait_reconnect_response; ///< Waiting on reconnect response from client (last message received)

        bool client_is_blocked; ///< True if client not currently accepting data to it
        bool client_is_connected; ///< True if currently connected
        bool client_is_enhanced; ///< True if enhanced client, false if plain text only
        ClientConnection::ClientType client_type; ///< Type of client connected
        std::string client_source; ///< Where client is connecting from (hostname, IP, etc)
        const dbtype::Id client_entity_id; ///< Entity ID associated with client

        ChannelId last_used_channel_id; ///< Last ID used for a new channel
        MessageSerialId last_used_message_ser_id; ///< Last ID used for a new message

        const SessionId session_id; ///< The ID for this session.
        MG_UnsignedInt client_window_size; ///< Window size to/from client

        Channels active_channels; ///< Channels active (including blocked) in this session
        BlockedChannelQueues blocked_channel_queues;  ///< Queued up data for blocked channels (towards server)
        ChannelIds pending_channels_delete; ///< Channels to be deleted
        ChannelIds pending_channels_unblock; ///< Channels unblocked that need backlog processed first

        const dbtype::TimeStamp session_established_time; ///< When session was first created.
        dbtype::TimeStamp last_activity_time; ///< The last time data from client received.

        ClientConnection *client_ptr; ///< The current client connection associated with this session
        RouterSessionManager *router_ptr; ///< Pointer to active router

        boost::recursive_mutex client_lock; ///< Lock for class instance
    };
}
}

#endif //MUTGOS_COMM_CLIENTSESSION_H
