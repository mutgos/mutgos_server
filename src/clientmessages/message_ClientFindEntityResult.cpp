/*
 * message_ClientFindEntityResult.cpp
 */

#include <string>
#include <vector>

#include "dbtypes/dbtype_Id.h"

#include "clientmessages/message_ClientMessage.h"
#include "clientmessages/message_ClientFindEntityResult.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_MATCH_NAME_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_FIND_ENTITY_RESULT,
            mutgos::message::ClientFindEntityResult::make_instance);

    const static std::string MATCHING_ENTITIES_KEY = "result";
    const static std::string MATCHING_ENTITIES_ID_KEY = "id";
    const static std::string MATCHING_ENTITIES_NAME_KEY = "name";
    const static std::string MATCHING_ENTITIES_TYPE_KEY = "type";
    const static std::string SECURITY_VIOLATION_KEY = "securityViolation";
    const static std::string AMBIGUOUS_KEY = "ambiguous";
    const static std::string ERROR_KEY = "error";
    const static std::string ERROR_MESSAGE_KEY = "errorMessage";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientFindEntityResult::make_instance(void)
    {
        return new ClientFindEntityResult();
    }

    // ----------------------------------------------------------------------
    ClientFindEntityResult::ClientFindEntityResult(void)
        : ClientMessage(CLIENTMESSAGE_FIND_ENTITY_RESULT),
          security_violation(false),
          ambiguous(false),
          error(false)
    {
    }

    // ----------------------------------------------------------------------
    ClientFindEntityResult::ClientFindEntityResult(
        const ClientFindEntityResult &rhs)
        : ClientMessage(rhs),
          result(rhs.result),
          security_violation(rhs.security_violation),
          ambiguous(rhs.ambiguous),
          error(rhs.error),
          error_message(rhs.error_message)
    {
    }

    // ----------------------------------------------------------------------
    ClientFindEntityResult::~ClientFindEntityResult()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientFindEntityResult::clone(void) const
    {
        return new ClientFindEntityResult(*this);
    }

    // ----------------------------------------------------------------------
    ClientFindEntityResult::FoundEntity
    ClientFindEntityResult::get_first_entity(void) const
    {
        if (not result.empty())
        {
            return result.front();
        }
        else
        {
            return FoundEntity();
        }
    }

    // ----------------------------------------------------------------------
    bool ClientFindEntityResult::save(
        mutgos::json::JSONRoot &root,
        mutgos::json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        JSON_MAKE_ARRAY_NODE(entities_array);

        for (FoundEntities::const_iterator iter = result.begin();
            iter != result.end();
            ++iter)
        {
            const FoundEntity &entityTuple = *iter;

            JSON_MAKE_MAP_NODE(entity);

            // ID
            //
            JSON_MAKE_MAP_NODE(id_node);
            success = std::get<0>(entityTuple).save(root, id_node) and success;
            success = json::add_static_key_value(
                MATCHING_ENTITIES_ID_KEY,
                id_node,
                entity,
                root) and success;

            // Name
            //
            success = json::add_static_key_value(
                MATCHING_ENTITIES_NAME_KEY,
                std::get<1>(entityTuple),
                entity,
                root) and success;

            // Type
            //
            success = json::add_static_key_value(
                MATCHING_ENTITIES_TYPE_KEY,
                dbtype::entity_type_to_string(std::get<2>(entityTuple)),
                entity,
                root) and success;

            // Append to array
            //
            success = json::array_add_node(entity, entities_array, root)
                and success;
        }

        success = json::add_static_key_value(
            MATCHING_ENTITIES_KEY,
            entities_array,
            node,
            root) and success;

        // Save security violation (required)
        //
        success = json::add_static_key_value(
            SECURITY_VIOLATION_KEY,
            security_violation,
            node,
            root) and success;


        // Save ambiguous (required)
        //
        success = json::add_static_key_value(
            AMBIGUOUS_KEY,
            ambiguous,
            node,
            root) and success;

        // Save error (required)
        //
        success = json::add_static_key_value(
            ERROR_KEY,
            error,
            node,
            root) and success;

        // Save error message if error
        //
        if (error)
        {
            success = json::add_static_key_value(
                ERROR_MESSAGE_KEY,
                error_message,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientFindEntityResult::restore(const json::JSONNode &node)
    {
        return false;
    }
}
}