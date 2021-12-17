/*
 * useragent_PuppetCommandMessage.h
 */

#ifndef MUTGOS_USERAGENT_PUPPETCOMMANDMESSAGE_H
#define MUTGOS_USERAGENT_PUPPETCOMMANDMESSAGE_H

#include <string>

#include "executor/executor_ProcessMessage.h"
#include "dbtypes/dbtype_Id.h"

namespace
{
const std::string PUPPET_COMMAND_MESSAGE_SUBTYPE = "PuppetCommandMessage";
}

namespace mutgos
{
namespace useragent
{
/**
 * Represents the raw text to be sent to the puppet agent process, where it
 * will be processed.  This can also send a 'ping' message to the agent.
 */
class PuppetCommandMessage : public executor::ProcessMessage
{
public:
    /**
     * @return Subtype of this message for identification purposes.
     */
    static const std::string &message_subtype(void)
      { return ::PUPPET_COMMAND_MESSAGE_SUBTYPE;}

    /**
     * Creates am  interprocess puppet ping message.
     * @param puppet_id[in] The puppet the ping is for.
     */
    PuppetCommandMessage(
        const dbtype::Id &puppet_id)
        : ProcessMessage(
          executor::ProcessMessage::MESSAGE_INTERPROCESS,
          ::PUPPET_COMMAND_MESSAGE_SUBTYPE),
          puppet(puppet_id)
    { }

    /**
     * Creates am  interprocess puppet command message.
     * @param puppet_id[in] The puppet the command is for.
     * @param puppet_command[in] The input/command line for the puppet.
     */
    PuppetCommandMessage(
        const dbtype::Id &puppet_id,
        const std::string &puppet_command)
          : ProcessMessage(
              executor::ProcessMessage::MESSAGE_INTERPROCESS,
              "PuppetCommandMessage"),
          puppet(puppet_id),
          input_line(puppet_command)
    { }

    /**
     * Required virtual destructor.
     * This will also clean up the text line to avoid memory leaks.
     */
    virtual ~PuppetCommandMessage()
      { }

    /**
     * @return True if message is a ping message.
     */
    bool is_ping(void) const
      { return input_line.empty(); }

    /**
     * @return The ID of the puppet the command is for.
     */
    const dbtype::Id &get_puppet_id(void) const
      { return puppet; }

    /**
     * @return The input line (command) for the puppet.
     */
    const std::string &get_input_line(void) const
      { return input_line; }

private:

    /**
     * Copy constructor (disabled).
     */
    PuppetCommandMessage(const PuppetCommandMessage &rhs);

    /**
     * Assignment operator (disabled).
     */
    PuppetCommandMessage &operator=(const PuppetCommandMessage &rhs);

    const dbtype::Id puppet; ///< Which puppet to command
    const std::string input_line; ///< The puppet input (command)
};
}
}

#endif //MUTGOS_USERAGENT_PUPPETCOMMANDMESSAGE_H
