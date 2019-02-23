#ifndef MUTGOS_EXECUTOR_MESSAGE_H
#define MUTGOS_EXECUTOR_MESSAGE_H

#include <string>

namespace mutgos
{
namespace executor
{
    /**
     * The ProcessMessage class is used to send data to a process.
     * All classes wishing to send data to a Process must create a class
     * that contains the needed data and is a subclass of ProcessMessage.
     */
    class ProcessMessage
    {
    public:
        /**
         * As most message types are one of a few built-in types, they
         * are hardcoded in an enum for easy filtering.
         */
        enum ProcessMessageType
        {
            /** Message from a text channel */
            MESSAGE_TEXT_CHANNEL,
            /** Message from a client channel */
            MESSAGE_CLIENT_DATA_CHANNEL,
            /** Message from a channel, flow control */
            MESSAGE_CHANNEL_FLOW,
            /** Message from event system - EventMatchedMessage */
            MESSAGE_EVENT,
            /** Message from event system - subscription matched TODO ???  What is this for??? */
            MESSAGE_SUBSCRIPTION,
            /** Message from event system - SubscriptionDeletedMessage */
            MESSAGE_SUBSCRIPTION_DELETED,
            /** Message from another Process  */
            MESSAGE_INTERPROCESS,
            /** Key/value strings type message (map), flexible */
            MESSAGE_GENERIC,
            /** Message from plugin, specify subtype */
            MESSAGE_OTHER
        };

        /**
         * Virtual destructor.
         */
        virtual ~ProcessMessage();

        /**
         * @return The type of the message.
         */
        ProcessMessageType message_get_type(void) const
          { return message_type; }

        /**
         * @return The subtype of the message, or empty string if none.
         */
        const std::string &message_get_subtype(void) const
          { return message_subtype; }

    protected:

        /**
         * Constructor that specifies both the type and optional subtype.
         * @param type[in] The type of message.
         * @param subtype[in] The subtype of the message.
         */
        ProcessMessage(
            const ProcessMessageType type,
            const std::string &subtype);

        /**
         * Copy constructor.
         */
        ProcessMessage(const ProcessMessage &rhs);

        /**
         * Constructor that specifies the type only.
         * @param type[in] The type of message.
         */
        ProcessMessage(const ProcessMessageType type);

    private:
        const ProcessMessageType message_type; ///< Type of message.
        const std::string message_subtype; ///< If type requires, subtype of msg

        // Types cannot be changed once set; assignment disallowed.
        ProcessMessage &operator=(const ProcessMessage &rhs);
    };
}
}

#endif //MUTGOS_EXECUTOR_MESSAGE_H
