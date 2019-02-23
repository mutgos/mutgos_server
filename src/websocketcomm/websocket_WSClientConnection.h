/*
 * websocket_WSClientConnection.h
 */

#ifndef MUTGOS_WEBSOCKET_WSCLIENTCONNECTION_H
#define MUTGOS_WEBSOCKET_WSCLIENTCONNECTION_H

#include <string>
#include <memory>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/json_JsonUtilities.h"

#include "dbtypes/dbtype_Id.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_AuthenticationRequest.h"
#include "clientmessages/message_ChannelData.h"

#include "comminterface/comm_ClientConnection.h"

namespace mutgos
{
namespace comm
{
    // Forward declarations.
    //
    class ClientSession;
}

namespace websocket
{
    // Forward declarations.
    //
    class RawWSConnection;
    class WebsocketDriver;

    /**
     * A client connection to a websocket.  This implements the core
     * send/receive logic, authentication, etc. on a per websocket basis.
     * This is NOT thread safe.
     *
     * Note that non-Channel data request mesages can only be processed one at
     * a time.  That is, this can get a request message, and send a
     * single response in turn.  If another request message comes in before
     * the response can go out, not all responses will be sent because this
     * does not have a proper queueing system.  Therefore, clients must
     * only send one request at a time, and wait for the response before
     * issuing another request.
     *
     * Once authenticated, there is currently no reason to do an out of channel
     * request-response, so this works well enough for now.
     *
     * TODO add batch, admin modes for client type.
     */
    class WSClientConnection : public comm::ClientConnection
    {
    public:
        /**
         * Constructor.
         * @param driver[in] Pointer to the driver instance associated with
         * this WSClientConnection instance.
         * @param connection[in] Shared pointer to the raw connection
         * associated with the websocket.
         * @param source[in] Where the client is connecting from
         * (IP address, etc).
         */
        WSClientConnection(
            WebsocketDriver *driver,
            std::shared_ptr<RawWSConnection> connection,
            const std::string &source);

        /**
         * Destructor.  Will disconnect the websocket if still connected.
         * This should ONLY be called by WebsocketDriver.
         */
        virtual ~WSClientConnection();

        /**
         * Called by the driver if the connection is to be disconnected
         * and cleaned up.
         */
        void stop(void);

        /**
         * @return The send and receive window sizes, in number of messages.
         */
        virtual osinterface::OsTypes::UnsignedInt get_client_window_size(void);

        /**
         * @return True since a websocket connection is always an enhanced
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
         * @return The source of the connection.  For IP-based connections,
         * this might be the hostname or IP address of the client.  IM-based
         * connections might be a username.
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
         * Sends an acknowledgement that a message(s) have been received.
         * If sending the highest message received, intermediate messages
         * will automatically be considered as received, too.
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
         * @param channel_id[in] The ID of the channel the data is being sent
         * out on.
         * @param ser_id[in] The serial number of the message.
         * @param client_message[in] The data to send.
         * @return A status code indicating if the message could be sent.
         */
        virtual SendReturnCode client_send_data(
            const comm::ChannelId channel_id,
            const comm::MessageSerialId ser_id,
            const message::ClientMessage &client_message);

        /**
         * Called by the driver to allow the connection to handle pending
         * actions.
         */
        void do_work(void);

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
         * data from the client to be processed.  This will be one websocket
         * message at a time.
         * @param data_ptr[in] A buffer with all the data for a websocket
         * message.  Ownership of the pointer transfer to this method,
         * which is responsible for cleaning it up after.
         * @param data_size[in] The size of data_ptr.
         */
        void raw_data(char *data_ptr, const size_t data_size);

        /**
         * Called by the raw connection class instance when the timer
         * has expired.
         */
        void raw_timer_expired(void);

    private:

        /**
         * Puts the message on the actual send queue if there is room, not
         * blocked, etc.
         * If the method accepts the message, it will be sent very soon asynch
         * unless a disconnection occurs.
         * @param message[in] The message to send.
         * @return If the send was successful.
         */
        SendReturnCode send_message_raw(
            const message::ClientMessage &message);

        /**
         * Unconditionally puts the message on the actual send queue.  It will
         * be sent very soon asynch unless a disconnection occurs.
         * Most methods want to use send_message_raw() instead of this one.
         * @param message[in] The message to send.
         * @return True if serialization and queueing is successful.
         */
        bool queue_message_to_send(const message::ClientMessage &message);

        /**
         * Requests an immediate disconnect of the socket; internal states
         * are updated as needed.
         */
        void disconnect_socket(void);

        /**
         * Deserializes the provided JSON into a ClientMessage.
         * @param json_ptr[in] The JSON to parse.  The pointer will be owned by
         * this method.
         * @return A pointer to the deserialized message, if successful, or
         * null if error or unrecognized message.  Caller must manage the
         * pointer!
         */
        message::ClientMessage *restore_message(
            json::JsonParsedObject *json_ptr);

        /**
         * Processes an incoming message from the client.
         * @param message_ptr[in] The message to process.  Control of the
         * pointer will pass to this method.
         */
        void process_message(message::ClientMessage *message_ptr);

        /**
         * Handles a request site list message and sends the response.
         */
        void process_request_site_list(void);

        /**
         * Handles a request to authenticate the connection and sends
         * the response.
         * @param request[in] The authentication request.
         */
        void process_authentication_request(
            message::AuthenticationRequest &request);

        /**
         * Handles channel data, distributing it to the indicated channel.
         * @param channel_data[in,out] The channel data.  Ownership of the
         * contents will be transferred and removed from channel_data.
         */
        void process_channel_data(message::ChannelData &channel_data);

        /**
         * Request the driver call this class back later to perform service.
         */
        void request_service(void);


        /**
         * Used to indicate the phase of a graceful disconnect.
         */
        enum DisconnectState
        {
            /** Normal state; not disconnecting */
            DISCONNECT_STATE_NOT_REQUESTED,
            /** Requested to disconnect, has not yet sent message */
            DISCONNECT_STATE_REQUESTED,
            /** Disconnect message sent.  Once unblocked, close socket */
            DISCONNECT_STATE_SENT
        };

        MG_UnsignedInt client_window_size; ///< Send/recv window size, counted in messages
        comm::ClientConnection::ClientType client_type; ///< Type/mode of client connected
        std::string client_source; ///< Where the client is connecting from (IP address, etc)
        dbtype::Id client_entity_id; ///< Entity ID associated with the client, once authenticated

        bool client_blocked; ///< True if sending is currently blocked.
        bool client_connected; ///< True if websocket is currently connected.
        bool client_error; ///< True if client has an error condition requiring disconnection.
        DisconnectState client_disconnect_state; ///< What phase of disconnect we're in
        bool requested_service; ///< True if services has been requested on the driver

        MG_UnsignedInt outgoing_size; ///< Estimated bytes of pending outgoing data
        json::JSONRoot outgoing_json_node; ///< Temporary holding spot while building up outgoing data

        MG_UnsignedInt auth_attempts; ///< Number of bad attempts to authenticate

        comm::ClientSession *client_session_ptr; ///< Pointer to client session, when authenticated.
        WebsocketDriver * const driver_ptr; ///< Pointer to driver.
        std::shared_ptr<RawWSConnection> raw_connection; ///< Shared pointer to the raw connection.
    };
}
}

#endif //MUTGOS_WEBSOCKET_WSCLIENTCONNECTION_H
