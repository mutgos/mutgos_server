/*
 * message_ClientDataAcknowledgeReconnect.cpp
 */

#include "message_MessageFactory.h"

#include "message_ClientDataAcknowledge.h"
#include "message_ClientDataAcknowledgeReconnect.h"

namespace
{
    // Static registration
    const bool CLIENT_DATA_ACKNOWLEDGE_RECONNECT_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_DATA_ACKNOWLEDGE_RECONNECT,
            mutgos::message::ClientDataAcknowledgeReconnect::make_instance);
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientDataAcknowledgeReconnect::ClientDataAcknowledgeReconnect(void)
      : ClientDataAcknowledge(CLIENTMESSAGE_DATA_ACKNOWLEDGE_RECONNECT)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledgeReconnect::ClientDataAcknowledgeReconnect(
        const comm::MessageSerialId ser_id)
      : ClientDataAcknowledge(CLIENTMESSAGE_DATA_ACKNOWLEDGE_RECONNECT, ser_id)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledgeReconnect::ClientDataAcknowledgeReconnect(
        const ClientDataAcknowledgeReconnect &rhs)
      : ClientDataAcknowledge(rhs)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataAcknowledgeReconnect::~ClientDataAcknowledgeReconnect()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientDataAcknowledgeReconnect::make_instance(void)
    {
        return new ClientDataAcknowledgeReconnect();
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientDataAcknowledgeReconnect::clone(void) const
    {
        return new ClientDataAcknowledgeReconnect(*this);
    }
}
}