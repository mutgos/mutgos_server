/*
 * comm_RouterSessionManager.h
 */

#ifndef MUTGOS_COMM_ROUTER_H
#define MUTGOS_COMM_ROUTER_H

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/atomic/atomic.hpp>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

#include "comminterface/comm_CommonTypes.h"
#include "comminterface/comm_ClientSession.h"
#include "comminterface/comm_SessionStats.h"

namespace mutgos
{
// Forward declaractions
//
namespace events
{
    class Channel;
}

namespace message
{
    class ClientMessage;
}

namespace comm
{
    // Forward declarations.
    //
    class ClientConnection;
    class ConnectionDriver;

    // TODO Websocket driver could accept batches of output instead of one line at a time
    // TODO Add ping functionality if no recent activity

    /**
     * This class is both the Router, and Session Manager due to heavily
     * intertwined data.
     *
     * This class routes data to and from the various implementors of
     * ClientConnection (socket, websocket, etc).  It has code common to
     * all connection types, such as authentication.
     *
     * It also services all connection types on a single thread, however it
     * has to use frequent polling for this.
     * TODO Figure out a more efficient mechanism without frequent polling
     *
     * This class also keeps track of and manages what ClientSession is
     * associated with what Player, and also contains various algorithms related
     * to session management (getting list of who is online, idle disconnects,
     * channel management, etc).  Originally the functionality in this
     * paragraph was going to be in a separate class, but was decided the
     * complexity of dealing with pointers that could disappear at any point
     * was not worth it at this time.  Basically this class would become
     * a proxy/facade for everthing that other class would want.  The two
     * classes would be so closely intertwined that keeping them separate
     * didn't seem to have any benefit at this time.
     *
     * Currently this is single threaded.  In the future, multiple of these
     * could exist, running on different threads, with one set aside for
     * authentication, etc.  Drivers always take in a pointer to this
     * to allow for that possibility in the future.
     */
    class RouterSessionManager
    {
    public:
        typedef std::vector<SessionStats> SessionStatsVector;

        /**
         * Constructor.
         */
        RouterSessionManager();

        /**
         * Destructor.  This will call shutdown() automatically.
         */
        ~RouterSessionManager();

        /**
         * Initializes and starts the Router, including background thread(s).
         * @return True if successfully started or already started, or false
         * if error.
         */
        bool startup(void);

        /**
         * Shuts down the Router's background threads, if currently running,
         * and closes all connections and sessions.
         */
        void shutdown(void);

        /**
         * Adds a connection driver to the Router.  It will be polled
         * periodically.
         * This must be called before startup(), as it is not thread safe.
         * @param driver_ptr[in] The driver to add.  The pointer will be
         * owned by this class.
         */
        void add_connection_driver(ConnectionDriver *driver_ptr);

        /**
         * Used by Boost threads to start our threaded code.
         */
        void operator()();

        /**
         * @return The site IDs that currently have connections.
         */
        dbtype::Id::SiteIdVector get_entity_site_ids(void);

        /**
         * @param site_id[in] The site ID to get session stats for.
         * @return Information about every session for the site.
         */
        SessionStatsVector get_session_stats(
            const dbtype::Id::SiteIdType site_id);

        /**
         * @param site_id[in] The site ID to get online IDs for.
         * @return A list of IDs, representing the players who are currently
         * online for that site.  This will be empty if site ID is invalid
         * or no one is currently online for the site.
         */
        dbtype::Entity::IdVector get_online_ids(
            const dbtype::Id::SiteIdType site_id);

        /**
         * @param site_id[in] The site ID to check.
         * @return The number currently online at the site, or 0 if none or
         * site not found.
         */
        MG_UnsignedInt get_session_online_count(
            const dbtype::Id::SiteIdType site_id);

        /**
         * @param entity_id[in] Entity ID to get session stats for.
         * @return Information about the session associated with the
         * entity ID, or invalid stats if the entity doesn't exist or is not
         * currently connected.
         */
        SessionStats get_session_stats(const dbtype::Id &entity_id);

        /**
         * Adds a Channel to a session.
         * There is no 'remove channel' because closing a channel
         * removes it.
         * @param id[in] The Entity ID of who owns the session.
         * @param channel_ptr[in] The pointer to the Channel to add.  When this
         * method returns, if it indicates success, the pointer will be held
         * (but not owned) by this or a related subclass.
         * @param to_client[in] If true, the Channel is sending data TO the
         * client.  If false, the Channel is getting data FROM the client.
         * @return True if added, false if not (null pointer, id does
         * not correspond to a session).
         */
        bool add_channel(
            const dbtype::Id &id,
            events::Channel *channel_ptr,
            const bool to_client);

        /**
         * Forcibly disconnects and cleans up a session for the given entity.
         * No reconnection will be possible.
         * @param entity_id[in] The entity ID whose session is to be
         * disconnected and cleaned up.
         * @return True if session found and cleaned up, false if session
         * not found.
         */
        bool disconnect_session(const dbtype::Id &entity_id);

        /**
         * Adds or updates a session.  This is not normally used outside
         * this class.
         * @param connection_driver_ptr[in] Pointer to the connection driver.
         * The pointer will be kept.
         * @param connection_ptr[in] Pointer to the connection to add.
         * The pointer will be kept.
         * @param session_ptr[in] Pointer to the related session.
         * The pointer will be kept.
         * @param id[in] The entity ID associated with the connection.
         * @return True if found and updated, false if new and added.
         */
        bool add_session(
            ConnectionDriver *connection_driver_ptr,
            ClientConnection *connection_ptr,
            ClientSession *session_ptr,
            const dbtype::Id &id);

        /**
         * Given a connection that is not currently authorized, this is called
         * by a connection driver when an authorization is attempted.  This
         * will set up the session if successful.
         * The entity ID in connection_ptr will be updated if successful.
         * @param name[in] The name of the player to authorize.
         * @param password[in] The password of the player to authorize.
         * @param connection_driver_ptr[in] Pointer to the connection driver.
         * @param connection_ptr[in] Pointer to the client connection.  If
         * authorization is successful, the pointer is assumed to be held onto
         * by the Router and must be released when done.  If authorization is
         * not successful, the pointer is not held onto.
         * @return A valid pointer to the ClientSession if authorization was
         * successful, or a null pointer if error or wrong name/password.
         */
        ClientSession *authorize_client(
            const std::string &name,
            const std::string &password,
            ConnectionDriver *connection_driver_ptr,
            ClientConnection *connection_ptr);

        /**
         * Given a connection that is not currently authorized, this is called
         * by a connection driver when an authorization is attempted due to a
         * reconnect.  This will update the session if successful.
         * The entity ID in connection_ptr will be updated if successful.
         * @param name[in] The name of the player to authorize.
         * @param password[in] The password of the player to authorize.
         * @param connection_driver_ptr[in] Pointer to the connection driver.
         * @param connection_ptr[in] Pointer to the client connection.  If
         * authorization is successful, the pointer is assumed to be held onto
         * by the Router and must be released when done.  If authorization is
         * not successful, the pointer is not held onto.
         * @param make_new_if_not_found[in] If true, will create a new session
         * if not found during reauthorization (which will be returned
         * as a pointer).  If false (default), a
         * nonexisting session is an error.
         * @return A valid pointer to the existing (or new) ClientSession if
         * re-authorization was successful, or a null pointer if error
         * (such as no session found) or wrong name/password.
         */
        ClientSession *reauthorize_client(
            const std::string &name,
            const std::string &password,
            ConnectionDriver *connection_driver_ptr,
            ClientConnection *connection_ptr,
            const bool make_new_if_not_found = false);

        /**
         * Called by the connection driver when an authorized client has
         * disconnected due to external factors.  This will not be called if
         * the connection is disconnected via a method call.
         * @deprecated ClientConnection should call ClientSession directly.
         * @param connection_ptr[in] Pointer to the client connection.  It will
         * not be automatically released.
         */
        void client_disconnected(ClientConnection *connection_ptr);

        /**
         * Called by a connection driver when a client has acknowledged receipt
         * of data.
         * @deprecated Driver should call ClientSession directly.
         * @param connection_ptr[in] Pointer to the client connection.
         * @param ser_id[in] The highest serial number of the data being
         * acknowledged.
         */
        void data_acknowledge(
            ClientConnection *connection_ptr,
            const MessageSerialId ser_id);

        /**
         * Called by a connection driver when a client has reconnected and
         * wants to indicate the last message it received.
         * The Router sends it first after a successful reauthentication, which
         * prompts the client to send an equivalent back (this method).
         * @deprecated Driver should call ClientSession directly.
         * @param ser_id[in] The highest serial number of the data being
         * acknowledged.
         */
        void data_acknowledge_reconnect(
            ClientConnection *connection_ptr,
            const MessageSerialId ser_id);

        /**
         * Called by a connection driver when a client's connection is now
         * able to send more data out.
         * @deprecated Driver should call ClientSession directly.
         * @param connection_ptr[in] Pointer to the client connection.
         */
        void client_unblocked(ClientConnection *connection_ptr);

        /**
         * Called by a connection driver when text data has been received from
         * the client.
         * @deprecated Driver should call ClientSession directly.
         * @param connection_ptr[in] Pointer to the client connection.
         * @param channel_id[in] The ID of the channel the data has been
         * received on.
         * @param ser_id[in] The serial number of the text data, used for
         * ACK of receipt.
         * @param text_line_ptr[in] The data itself.  Control of the pointer
         * will pass to this method.
         */
        void data_from_client(
            ClientConnection *connection_ptr,
            const ChannelId channel_id,
            const MessageSerialId ser_id,
            text::ExternalTextLine *text_line_ptr);

        /**
         * Called by a connection driver when enhanced data has been received
         * from the client.
         * @deprecated Driver should call ClientSession directly.
         * @param connection_ptr[in] Pointer to the client connection.
         * @param channel_id[in] The ID of the channel the data has been
         * received on.
         * @param ser_id[in] The serial number of the enhanced data, used for
         * ACK of receipt.
         * @param client_message_ptr[in] The data itself.  Control of the
         * pointer will pass to this method.
         */
        void data_from_client(
            ClientConnection *connection_ptr,
            const ChannelId channel_id,
            const MessageSerialId ser_id,
            message::ClientMessage *client_message_ptr);

        /**
         * Called by a ClientSession to indicate it has data waiting to be
         * sent to the client or other actions to perform.
         * This can be called at any time, even if ClientSession is currently
         * in process_pending().  Only call it once before being serviced
         * again, or the caller will be serviced multiple times.
         * @param session_ptr[in] The session that needs process_pending() to
         * be called soon.
         */
        void session_has_pending_actions(ClientSession *session_ptr);

        /**
         * Called by ClientSession when it is done with a ClientConnection.
         * Note this may be called when a ClientSession is being destructed.
         * @param connection_ptr[in] The connection pointer to release (free).
         * When this call returns, the pointer must be considered invalid.
         */
        void release_connection(ClientConnection *connection_ptr);

    private:

        /**
         * Main loop to service connection drivers and sessions.
         */
        void thread_main(void);

        /**
         * Read locking is assumed to have already been performed.
         * @param session_ptr[in] The session pointer.
         * @return The associated connection pointer, or null if not found.
         */
        ClientConnection *get_connection(ClientSession *session_ptr);

        /**
         * Read locking is assumed to have already been performed.
         * @param connection_ptr[in] The connection pointer.
         * @return The associated session pointer, or null if not found.
         */
        ClientSession *get_session(ClientConnection *connection_ptr);

        /**
         * Calls any sessions needing to service pending operations.
         * No locks may be activated before calling this method; it will
         * automatically get the locks it needs.
         */
        void service_sessions(void);

        /**
         * Removes an existing session.  If the session is currently connected,
         * it will be disconnected and all Channels closed.  This is the only
         * way to delete/forcibly disconnect a session.
         * No reconnection will be possible.
         * No locking must be performed prior to calling this method.
         * @param session_ptr[in] The pointer to the session to remove.  After
         * this call returns, the pointer should be considered invalid.
         */
        void remove_session(ClientSession *session_ptr);

        /**
         * Finds the player given (full name only, no partial matches) and
         * confirms the password provided matches what's stored.
         * @param site_id[in] The site ID the player is associated with.
         * @param name[in] The full login (Entity) name of the player to be
         * checked.
         * @param password[in] The password to be checked.
         * @return The ID associated with the authenticated Player, or an
         * invalid ID if bad site ID, name, or password.
         */
        dbtype::Id check_password(
            const dbtype::Id::SiteIdType site_id,
            const std::string &name,
            const std::string &password);

        /**
         * Adds an ID and associated session pointer to the ID to session maps.
         * Assumes write locking has already taken place.
         * @param id[in] The ID associated with the session.
         * @param session_ptr[in] The session pointer to add or update for the
         * ID.
         */
        void add_entity_session(
            const dbtype::Id &id,
            ClientSession *session_ptr);

        /**
         * Assumes read locking has already taken place.
         * @param id[in] The Entity ID to get the session for.
         * @return The pointer to the session, or null if not found.
         */
        ClientSession *get_entity_session(const dbtype::Id &id) const;

        /**
         * Deletes a session added with add_entity_session().
         * Assumes write locking has already taken place.
         * @param id[in] The ID associated with the session to remove.
         * @return True if session found and removed.
         */
        bool remove_entity_session(const dbtype::Id &id);

        // No copying
        RouterSessionManager &operator=(const RouterSessionManager &rhs);
        RouterSessionManager(const RouterSessionManager &rhs);

        typedef std::pair<ConnectionDriver *, ClientSession *> DriverSession;
        typedef std::vector<ConnectionDriver *> ConnectionDrivers;
        typedef std::map<ClientConnection *, DriverSession> ConnectionSessionMap;
        typedef std::map<ClientSession *, ClientConnection *> SessionConnectionMap;

        typedef std::map<dbtype::Id, ClientSession *> EntitySessionMap;
        typedef std::map<dbtype::Id::SiteIdType, EntitySessionMap> SiteSessionsMap;

        typedef std::deque<ClientSession *> SessionQueue;
        typedef std::vector<ClientSession *> SessionVector;

        ConnectionDrivers connection_drivers; ///< Connection drivers to poll
        ConnectionSessionMap connection_to_session; ///< Maps connection pointer to session pointer
        SessionConnectionMap session_to_connection; ///< Maps session pointer to connection pointer
        SiteSessionsMap site_to_sessions; ///< Maps site ID to sessions for that site

        SessionQueue pending_actions; ///< Sessions that have pending actions, needing a callback

        boost::thread *thread_ptr; ///< Non-null when thread is running.

        boost::recursive_mutex callback_lock; ///< Lock for when calling back ClientSessions. Lock before router_lock if using.
        boost::recursive_mutex router_lock; ///< Lock for class instance
        boost::atomic<bool> shutdown_thread_flag; ///< True if thread should shutdown
    };
}
}

#endif //MUTGOS_COMM_ROUTER_H
