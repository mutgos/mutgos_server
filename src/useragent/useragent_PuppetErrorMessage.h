/*
 * useragent_PuppetErrorMessage.h
 */

#ifndef MUTGOS_USERAGENT_PUPPETERRORMESSAGE_H
#define MUTGOS_USERAGENT_PUPPETERRORMESSAGE_H

#include <string>

#include "executor/executor_ProcessMessage.h"
#include "dbtypes/dbtype_Id.h"

namespace
{
const std::string PUPPET_ERROR_MESSAGE_SUBTYPE = "PuppetErrorMessage";
}

namespace mutgos
{
namespace useragent
{
/**
 * Represents an error message from processing a PuppetCommandMessage,
 * sent from the puppet manager to the user agent.
 */
class PuppetErrorMessage : public executor::ProcessMessage
{
public:
    /**
     * @return Subtype of this message for identification purposes.
     */
    static const std::string &message_subtype(void)
      { return ::PUPPET_ERROR_MESSAGE_SUBTYPE;}

    /**
     * Creates am  interprocess puppet error message.
     * @param puppet_id[in] The puppet the error message is about.
     * @param message[in] The error message concerning the puppet.  The error
     * is suitable for display to the Player.
     */
    PuppetErrorMessage(
        const dbtype::Id &puppet_id,
        const std::string &message)
          : ProcessMessage(
              executor::ProcessMessage::MESSAGE_INTERPROCESS,
              ::PUPPET_ERROR_MESSAGE_SUBTYPE),
          puppet(puppet_id),
          error_message(message)
    { }

    /**
     * Required virtual destructor.
     */
    virtual ~PuppetErrorMessage()
      { }

    /**
     * @return The ID of the puppet the error is about.
     */
    const dbtype::Id &get_puppet_id(void) const
      { return puppet; }

    /**
     * @return The error message, expected to be displayed to the Player.
     */
    const std::string &get_error_message(void) const
      { return error_message; }

private:

    const dbtype::Id puppet; ///< Which puppet the error is about
    const std::string error_message; ///< The error message
};
}
}

#endif //MUTGOS_USERAGENT_PUPPETERRORMESSAGE_H
