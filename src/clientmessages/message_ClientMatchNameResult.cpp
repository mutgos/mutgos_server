/*
 * message_ClientMatchNameResult.cpp
 */

#include <string>
#include <vector>

#include "dbtypes/dbtype_Id.h"

#include "clientmessages/message_ClientMessage.h"
#include "message_ClientMatchNameResult.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_MATCH_NAME_REQUEST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_MATCH_NAME_RESULT,
            mutgos::message::ClientMatchNameResult::make_instance);

    const static std::string MATCHING_IDS_KEY = "matchingIds";
    const static std::string SECURITY_VIOLATION_KEY = "securityViolation";
    const static std::string AMBIGUOUS_KEY = "ambiguous";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientMatchNameResult::make_instance(void)
    {
        return new ClientMatchNameResult();
    }

    // ----------------------------------------------------------------------
    ClientMatchNameResult::ClientMatchNameResult(void)
        : ClientMessage(CLIENTMESSAGE_MATCH_NAME_RESULT),
          security_violation(false),
          ambiguous(false)
    {
    }

    // ----------------------------------------------------------------------
    ClientMatchNameResult::ClientMatchNameResult(
        const ClientMatchNameResult &rhs)
        : ClientMessage(rhs),
          matching_ids(rhs.matching_ids),
          security_violation(rhs.security_violation),
          ambiguous(rhs.ambiguous)
    {
    }

    // ----------------------------------------------------------------------
    ClientMatchNameResult::~ClientMatchNameResult()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientMatchNameResult::clone(void) const
    {
        return new ClientMatchNameResult(*this);
    }

    // ----------------------------------------------------------------------
    dbtype::Id ClientMatchNameResult::get_matching_id(void) const
    {
        if (not matching_ids.empty())
        {
            return matching_ids.front();
        }
        else
        {
            return dbtype::Id();
        }
    }

    // ----------------------------------------------------------------------
    bool ClientMatchNameResult::save(
        mutgos::json::JSONRoot &root,
        mutgos::json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        JSON_MAKE_ARRAY_NODE(matching_array);

        for (MatchingIds::const_iterator iter = matching_ids.begin();
            iter != matching_ids.end();
            ++iter)
        {
            JSON_MAKE_MAP_NODE(id_node);
            success = iter->save(root, id_node) and success;
        }

        success = json::add_static_key_value(
            MATCHING_IDS_KEY,
            matching_array,
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

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientMatchNameResult::restore(const json::JSONNode &node)
    {
        return false;
    }
}
}