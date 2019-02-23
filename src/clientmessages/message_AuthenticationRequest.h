/*
 * message_AuthenticationRequest.h
 */

#ifndef MUTGOS_MESSAGE_AUTHENTICATIONREQUEST_H
#define MUTGOS_MESSAGE_AUTHENTICATIONREQUEST_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"

#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to authenticate.
     * This is only sent from the client to the server.
     */
    class AuthenticationRequest : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        AuthenticationRequest(void);

        /**
         * Copy constructor.
         * @param ser_id[in] The source to copy from.
         */
        AuthenticationRequest(const AuthenticationRequest &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~AuthenticationRequest();

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * Sets the player name.
         * @param name[in] The player name to authenticate.
         */
        void set_player_name(const std::string &name)
          { player_name = name; }

        /**
         * @return The player name to authenticate.
         */
        const std::string &get_player_name(void) const
          { return player_name; }

        /**
         * Sets the player password.
         * @param password[in] The player password to authenticate.
         */
        void set_player_password(const std::string &password)
          { player_password = password; }

        /**
         * @return The player password to authenticate.
         */
        const std::string &get_player_password(void) const
          { return player_password; }

        /**
         * Sets the site ID associated with the player being authenticated.
         * @param site_id[in] The site ID of the player being authenticated.
         */
        void set_player_site_id(const dbtype::Id::SiteIdType site_id)
          { player_site_id = site_id; }

        /**
         * @return The site ID of the player being authenticated.
         */
        dbtype::Id::SiteIdType get_player_site_id(void) const
          { return player_site_id; }

        /**
         * Sets the reconnect flag, indicating if the authentication attempt
         * is to reconnect to an existing session.
         * @param reconnect[in] True if reconnecting.
         */
        void set_reconnect_flag(const bool reconnect)
          { player_reconnect = reconnect; }

        /**
         * @return True if reconnecting.
         */
        bool get_reconnect_flag(void) const
          { return player_reconnect; }

        /**
         * Sets the window size.
         * @param size The send/receive window size, measured in messages
         * (not bytes).
         */
        void set_window_size(const MG_UnsignedInt size)
          { window_size = size; }

        /**
         * @return The send/receive window size, measured in messages
         * (not bytes).
         */
        MG_UnsignedInt get_window_size(void) const
          { return window_size; }

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return False, since not currently supported.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        std::string player_name; ///< Name of player connecting.
        std::string player_password; ///< Password of player connecting.
        dbtype::Id::SiteIdType player_site_id; ///< Site connecting to
        bool player_reconnect; ///< True if this is a reconnect attempt.
        MG_UnsignedInt window_size; ///< Send/recv window size, in message counts
    };
}
}

#endif //MUTGOS_MESSAGE_AUTHENTICATIONREQUEST_H
