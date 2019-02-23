#ifndef MUTGOS_MESSAGE_REGISTRY_H
#define MUTGOS_MESSAGE_REGISTRY_H

#include <vector>
#include "message_ClientMessageType.h"

namespace mutgos
{
namespace message
{
    // Forward declarations
    //
    class ClientMessage;

    // A factory that maps message type to the creation of a new message
    // of that type, primarily used for deserialization.
    //
    class MessageFactory
    {
    public:
        typedef ClientMessage *(*ClientMessageCreateFunc)();

        /**
         * Registers a message type and its creation function.  Used during
         * static initialization of the libraries.  If the same method is
         * re-registered, the existing entry will simply be overwritten.
         * @param type[in] The type of message to register.
         * @param create_func[in] A no-arguments method that creates a new
         * instance of the message.
         * @return True.  Used to give static initalization something to store.
         */
        static bool register_message(
            const ClientMessageType type,
            ClientMessageCreateFunc create_func);

        /**
         * Creates a client message of the given type.
         * @param type[in] The type of message to create.
         * @return Pointer to the newly created message.  Caller must manage
         * the pointer!  Returns null if type not found or error.
         */
        static ClientMessage *create_message(const ClientMessageType type);

    private:
        // Static-only class
        //
        MessageFactory(void);
        MessageFactory(const MessageFactory &rhs);

        typedef std::vector<ClientMessageCreateFunc> ClientMessageFactory;

        static ClientMessageFactory client_message_factory; ///< Factory for client messages
    };
}
}

#endif //MUTGOS_MESSAGE_REGISTRY_H
