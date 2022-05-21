/*
 * message_ClientConnectPuppetRequest.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTCONNECTPUPPETREQUEST_H
#define MUTGOS_MESSAGE_CLIENTCONNECTPUPPETREQUEST_H

#include "dbtypes/dbtype_Id.h"
#include "comminterface/comm_CommonTypes.h"
#include "utilities/json_JsonUtilities.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    class ClientConnectPuppetRequest : public ClientMessage
    {
    public:
        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        ClientConnectPuppetRequest(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientConnectPuppetRequest(const ClientConnectPuppetRequest &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientConnectPuppetRequest();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The entity ID of the puppet to connect/start.
         */
        const dbtype::Id &get_puppet_entity_id(void) const
          { return puppet_entity_id; }

        /**
         * Sets the entity ID of the puppet to connect/start.
         * @param id[in] The puppet entity ID.
         */
        void set_puppet_entity_id(const dbtype::Id &id)
          { puppet_entity_id = id; }

        /**
         * Saves this message to the provided document.
         * This is normally not used, but is available for debugging/testing.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        dbtype::Id puppet_entity_id; ///< The ID of the puppet to connect/start
    };
}
}


#endif //MUTGOS_MESSAGE_CLIENTCONNECTPUPPETREQUEST_H
