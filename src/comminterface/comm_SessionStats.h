/*
 * comm_SessionStats.h
 */

#ifndef MUTGOS_COMM_SESSIONSTATS_H
#define MUTGOS_COMM_SESSIONSTATS_H

#include <string>
#include <vector>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"

#include "comminterface/comm_ClientConnection.h"

namespace mutgos
{
namespace comm
{
    // Simple container class that has aggregated information about a
    // particular session, partly for display to a user but also for
    // other uses too.
    //
    class SessionStats
    {
    public:
        /**
         * Constructor that sets everything.
         * @param id[in] The entity ID associated with the connection.
         * @param is_connected[in] True if currently connected, false if
         * waiting for a reconnect.
         * @param when_connected[in] What time the initial connection was made.
         * @param last_activity[in] The last time any activity came from the
         * client.
         * @param is_enhanced[in] True if client can accept more than plain
         * text, false if plain text only.
         * @param source[in] Where the connection originates from (IP address,
         * hostname, etc).
         * @param type[in] What type of connection this is (batch, etc).
         */
        SessionStats(
            const dbtype::Id &id,
            const bool is_connected,
            const dbtype::TimeStamp &when_connected,
            const dbtype::TimeStamp &last_activity,
            const bool is_enhanced,
            const std::string &source,
            const ClientConnection::ClientType type)
          : entity_id(id),
            connected(is_connected),
            connection_time(when_connected),
            last_activity_time(last_activity),
            connection_is_enhanced(is_enhanced),
            connection_source(source),
            client_type(type)
          { }

        /**
         * Default constructor for STL, etc.
         */
        SessionStats(void)
            : connected(false),
              connection_is_enhanced(false),
              client_type(ClientConnection::CLIENT_TYPE_BATCH)
        { }

        /**
         * Destructor.
         */
        ~SessionStats()
          { }

        /**
         * @return The entity ID associated with the connection.
         */
        const dbtype::Id &get_entity_id(void) const
          { return entity_id; }

        /**
         * @return True if currently connected, false if waiting for a
         * reconnect.
         */
        bool is_connected(void) const
          { return connected; }

        /**
         * @return What time the initial connection was made.
         */
        const dbtype::TimeStamp &get_connection_time(void) const
          { return connection_time; }

        /**
         * @return The last time any activity came from the client.
         */
        const dbtype::TimeStamp &get_last_activity_time(void) const
          { return last_activity_time; }

        /**
         * @return True if client can accept more than plain text, false if
         * plain text only.
         */
        bool is_enhanced(void) const
          { return connection_is_enhanced; }

        /**
         * @return Where the connection originates from (IP address,
         * hostname, etc).
         */
        const std::string &get_connection_source(void) const
          { return connection_source; }

        /**
         * @return What type of connection this is (batch, etc).
         */
        ClientConnection::ClientType get_client_type(void) const
          { return client_type; }

    private:
        dbtype::Id entity_id;  ///< Entity ID associated with the connection.
        bool connected; ///< True if currently connected, false if waiting for reconnect
        dbtype::TimeStamp connection_time; ///< When the client connected
        dbtype::TimeStamp last_activity_time; ///< Last time the user did something on the connection
        bool connection_is_enhanced; ///< True if enhanced, false if text only
        std::string connection_source; ///< Where the connection originates from
        ClientConnection::ClientType client_type; ///< Determines if client is interactive, batch, etc
    };
}
}

#endif //MUTGOS_COMM_SESSIONSTATS_H
