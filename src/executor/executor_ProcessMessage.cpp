
#include "executor/executor_ProcessMessage.h"

namespace mutgos
{
namespace executor
{
    //-----------------------------------------------------------------------
    ProcessMessage::ProcessMessage(
        const ProcessMessageType type,
        const std::string &subtype)
      : message_type(type),
        message_subtype(subtype)
    {
    }

    //-----------------------------------------------------------------------
    ProcessMessage::ProcessMessage(const ProcessMessageType type)
      : message_type(type),
        message_subtype("")
    {
    }

    //-----------------------------------------------------------------------
    ProcessMessage::ProcessMessage(const ProcessMessage &rhs)
      : message_type(rhs.message_type),
        message_subtype(rhs.message_subtype)
    {
    }

    //-----------------------------------------------------------------------
    ProcessMessage::~ProcessMessage()
    {
    }
}
}