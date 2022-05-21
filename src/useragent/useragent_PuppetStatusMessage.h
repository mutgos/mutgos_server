/*
 * useragent_PuppetStatusMessage.h
 */

#ifndef MUTGOS_USERAGENT_PUPPETSTATUSMESSAGE_H
#define MUTGOS_USERAGENT_PUPPETSTATUSMESSAGE_H

#include <string>

#include "executor/executor_ProcessMessage.h"
#include "dbtypes/dbtype_Id.h"

namespace
{
const std::string PUPPET_STATUS_MESSAGE_SUBTYPE = "PuppetStatusMessage";
}


namespace mutgos
{
namespace useragent
{
/**
 * Represents a status from a puppet agent.
 * Sent from the puppet agent to the puppet manager.
 */
class PuppetStatusMessage : public executor::ProcessMessage
{
public:
    /**
     * @return Subtype of this message for identification purposes.
     */
    static const std::string &message_subtype(void)
      { return ::PUPPET_STATUS_MESSAGE_SUBTYPE;}

    /**
     * Creates an interprocess puppet status message.
     * Currently the only use for this message is to indicate the agent
     * has exited.
     * @param puppet_id[in] The puppet the status message is about.
     */
    PuppetStatusMessage(
        const dbtype::Id &puppet_id)
          : ProcessMessage(
              executor::ProcessMessage::MESSAGE_INTERPROCESS,
              ::PUPPET_STATUS_MESSAGE_SUBTYPE),
          puppet(puppet_id)
    { }

    /**
     * Required virtual destructor.
     */
    virtual ~PuppetStatusMessage()
      { }

    /**
     * @return The ID of the puppet the error is about.
     */
    const dbtype::Id &get_puppet_id(void) const
      { return puppet; }

private:

    const dbtype::Id puppet; ///< Which puppet the status is about
};
}
}

#endif //MUTGOS_USERAGENT_PUPPETSTATUSMESSAGE_H
