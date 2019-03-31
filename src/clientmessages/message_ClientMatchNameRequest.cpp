/*
 * message_ClientMatchNameRequest.cpp
 */

#include <string>

#include "dbtypes/dbtype_EntityType.h"

#include "clientmessages/message_ClientMessage.h"
#include "message_ClientMatchNameRequest.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_MATCH_NAME_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_MATCH_NAME_REQUEST,
            mutgos::message::ClientMatchNameRequest::make_instance);

    const static std::string SEARCH_STRING_KEY = "searchString";
    const static std::string EXACT_MATCH_KEY = "exactMatch";
    const static std::string ENTITY_TYPE_KEY = "entityType";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientMatchNameRequest::make_instance(void)
    {
        return new ClientMatchNameRequest();
    }

    // ----------------------------------------------------------------------
    ClientMatchNameRequest::ClientMatchNameRequest(void)
      : ClientMessage(CLIENTMESSAGE_MATCH_NAME_REQUEST),
        exact_match(true),
        entity_type(dbtype::ENTITYTYPE_entity)
    {
    }

    // ----------------------------------------------------------------------
    ClientMatchNameRequest::ClientMatchNameRequest(
        const ClientMatchNameRequest &rhs)
        : ClientMessage(rhs),
          search_string(rhs.search_string),
          exact_match(rhs.exact_match),
          entity_type(rhs.entity_type)
    {
    }

    // ----------------------------------------------------------------------
    ClientMatchNameRequest::~ClientMatchNameRequest()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientMatchNameRequest::clone(void) const
    {
        return new ClientMatchNameRequest(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientMatchNameRequest::save(
        mutgos::json::JSONRoot &root,
        mutgos::json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        // Save search string (required)
        //
        success = json::add_static_key_value(
            SEARCH_STRING_KEY,
            search_string,
            node,
            root) and success;

        // Save exact match (required)
        //
        success = json::add_static_key_value(
            EXACT_MATCH_KEY,
            exact_match,
            node,
            root) and success;

        // Save Entity type (required)
        //
        const std::string &type_to_string =
            dbtype::entity_type_to_string(entity_type);

        success = json::add_static_key_value(
            ENTITY_TYPE_KEY,
            type_to_string,
            node,
            root) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientMatchNameRequest::restore(const mutgos::json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore search string (required)
        //
        success = json::get_key_value(
            SEARCH_STRING_KEY,
            node,
            search_string) and success;

        // Restore exact match (required)
        //
        success = json::get_key_value(
            EXACT_MATCH_KEY,
            node,
            exact_match) and success;

        // Restore Entity type (required)
        //
        std::string type_to_string;

        success = json::get_key_value(
            ENTITY_TYPE_KEY,
            node,
            type_to_string) and success;

        if (success)
        {
            entity_type = dbtype::string_to_entity_type(type_to_string);
            success = (entity_type != dbtype::ENTITYTYPE_invalid);
        }

        return success;
    }
}
}
