/*
 * message_ClientAuthenticationResult.cpp
 */

#include <string>

#include "message_ClientMessage.h"
#include "message_ClientMessageType.h"
#include "message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

#include "message_ClientAuthenticationResult.h"

namespace
{
    const static std::string AUTHENTICATION_RESULT_KEY =
        "authenticationResult";
    const static std::string NEGOTIATION_RESULT_KEY =
        "negotiationResult";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientAuthenticationResult::ClientAuthenticationResult(void)
      : ClientMessage(CLIENTMESSAGE_AUTHENTICATION_RESULT),
        authentication_result(false),
        negotiation_result(false)
    {
    }

    // ----------------------------------------------------------------------
    ClientAuthenticationResult::ClientAuthenticationResult(
        const bool auth_result,
        const bool param_result)
      : ClientMessage(CLIENTMESSAGE_AUTHENTICATION_RESULT),
        authentication_result(auth_result),
        negotiation_result(param_result)
    {
    }

    // ----------------------------------------------------------------------
    ClientAuthenticationResult::ClientAuthenticationResult(
        const ClientAuthenticationResult &rhs)
      : ClientMessage(rhs),
        authentication_result(rhs.authentication_result),
        negotiation_result(rhs.negotiation_result)
    {
    }

    // ----------------------------------------------------------------------
    ClientAuthenticationResult::~ClientAuthenticationResult()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientAuthenticationResult::make_instance(void)
    {
        return new ClientAuthenticationResult();
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientAuthenticationResult::clone(void) const
    {
        return new ClientAuthenticationResult(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientAuthenticationResult::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        success = json::add_static_key_value(
            AUTHENTICATION_RESULT_KEY,
            authentication_result,
            node,
            root) and success;

        success = json::add_static_key_value(
            NEGOTIATION_RESULT_KEY,
            negotiation_result,
            node,
            root) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientAuthenticationResult::restore(const json::JSONNode &node)
    {
        return false;
    }
}
}
