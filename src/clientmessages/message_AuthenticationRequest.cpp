/*
 * message_AuthenticationRequest.cpp
 */

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

#include "clientmessages/message_ClientMessage.h"
#include "message_AuthenticationRequest.h"

namespace
{
    // Static registration
    const bool CLIENT_AUTHENTICATION_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_AUTHENTICATION_REQUEST,
            mutgos::message::AuthenticationRequest::make_instance);

    const static std::string PLAYER_NAME_KEY = "name";
    const static std::string PLAYER_PASSWORD_KEY = "password";
    const static std::string PLAYER_SITE_ID_KEY = "site";
    const static std::string PLAYER_RECONNECT_KEY = "isReconnect";
    const static std::string WINDOW_SIZE_KEY = "windowSize";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    AuthenticationRequest::AuthenticationRequest(void)
      : ClientMessage(CLIENTMESSAGE_AUTHENTICATION_REQUEST),
        player_site_id(0),
        player_reconnect(false),
        window_size(0)
    {
    }

    // ----------------------------------------------------------------------
    AuthenticationRequest::AuthenticationRequest(
        const AuthenticationRequest &rhs)
      : ClientMessage(rhs),
        player_name(rhs.player_name),
        player_password(rhs.player_password),
        player_site_id(rhs.player_site_id),
        player_reconnect(rhs.player_reconnect),
        window_size(rhs.window_size)
    {
    }

    // ----------------------------------------------------------------------
    AuthenticationRequest::~AuthenticationRequest()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *AuthenticationRequest::make_instance(void)
    {
        return new AuthenticationRequest();
    }

    // ----------------------------------------------------------------------
    ClientMessage *AuthenticationRequest::clone(void) const
    {
        return new AuthenticationRequest(*this);
    }

    // ----------------------------------------------------------------------
    bool AuthenticationRequest::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        return false;
    }

    // ----------------------------------------------------------------------
    bool AuthenticationRequest::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        success = json::get_key_value(
            PLAYER_NAME_KEY,
            node,
            player_name) and success;

        success = json::get_key_value(
            PLAYER_PASSWORD_KEY,
            node,
            player_password) and success;

        success = json::get_key_value(
            PLAYER_SITE_ID_KEY,
            node,
            player_site_id) and success;

        success = json::get_key_value(
            PLAYER_RECONNECT_KEY,
            node,
            player_reconnect) and success;

        success = json::get_key_value(
            WINDOW_SIZE_KEY,
            node,
            window_size) and success;

        return success;
    }
}
}
