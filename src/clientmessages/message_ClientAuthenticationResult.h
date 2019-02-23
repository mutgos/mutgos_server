/*
 * message_ClientAuthenticationResult.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTAUTHENTICATIONRESULT_H
#define MUTGOS_MESSAGE_CLIENTAUTHENTICATIONRESULT_H

#include "message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Used to indicate if authentication was successful or not, just prior
     * to sending Channel data.
     * This is only sent from the server to the client.
     */
    class ClientAuthenticationResult : public ClientMessage
    {
    public:
        /**
         * Default constructor generally used for deserialization.
         */
        ClientAuthenticationResult(void);

        /**
         * Constructor that sets all attributes.
         * @param auth_result[in] The authentication result (true for success,
         * false for failure/bad password/etc).
         * @param param_result[in] The negotiation result (for all parameters
         * other than authentication).  True for success, false for failure/
         * bad data/etc.
         */
        ClientAuthenticationResult(
            const bool auth_result,
            const bool param_result);

        /**
         * Copy constructor.
         * @param ser_id[in] The source to copy from.
         */
        ClientAuthenticationResult(const ClientAuthenticationResult &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientAuthenticationResult();

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * Sets the authentication result.
         * @param result[in] The authentication result (true for success,
         * false for failure/bad password/etc).
         */
        void set_authentication_result(const bool result)
          { authentication_result = result; }

        /**
         * @return The authentication result (true for success, false for
         * failure/bad password/etc).
         */
        bool get_authentication_result(void) const
          { return authentication_result; }

        /**
         * Sets the negotiation result.
         * @param result[in] The negotiation result (for all parameters
         * other than authentication).  True for success, false for failure/
         * bad data/etc.
         */
        void set_negotiation_result(const bool result)
          { negotiation_result = result; }

        /**
         * @return The negotiation result (for all parameters
         * other than authentication).  True for success, false for failure/
         * bad data/etc.
         */
        bool get_negotiation_result(void) const
          { return negotiation_result; }

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * Not currently supported since it is not used.
         * @param node[in] The JSON node to restore state from.
         * @return False as it is not currently supported.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        bool authentication_result; ///< True if authentication was successful, false if failure
        bool negotiation_result; ///< True if connection parameters other than authentication were accepted.
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTAUTHENTICATIONRESULT_H
