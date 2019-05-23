#include "logging/log_Logger.h"

#include "message_ClientMessageType.h"
#include "message_ClientMessage.h"
#include "message_MessageFactory.h"

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    bool MessageFactory::register_message(
        const ClientMessageType type,
        ClientMessageCreateFunc create_func)
    {
        if (not create_func)
        {
            LOG(error, "message", "register_message", "create_func is null!");
        }
        else
        {
            ClientMessageFactory &client_message_factory = registry();

            if (client_message_factory.empty() ||
                ((client_message_factory.size() - 1) < type))
            {
                client_message_factory.resize(type + 1, 0);
            }

            client_message_factory[type] = create_func;
        }

        return create_func;
    }

    // ----------------------------------------------------------------------
    ClientMessage *MessageFactory::create_message(const ClientMessageType type)
    {
        ClientMessage *message_ptr = 0;

        if ((type >= CLIENTMESSAGE_END_INVALID) or (type < 0))
        {
            LOG(error, "message", "create_message", "Invalid type specified!");
        }
        else
        {
            ClientMessageCreateFunc
                function = registry()[type];

            if (function)
            {
                message_ptr = (*function)();
            }
        }

        return message_ptr;
    }
}
}