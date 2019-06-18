/*
 * message_ClientMatchNameRequest.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTMATCHNAMEREQUEST_H
#define MUTGOS_MESSAGE_CLIENTMATCHNAMEREQUEST_H

#include <string>

#include "dbtypes/dbtype_EntityType.h"

#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to query the database for an Entity matching
     * the given parameters.
     *
     * See primitives::DatabasePrims::match_name_to_id() to understand
     * how to use the arguments.
     * @see primitives::DatabasePrims::match_name_to_id()
     * @see ClientMatchNameResult
     */
    class ClientMatchNameRequest : public ClientMessage
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
        ClientMatchNameRequest(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientMatchNameRequest(const ClientMatchNameRequest &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientMatchNameRequest();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The Entity or action name to search for.
         */
        const std::string &get_search_string(void) const
        { return search_string; }

        /**
         * Sets the search string.
         * @param search[in] The Entity or action name to search for.
         */
        void set_search_string(const std::string &search)
        { search_string = search; }

        /**
         * @return True if exact match is required.
         */
        bool get_exact_match_flag(void) const
        { return exact_match; }

        /**
         * Sets the exact match flag.
         * @param exact[in] True if exact match is required.  Default true.
         */
        void set_exact_match_flag(const bool exact)
        { exact_match = exact; }

        /**
         * @return The Entity type to search for.
         */
        dbtype::EntityType get_entity_type(void) const
        { return entity_type; }

        /**
         * Sets the entity type.
         * @param type[in] The Entity type to search for.  Default 'Entity'.
         */
        void set_entity_type(const dbtype::EntityType type)
        { entity_type = type; }

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
        std::string search_string; ///< The string to search for
        bool exact_match; ///< True if exact match required
        dbtype::EntityType entity_type; ///< Entity type to search for.
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTMATCHNAMEREQUEST_H
