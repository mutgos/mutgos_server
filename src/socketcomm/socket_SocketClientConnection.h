/*
 * socket_SocketClientConnection.h
 */

#ifndef MUTGOS_SOCKET_SOCKETCLIENTCONNECTION_H
#define MUTGOS_SOCKET_SOCKETCLIENTCONNECTION_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <deque>
#include <map>
#include <stddef.h>

#include <boost/shared_ptr.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/json_JsonUtilities.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_ChannelData.h"

#include "comminterface/comm_CommonTypes.h"
#include "comminterface/comm_ClientConnection.h"

#include "socket_CommandProcessor.h"

namespace mutgos
{
namespace comm
{
    // Forward declarations.
    //
    class ClientSession;
}

namespace socket
{
    // Forward declarations.
    //
    class RawSocketConnection;
    class SocketDriver;

    /**
     * A client connection to a socket.  This implements the core
     * send/receive logic, authentication, etc. on a per socket basis.
     * This is NOT thread safe.
     *
     * Note that non-authenticated and driver specific type commands
     * ('connect', color on/off, etc) can only be processed one at a time.
     * That is, this can get a driver specific command, and send a single
     * response in turn.  If another driver specific command comes in before
     * the response can go out, not all responses will be sent because this
     * does not have a proper queueing system.  Therefore, clients must
     * only send one command at a time, and wait for the response before
     * issuing another request.
     *
     * ## is used to send the text after it to the lowest input channel on
     * the stack, which should be the user's agent process.
     *
     * # is used to send a command directly to this class, like for turning
     * color on and off.
     *
     * This is a bit different than a typical raw socket driver in that it
     * maintains a history of the last few lines sent to the client.  This
     * is used so that if the client has connection issues, they won't miss
     * any lines when they reconnect.  Unfortunately, this complicates the
     * code quite a bit because we have no way of knowing what lines were
     * actually received by the client, and can only guess.  The general idea
     * is to not acknowledge (to ClientSession) the last few lines sent to the
     * client, meaning it must keep track of the last few serial IDs.
     *
     * If enough incoming data is received from the client, it will trigger a
     * significant reduction in lines not being acknowledged, with the
     * assumption that the connection must be OK both directions.  This will
     * avoid spamming the user with the full history it could maintain should
     * they reconnect due to a sudden socket disconnect (NAT, computer crash,
     * etc).
     *
     * TODO add batch, admin modes for client type.
     */
    class SocketClientConnection : public comm::ClientConnection
    {
    public:
        /**
         * Constructor.
         * @param driver[in] Pointer to the driver instance associated with
         * this SocketClientConnection instance.
         * @param connection[in] Shared pointer to the raw connection
         * associated with the socket.
         * @param source[in] Where the client is connecting from
         * (IP address, etc).
         */
        SocketClientConnection(
            SocketDriver *driver,
            boost::shared_ptr<RawSocketConnection> connection,
            const std::string &source);

        /**
         * Destructor.  Will disconnect the socket if still connected.
         * This should ONLY be called by SocketDriver.
         */
        virtual ~SocketClientConnection();

        /**
         * Called by the driver if the connection is to be disconnected
         * and cleaned up.
         */
        void stop(void);

        /**
         * Called by the driver to allow the connection to handle pending
         * actions.
         */
        void do_work(void);

        /**
         * @return The driver pointer.
         */
        SocketDriver *get_driver(void) const
        { return driver_ptr; }

        /**
         * @return The send and receive window sizes, in number of messages.
         */
        virtual osinterface::OsTypes::UnsignedInt get_client_window_size(void);

        /**
         * @return False since a socket connection is never an enhanced
         * client at this time.
         */
        virtual bool client_is_enhanced(void);

        /**
         * @return The type of connection the client wants.
         */
        virtual ClientType get_client_type(void);

        /**
         * @return True if nothing more can currently be sent on the connection.
         * This can happen when the outgoing OS buffer is full, network
         * congestion, disconnected, slow speeds, etc.
         */
        virtual bool client_is_send_blocked(void);

        /**
         * @return True if currently connected.
         */
        virtual bool client_is_connected(void);

        /**
         * Forcibly disconnects the connection.  The Session will not be
         * notified.
         */
        virtual void client_disconnect(void);

        /**
         * @return The site this connection is associated with, or default
         * if not yet attempted to authenticate.
         */
        virtual dbtype::Id::SiteIdType client_get_site_id(void);

        /**
         * Used to set the site ID prior to authentication.
         * If authentication has already occurred, this will have no effect.
         * @param site_id[in] The site ID to set.
         */
        void client_set_site_id(const dbtype::Id::SiteIdType site_id);

        /**
         * @return The source of the connection.  For IP-based connections,
         * this would be the hostname or IP address of the client.
         */
        virtual const std::string &client_get_source(void);

        /**
         * Called by the Router when being authenticated, this will set
         * the entity ID associated with the connection when it is known.
         * @param entity_id[in] The entity ID (Player Entity) associated
         * with the connection.
         */
        virtual void client_set_entity_id(const dbtype::Id &entity_id);

        /**
         * @return The entity ID (Player Entity) associated with the
         * connection, if set.
         */
        virtual dbtype::Id client_get_entity_id(void);

        /**
         * Sets the client session on this connection.
         * Once set to a valid pointer, it cannot be changed.
         * @param session_ptr[in] The pointer to the session.
         */
        void client_set_session(comm::ClientSession *session_ptr);

        /**
         * Sends the given text line to the active input channel.
         * @param line_ptr[in] The text line to send.  Ownership of the pointer
         * will pass to this method.
         * @param to_agent[in] If true, the text actually goes to the agent
         * instead of the currently active channel.
         */
        void send_to_input_channel(
            text::ExternalTextLine *line_ptr,
            const bool to_agent = false);

        /**
         * Sends an acknowledgement that a message(s) have been received.
         * If sending the highest message received, intermediate messages
         * will automatically be considered as received, too.
         * For sockets, this doesn't do anything because we can't tell clients
         * their line was received.
         * @param ser_id[in] The highest serial number of the message to be
         * acknowledged.  Note that serial numbers eventually wrap.
         * @return A status code indicating if the acknowledgement could
         * be sent.
         */
        virtual SendReturnCode client_send_acknowledge_data(
            const comm::MessageSerialId ser_id);

        /**
         * Called just after a client indicates it has reconnected.  This tells
         * the client the last message the Router received from it.
         * This is called by the Router first when a reconnection has been
         * detected.
         * For sockets, this doesn't do anything because we can't tell clients
         * their line was received.
         * @param ser_id[in] The highest serial number of the data being
         * acknowledged.
         * @return A status code indicating if the acknowledgement could
         * be sent.
         */
        virtual SendReturnCode client_send_acknowledge_data_reconnect(
            const comm::MessageSerialId ser_id);

        /**
         * Indicates a channel has been open, closed, blocked, etc.  This is
         * used both by the driver to adjust any internal status, and is to be
         * sent out to the client.
         * In our case, this is used to add and remove from the
         * @param ser_id[in] The serial number of the message.
         * @param channel_status[in] The channel status.
         * @return A status code indicating if the message could be sent.
         */
        virtual SendReturnCode client_channel_status_changed(
            const comm::MessageSerialId ser_id,
            const message::ChannelStatusChange &channel_status);

        /**
         * Sends text data to a client.
         * @param channel_id[in] The ID of the channel the data is being sent
         * out on.
         * @param ser_id[in] The serial number of the message.
         * @param text_line[in] The data to send.
         * @return A status code indicating if the message could be sent.
         */
        virtual SendReturnCode client_send_data(
            const comm::ChannelId channel_id,
            const comm::MessageSerialId ser_id,
            const text::ExternalTextLine &text_line);

        /**
         * Sends enhanced data to a client.
         * This is not supported on socket connections.
         * @param channel_id[in] The ID of the channel the data is being sent
         * out on.
         * @param ser_id[in] The serial number of the message.
         * @param client_message[in] The data to send.
         * @return A status code indicating this is not supported.
         */
        virtual SendReturnCode client_send_data(
            const comm::ChannelId channel_id,
            const comm::MessageSerialId ser_id,
            const message::ClientMessage &client_message);

        /**
         * Sends the given control text as soon as the socket can send
         * data again.  It is expected multiple lines to be sent at once
         * (basically the entire output from a command), as it is not possible
         * to easily send them one at a time with this method.
         *
         * Note that once text has been accepted for sending with this method,
         * it cannot be called again until the socket has finished sending
         * the output.  This is typically not a problem, because the general
         * flow is:  1) Get a command incoming from the socket, 2) call this
         * once with the response.  For now, it's not expected this would be
         * called without a corresponding user command.
         * @param text[in] The text to send, including newlines.
         * @return True if successfully queued to send, false if currently
         * sending or disconnected.
         */
        bool send_control_text_raw(const std::string &text);

        /**
         * Called when the connection has been fully established and is ready
         * for data to be sent.
         */
        void raw_ready(void);

        /**
         * Called by the raw connection class instance when it has finished
         * sending a buffer of data.  This implies it is no longer blocked
         * for new sending.
         */
        void raw_send_complete(void);

        /**
         * Called by the raw connection class instance when it has lost the
         * connection to the client.  This works one way: the client will
         * never reconnect using this raw and ClientConnection instance.
         */
        void raw_disconnected(void);

        /**
         * Called by the raw connection class instance when it has incoming
         * data from the client to be processed.
         * @param data_ptr[in] A buffer with all the data for a socket
         * message.  Ownership of the pointer does NOT transfer to this method.
         * When the call has completed, the data provided may change; it must
         * be copied as needed.
         * @param data_size[in] The size of data_ptr.
         */
        void raw_data(const char *data_ptr, const size_t data_size);

        /**
         * Called by the raw connection class instance when the timer
         * has expired.
         */
        void raw_timer_expired(void);

        /**
         * Sets whether or not ANSI is enabled.
         * @param enabled[in] True if enabled, false to strip out ANSI color.
         */
        void set_ansi_enabled(const bool enabled)
        { config_ansi_enabled = enabled; }

    private:

        /**
         * Puts the full text line on the actual send queue if there is room,
         * not blocked, etc.
         * If the method accepts the message, it will be sent very soon asynch
         * unless a disconnection occurs.
         * @param text[in] The full text line to send, excluding the newline at
         * the end.
         * @return If the send was successful, blocked, etc.
         */
        SendReturnCode send_text_line(const std::string &text);

        /**
         * Requests an immediate disconnect of the socket; internal states
         * and the session are updated as needed.  It is safe to call this
         * more than once.
         */
        void disconnect_socket(void);

        /**
         * Adds the message serial ID to the pending ack list.
         * @param ser_id[in] The serial ID to acknowledge.
         * @param size[in] The size of the encoded message.
         */
        void pending_ser_ack(
            const comm::MessageSerialId ser_id,
            const MG_UnsignedInt size = 0);

        /**
         * Determines what previously sent outgoing data needs to be ACKed
         * back to the ClientSession and does so, cleaning up
         * pending_serial_ids and pending_ids_message_size in the process.
         * @param from_client_input[in] True if ACK is due to enough input
         * from client arriving.
         */
        void ack_outgoing_data(const bool from_client_input);

        /**
         * Given new incoming data (which may be only part of a line),
         * determine if any complete lines have been created (combining with
         * incoming_text_buffer as needed), and if so, pass them to
         * process_incoming_line() after converting to UTF8 and
         * ExternalTextLine.  Leftover data will be stored in the buffer.
         * @param data[in,out] The raw data from the socket to process.  The
         * data may be modified directly for efficiency.
         */
        void process_raw_incoming_data(std::string &data);

        /**
         * The incoming ser ID is not really used for anything, but having
         * it sequentially increment (with auto-rollover) might be helpful
         * for debugging.
         * @return The next incoming message serial ID.
         */
        comm::MessageSerialId get_next_incoming_ser_id(void);

        /**
         * Request the driver call this class back later to perform service.
         */
        void request_service(void);


        /** First is channel ID, second is true if currently blocked */
        typedef std::pair<comm::ChannelId, bool> ChannelIdBlocked;
        typedef std::vector<ChannelIdBlocked> ChannelStack;

        /** First is the message serial ID, second is the encoded size of the message */
        typedef std::pair<comm::MessageSerialId, MG_UnsignedInt> SerialIdSize;
        typedef std::deque<SerialIdSize> PendingSerialIds;

        typedef std::pair<std::string, dbtype::TimeStamp> PuppetNameTimestamp;

        MG_UnsignedInt client_window_size; ///< Send/recv window size, counted in messages
        MG_UnsignedInt max_pending_data_size; ///< Targeted (soft limit) size (bytes) of pending outgoing data
        comm::ClientConnection::ClientType client_type; ///< Type/mode of client connected
        std::string client_source; ///< Where the client is connecting from (IP address, etc)
        dbtype::Id client_entity_id; ///< Entity ID associated with the client, once authenticated

        bool client_blocked; ///< True if sending is currently blocked.
        bool client_connected; ///< True if socket is currently connected.
        bool client_do_reconnect; ///< True if reconnect procedure is required
        bool requested_service; ///< True if services has been requested on the driver

        // Client configuration
        bool config_ansi_enabled;

        std::string outgoing_text_buffer; ///< Buffer/queue of outgoing data
        std::string outgoing_control_buffer; ///< Temporary buffer/queue of outgoing responses from control commands
        std::string incoming_text_buffer; ///< Buffered partial incoming line

        PendingSerialIds pending_serial_ids; ///< Outgoing message serial IDs that have yet to be ACKed
        MG_LongUnsignedInt pending_ids_message_size; ///< Size of encoded messages from pending_serial_ids added up
        MG_UnsignedInt ack_lines_received_from_client; ///< Number of lines recv from client starting from last 'mega ACK'.
        comm::MessageSerialId next_input_ser_id; ///< Next serID for data from the client

        // Channel information
        comm::ChannelId channel_main_input_id; ///< Represents the 'main input' channel, to the command processor
        ChannelStack channel_input_stack; ///< Stack of open input (from client) channels, with latest having a higher index
        ChannelStack channel_output_stack; ///< Stack of open output (to client) channels, with latest having a higher index
        std::map<comm::ChannelId, PuppetNameTimestamp> puppet_channel_info; ///< Maps puppet output channel to info.
        dbtype::TimeStamp last_puppet_check_time; ///< Last time we cleaned out old puppet channels.

        comm::ClientSession *client_session_ptr; ///< Pointer to client session, when authenticated.
        SocketDriver * const driver_ptr; ///< Pointer to driver.
        boost::shared_ptr<RawSocketConnection> raw_connection; ///< Shared pointer to the raw connection.
        CommandProcessor command_processor; ///< Processes incoming lines.
    };
}
}

#endif //MUTGOS_SOCKET_SOCKETCLIENTCONNECTION_H
