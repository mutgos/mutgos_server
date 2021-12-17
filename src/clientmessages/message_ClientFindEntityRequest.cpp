/*
 * message_ClientFindEntityRequest.cpp
 */

#include <string>

#include "dbtypes/dbtype_EntityType.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_ClientFindEntityRequest.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_MATCH_NAME_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_FIND_ENTITY_REQUEST,
            mutgos::message::ClientFindEntityRequest::make_instance);

    const static std::string SEARCH_STRING_KEY = "searchString";
    const static std::string EXACT_MATCH_KEY = "exactMatch";
    const static std::string ENTITY_TYPE_KEY = "entityType";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientFindEntityRequest::make_instance(void)
    {
        return new ClientFindEntityRequest();
    }

    // ----------------------------------------------------------------------
    ClientFindEntityRequest::ClientFindEntityRequest(void)
      : ClientMessage(CLIENTMESSAGE_FIND_ENTITY_REQUEST),
        exact_match(true),
        entity_type(dbtype::ENTITYTYPE_entity)
    {
    }

    // ----------------------------------------------------------------------
    ClientFindEntityRequest::ClientFindEntityRequest(
        const ClientFindEntityRequest &rhs)
        : ClientMessage(rhs),
          search_string(rhs.search_string),
          exact_match(rhs.exact_match),
          entity_type(rhs.entity_type)
    {
    }

    // ----------------------------------------------------------------------
    ClientFindEntityRequest::~ClientFindEntityRequest()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientFindEntityRequest::clone(void) const
    {
        return new ClientFindEntityRequest(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientFindEntityRequest::save(
        mutgos::json::JSONRoot &root,
        mutgos::json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        if (not search_string.empty())
        {
            // Save search string
            //
            success = json::add_static_key_value(
                SEARCH_STRING_KEY,
                search_string,
                node,
                root) and success;

            // Save exact match
            //
            success = json::add_static_key_value(
                EXACT_MATCH_KEY,
                exact_match,
                node,
                root) and success;
        }

        // Save Entity type
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
    bool ClientFindEntityRequest::restore(const mutgos::json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore search string
        //
        json::get_key_value(
            SEARCH_STRING_KEY,
            node,
            search_string);

        if (not search_string.empty())
        {
            // Restore exact match (required with search string)
            //
            success = json::get_key_value(
                EXACT_MATCH_KEY,
                node,
                exact_match) and success;
        }

        // Restore Entity type
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
