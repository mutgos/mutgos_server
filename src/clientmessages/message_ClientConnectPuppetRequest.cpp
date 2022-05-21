/*
 * message_ClientConnectPuppetRequest.cpp
 */

#include "message_ClientConnectPuppetRequest.h"
#include "clientmessages/message_ClientMessageType.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_CONNECT_PUPPET_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_CONNECT_PUPPET_REQUEST,
            mutgos::message::ClientConnectPuppetRequest::make_instance);

    const static std::string PUPPET_ENTITY_ID_KEY = "puppetEntityId";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientConnectPuppetRequest::make_instance(void)
    {
        return new ClientConnectPuppetRequest();
    }

    // ----------------------------------------------------------------------
    ClientConnectPuppetRequest::ClientConnectPuppetRequest(void)
        : ClientMessage(CLIENTMESSAGE_CONNECT_PUPPET_REQUEST)
    {
    }

    // ----------------------------------------------------------------------
    ClientConnectPuppetRequest::ClientConnectPuppetRequest(
        const ClientConnectPuppetRequest &rhs)
        : ClientMessage(rhs),
          puppet_entity_id(rhs.puppet_entity_id)
    {
    }

    // ----------------------------------------------------------------------
    ClientConnectPuppetRequest::~ClientConnectPuppetRequest()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientConnectPuppetRequest::clone() const
    {
        return new ClientConnectPuppetRequest(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientConnectPuppetRequest::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        JSON_MAKE_ARRAY_NODE(id_node);

        if (success)
        {
            success = puppet_entity_id.save(root, id_node) and success;

            success = json::add_static_key_value(
                PUPPET_ENTITY_ID_KEY,
                id_node,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientConnectPuppetRequest::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore ID (required).
        //
        const json::JSONNode *id_node = 0;

        success = json::get_key_value(
            PUPPET_ENTITY_ID_KEY,
            node,
            id_node) and success;

        if (success)
        {
            success = puppet_entity_id.restore(*id_node) and success;
        }

        return success;
    }

}
}