/*
 * message_ClientExecuteEntity.cpp
 */

#include <string>

#include "clientmessages/message_ClientExecuteEntity.h"
#include "clientmessages/message_ClientMessageType.h"
#include "clientmessages/message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

namespace
{
    // Static registration
    const bool CLIENT_EXECUTE_ENTITY_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_EXECUTE_ENTITY,
            mutgos::message::ClientExecuteEntity::make_instance);

    const static std::string ENTITY_ID_KEY = "entityId";
    const static std::string PROGRAM_ARGUMENTS_KEY = "programArguments";
    const static std::string CHANNEL_SUBTYPE_KEY = "channelSubtype";
}


namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *ClientExecuteEntity::make_instance(void)
    {
        return new ClientExecuteEntity();
    }

    // ----------------------------------------------------------------------
    ClientExecuteEntity::ClientExecuteEntity(void)
        : ClientMessage(CLIENTMESSAGE_EXECUTE_ENTITY)
    {
    }

    // ----------------------------------------------------------------------
    ClientExecuteEntity::ClientExecuteEntity(const ClientExecuteEntity &rhs)
        : ClientMessage(rhs),
          entity_id(rhs.entity_id),
          program_arguments(rhs.program_arguments),
          channel_subtype(rhs.channel_subtype)
    {
    }

    // ----------------------------------------------------------------------
    ClientExecuteEntity::~ClientExecuteEntity()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientExecuteEntity::clone() const
    {
        return new ClientExecuteEntity(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientExecuteEntity::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        // Save ID (required)
        //
        JSON_MAKE_MAP_NODE(id_node);
        success = entity_id.save(root, id_node) and success;

        if (success)
        {
            success = json::add_static_key_value(
                ENTITY_ID_KEY,
                id_node,
                node,
                root) and success;
        }

        // Save arguments (if any)
        //
        if (not program_arguments.empty())
        {
            JSON_MAKE_ARRAY_NODE(arg_array);

            success = json::array_add_value(
                program_arguments,
                arg_array,
                root) and success;

            if (success)
            {
                success = json::add_static_key_value(
                    PROGRAM_ARGUMENTS_KEY,
                    arg_array,
                    node,
                    root) and success;
            }
        }

        // Save channel name (if set)
        //
        if (not channel_subtype.empty())
        {
            success = json::add_static_key_value(
                CHANNEL_SUBTYPE_KEY,
                channel_subtype,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientExecuteEntity::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore ID (required).
        //
        const json::JSONNode *id_node = 0;

        success = json::get_key_value(ENTITY_ID_KEY, node, id_node) and success;

        if (success)
        {
            success = entity_id.restore(*id_node) and success;
        }

        // Restore program arguments if found
        //
        program_arguments.clear();

        const json::JSONNode *arg_node = 0;

        json::get_key_value(PROGRAM_ARGUMENTS_KEY, node, arg_node);

        if (arg_node)
        {
            success = json::array_get_value(*arg_node, program_arguments)
                and success;
        }

        // Restore channel subtype if found
        //
        channel_subtype.clear();
        json::get_key_value(CHANNEL_SUBTYPE_KEY, node, channel_subtype);

        return success;
    }
}
}