/*
 * message_ClientDisconnect.cpp
 */


#include "clientmessages/message_ClientMessage.h"

#include "message_ClientDisconnect.h"

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientDisconnect::ClientDisconnect(void)
      : ClientMessage(CLIENTMESSAGE_DISCONNECT)
    {
    }

    // ----------------------------------------------------------------------
    ClientDisconnect::ClientDisconnect(const ClientDisconnect &rhs)
      : ClientMessage(rhs)
    {
    }

    // ----------------------------------------------------------------------
    ClientDisconnect::~ClientDisconnect()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientDisconnect::make_instance(void)
    {
        return new ClientDisconnect();
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientDisconnect::clone(void) const
    {
        return new ClientDisconnect(*this);
    }
}
}