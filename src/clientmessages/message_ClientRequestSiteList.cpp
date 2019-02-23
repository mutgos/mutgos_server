/*
 * message_ClientRequestSiteList.cpp
 */

#include "clientmessages/message_MessageFactory.h"

#include "clientmessages/message_ClientRequestSiteList.h"

namespace
{
    // Static registration
    const bool CLIENT_REQUEST_SITE_LIST_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_REQUEST_SITE_LIST,
            mutgos::message::ClientRequestSiteList::make_instance);
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientRequestSiteList::ClientRequestSiteList(void)
      : ClientMessage(CLIENTMESSAGE_REQUEST_SITE_LIST)
    {
    }

    // ----------------------------------------------------------------------
    ClientRequestSiteList::ClientRequestSiteList(
        const ClientRequestSiteList &rhs)
      : ClientMessage(rhs)
    {
    }

    // ----------------------------------------------------------------------
    ClientRequestSiteList::~ClientRequestSiteList()
    {
    }

    // ----------------------------------------------------------------------
    ClientMessage *ClientRequestSiteList::make_instance(void)
    {
        return new ClientRequestSiteList();
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientRequestSiteList::clone(void) const
    {
        return new ClientRequestSiteList(*this);
    }
}
}
