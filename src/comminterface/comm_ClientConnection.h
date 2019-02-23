/*
 * comm_ClientConnection.h
 */

#ifndef MUTGOS_COMM_CLIENTCONNECTION_H
#define MUTGOS_COMM_CLIENTCONNECTION_H

#include <string>

#include "comminterface/comm_CommonTypes.h"
#include "clientmessages/message_ChannelStatusChange.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"

#include "text/text_ExternalText.h"
#include "clientmessages/message_ClientMessage.h"

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace events
{
    // Forward declarations for channels
    //
    class Channel;
    class TextChannel;
    class ClientDataChannel;
}

namespace comm
{
    // Forward declaration
    //
    class ClientSession;

    /**
     * Interface class that connections to the outside world must implement.
     * Each instance represents a single connection by a client.
     *
     * This is not designed to be thread safe, as the RouterSessionManager
     * is single-threaded.
     */
    class ClientConnection
    {
    public:
        /**
         * Enums representing status after sending data to a client.
         */
        enum SendReturnCode
        {
            /** Sent data was completely accepted */
            SEND_OK,
            /** Sent data was completely accepted, but is now blocked */
            SEND_OK_BLOCKED,
            /** Data was not accepted */
            SEND_BLOCKED,
            /** Data was not accepted because client is disconnected */
            SEND_DISCONNECTED,
            /** The type of data is not supported on this client connection */
            SEND_NOT_SUPPORTED
        };

        /**
         * What mode a client wants to connect as.  This will determine what
         * sorts of channels are open, and if they will appear as online or
         * not.
         */
        enum ClientType
        {
            /** Client connecting to admin console */
            CLIENT_TYPE_ADMIN,
            /** Normal, interactive client connection */
            CLIENT_TYPE_INTERACTIVE,
            /** Batch mode (database access only) */
            CLIENT_TYPE_BATCH
        };

        /**
         * Required virtual destructor.
         */
        virtual ~ClientConnection()
          { }

        /**
         * @return The send and receive window sizes, if an enhanced
         * client.
         */
        virtual MG_UnsignedInt get_client_window_size(void) =0;

        /**
         * @return True if this is an enhanced connection, meaning
         * ClientDataChannel(s) will be available.
         */
        virtual bool client_is_enhanced(void) =0;

        /**
         * @return The type of connection the client wants.
         */
        virtual ClientType get_client_type(void) =0;

        /**
         * @return True if nothing more can currently be sent on the connection.
         * This can happen when the outgoing OS buffer is full, network
         * congestion, disconnected, slow speeds, etc.
         */
        virtual bool client_is_send_blocked(void) =0;

        /**
         * @return True if currently connected.
         */
        virtual bool client_is_connected(void) =0;

        /**
         * Forcibly disconnects the connection.
         */
        virtual void client_disconnect(void) =0;

        /**
         * @return The site this connection is associated with.
         */
        virtual dbtype::Id::SiteIdType client_get_site_id(void) =0;

        /**
         * @return The source of the connection.  For IP-based connections,
         * this might be the hostname or IP address of the client.  IM-based
         * connections might be a username.
         */
        virtual const std::string &client_get_source(void) =0;

        /**
         * Called by the Router when being authenticated, this will set
         * the entity ID associated with the connection when it is known.
         * @param entity_id[in] The entity ID (Player Entity) associated
         * with the connection.
         */
        virtual void client_set_entity_id(const dbtype::Id &entity_id) =0;

        /**
         * @return The entity ID (Player Entity) associated with the
         * connection, if set.
         */
        virtual dbtype::Id client_get_entity_id(void) =0;

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
            const MessageSerialId ser_id) =0;

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
            const MessageSerialId ser_id) =0;

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
            const message::ChannelStatusChange &channel_status) =0;

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
            const text::ExternalTextLine &text_line) =0;

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
            const message::ClientMessage &client_message) =0;

    protected:

        /**
         * Interface constructor used by subclasses.
         */
        ClientConnection(void)
          { }

    private:
        // No copying
        ClientConnection &operator=(const ClientConnection &rhs);
        ClientConnection(const ClientConnection &rhs);
    };
}
}

#endif //MUTGOS_COMM_CLIENTCONNECTION_H
