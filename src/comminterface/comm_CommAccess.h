 /*
 * comm_CommAccess.h
 */

#ifndef MUTGOS_SERVER_COMM_COMMACCESS_H
#define MUTGOS_SERVER_COMM_COMMACCESS_H

#include "osinterface/osinterface_OsTypes.h"

#include "comm_RouterSessionManager.h"
#include "comm_SessionStats.h"

#include "dbtypes/dbtype_Entity.h"

namespace mutgos
{
namespace comm
{
    /**
     * Other namespaces can use this interface to interact with the external
     * communications (comm) subsystem.  Most of comm runs on its own thread
     * and very little needs to be done via this class, since the primary
     * interaction in and out of the server is via Channels.
     */
    class CommAccess
    {
        // TODO Command Interpreter will need a 'paste' mode to facilitate paste-building because channels screw that up.
        // TODO Command Interpreter will likely need to block after each command

        /**
         * Common code to avoid data loss on reconnect:
         * Can ask for list of open channels, and ask for them to be inserted in front of queue for connection
         * Can indicate item's receipt confirmed by client, to pop off queue for client
         * Can ask for all sent data to be placed back in queue for client, to be resent
         * Each item has an ID (eventually loops back, unsigned int), an a flag indicating if it has ever been sent
         *
         * Comm packages implement listener that gets called back when any new message on channel is to be sent.
         *      Listener can indicate if it accepted the message or if blocked
         *
         * Comm package, when not calling select, enables callback.
         *
         * Reverse for getting messages from client.  Comm package calls listener, which can accept or defer
         *
         * Channels are all put into one queue per client, because they could be opened and closed while the client is disconnected.
         *      Will need to keept track of currently open channels as of the start of queue, in case switching machines
         * If switching type of client (enhanced vs non-enhanced), only base channel(s) are kept.  All others are closed - but pending data for text channels will be sent.
         *
         *
         * The common area that each comm type (ws, socket, etc) utilize will do stuff like this:
         *      Heartbeat each one at time so they can call select(), manage connections, etc.
         *      Send them data when available
         */



    public:
        typedef RouterSessionManager::SessionStatsVector SessionStatsVector;

        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static CommAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static CommAccess *instance(void)
        { return singleton_ptr;  }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * Not thread safe.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Not thread safe.
         */
        void shutdown(void);


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
         * @return The site IDs that currently have connections.
         */
        dbtype::Id::SiteIdVector get_entity_site_ids(void);

        /**
         * @param site_id[in] The site ID to check.
         * @return The number currently online at the site, or 0 if none or
         * site not found.
         */
        MG_UnsignedInt get_session_online_count(
            const dbtype::Id::SiteIdType site_id);

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
         * @param entity_id[in] Entity ID to get session stats for.
         * @return Information about the session associated with the
         * entity ID, or invalid stats if the entity doesn't exist or is not
         * currently connected.
         */
        SessionStats get_session_stats(const dbtype::Id &entity_id);

    private:

        /**
         * Private singleton constructor.
         */
        CommAccess(void);

        /**
         * Private singleton destructor.
         */
        ~CommAccess();

        /**
         * Adds all comm modules to the router.  Used before starting
         * the router.
         * @return Success of all modules instantiated and added.
         */
        bool add_comm_modules(void);

        static CommAccess *singleton_ptr; ///< Pointer to singleton instance
        RouterSessionManager router; ///< Router and session manager for all connections
    };
}
}

#endif //MUTGOS_SERVER_COMM_COMMACCESS_H
