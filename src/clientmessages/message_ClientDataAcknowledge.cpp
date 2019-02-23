#include <string>

#include "comminterface/comm_CommonTypes.h"
#include "message_ClientMessageType.h"
#include "message_MessageFactory.h"

#include "utilities/json_JsonUtilities.h"

#include "message_ClientDataAcknowledge.h"

namespace
{
    // Static registration
    const bool CLIENT_DATA_ACKNOWLEDGE_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_DATA_ACKNOWLEDGE,
            mutgos::message::ClientDataAcknowledge::make_instance);

    const static std::string MESSAGE_SER_ID_KEY = "messageSerId";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientDataAcknowledge::ClientDataAcknowledge(void)
        : ClientMessage(CLIENTMESSAGE_DATA_ACKNOWLEDGE),
          message_ser_id(0)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledge::ClientDataAcknowledge(
        const comm::MessageSerialId ser_id)
        : ClientMessage(CLIENTMESSAGE_DATA_ACKNOWLEDGE),
          message_ser_id(ser_id)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledge::ClientDataAcknowledge(
        const ClientDataAcknowledge &rhs)
      : ClientMessage(rhs),
        message_ser_id(rhs.message_ser_id)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledge::~ClientDataAcknowledge()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientDataAcknowledge::make_instance(void)
    {
        return new ClientDataAcknowledge();
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientDataAcknowledge::clone(void) const
    {
        return new ClientDataAcknowledge(*this);
    }

    // ----------------------------------------------------------------------
    bool ClientDataAcknowledge::save(
        json::JSONRoot &root,
        json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        success = json::add_static_key_value(
            MESSAGE_SER_ID_KEY,
            message_ser_id,
            node,
            root) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientDataAcknowledge::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        success = json::get_key_value(
            MESSAGE_SER_ID_KEY,
            node,
            message_ser_id) and success;

        return success;
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledge::ClientDataAcknowledge(const ClientMessageType type)
        : ClientMessage(type),
          message_ser_id(0)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledge::ClientDataAcknowledge(
        const ClientMessageType type,
        const comm::MessageSerialId ser_id)
        : ClientMessage(type),
          message_ser_id(ser_id)
    {
    }
}
}
