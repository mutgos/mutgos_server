/*
 *  message_LocationInfoChange.cpp
 */

#include <string>

#include "dbtypes/dbtype_Id.h"
#include "clientmessages/message_ClientMessage.h"
#include "message_MessageFactory.h"
#include "utilities/json_JsonUtilities.h"

#include "message_LocationInfoChange.h"

namespace
{
    // Static registration
    const bool CLIENT_LOCATION_INFO_CHANGE_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_LOCATION_INFO_CHANGE,
            mutgos::message::LocationInfoChange::make_instance);

    const static std::string NEW_ROOM_ID_KEY = "newRoomId";
    const static std::string NEW_ROOM_NAME_KEY = "newRoomName";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientMessage *LocationInfoChange::make_instance(void)
    {
        return new LocationInfoChange();
    }

    // ----------------------------------------------------------------------
    LocationInfoChange::LocationInfoChange(void)
      : ClientMessage(CLIENTMESSAGE_LOCATION_INFO_CHANGE)
    {
    }

    // ----------------------------------------------------------------------
    LocationInfoChange::LocationInfoChange(
        const mutgos::message::LocationInfoChange &rhs)
        : ClientMessage(rhs),
          new_room_id(rhs.new_room_id),
          new_room_name(rhs.new_room_name)
    {
    }

    // ----------------------------------------------------------------------
    LocationInfoChange::~LocationInfoChange()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *LocationInfoChange::clone(void) const
    {
        return new LocationInfoChange(*this);
    }

    // ----------------------------------------------------------------------
    bool LocationInfoChange::save(
        mutgos::json::JSONRoot &root,
        mutgos::json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        // Save new room ID if populated.
        //
        if (not new_room_id.is_default())
        {
            JSON_MAKE_MAP_NODE(id_node);
            success = new_room_id.save(root, id_node) and success;

            if (success)
            {
                success = json::add_static_key_value(
                    NEW_ROOM_ID_KEY,
                    id_node,
                    node,
                    root) and success;
            }
        }

        // Save new room name if populated.
        //
        if (not new_room_name.empty())
        {
            success = json::add_static_key_value(
                NEW_ROOM_NAME_KEY,
                new_room_name,
                node,
                root) and success;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool LocationInfoChange::restore(const mutgos::json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        // Restore new room ID if present.
        //
        const json::JSONNode *id_node = 0;

        if (json::get_key_value(NEW_ROOM_ID_KEY, node, id_node))
        {
            success = new_room_id.restore(*id_node) and success;
        }

        // Restore the new room name if present.
        //
        json::get_key_value(NEW_ROOM_NAME_KEY, node, new_room_name);

        return success;
    }
}
}
