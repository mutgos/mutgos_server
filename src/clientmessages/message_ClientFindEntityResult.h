/*
 * message_ClientFindEntityResult.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTFINDENTITYRESULT_H
#define MUTGOS_MESSAGE_CLIENTFINDENTITYRESULT_H

#include <string>
#include <tuple>
#include <vector>

#include "clientmessages/message_ClientMessage.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityType.h"

namespace mutgos
{
namespace message
{
    /**
     * Provided as a response to any queries that search for Entities.
     * This is not received by the server; getters are for testing only.
     * @see ClientFindEntityRequest
     */
    class ClientFindEntityResult : public ClientMessage
    {
    public:
        /** ID, name, and type of Entity */
        typedef std::tuple<dbtype::Id, std::string , dbtype::EntityType>
            FoundEntity;
        typedef std::vector<FoundEntity> FoundEntities;

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        ClientFindEntityResult(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientFindEntityResult(const ClientFindEntityResult &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientFindEntityResult();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return All found Entities, or empty if error or none.
         */
        const FoundEntities &get_entities(void) const
        { return result; }

        /**
         * Sets the found entities.
         * @param entities[in] The Entities found.
         */
        void set_entities(const FoundEntities &entities)
        { result = entities; }

        /**
         * Used when it's known there will be at most one match.
         * @return The first Entity found, or default if none.
         */
        FoundEntity get_first_entity(void) const;

        /**
         * Adds an Entity that was found.
         * @param id[in] The ID of the Entity to add.
         * @param name[in] The name of the Entity to add.
         * @param type[in] The type of the Entity to add.
         */
        void add_entity(
            const dbtype::Id &id,
            const std::string &name,
            const dbtype::EntityType &type)
        { result.push_back(FoundEntity(id, name, type)); }

        /**
         * @return True if the failure was due to a security violation.
         */
        bool get_security_violation_flag(void) const
        { return security_violation; }

        /**
         * Sets the security violation flag.
         * @param violation[in] True if the failure was due to a security
         * violation.  Default false.
         */
        void set_security_violation_flag(const bool violation)
        { security_violation = violation; }

        /**
         * @return True if the failure was due to an ambiguous match.
         */
        bool get_ambiguous_flag(void) const
        { return ambiguous; }

        /**
         * Sets the ambiguous flag.
         * @param ambiguous_flag[in] True if the failure was due to an
         * ambiguous match.  Default false.
         */
        void set_ambiguous_flag(const bool ambiguous_flag)
        { ambiguous = ambiguous_flag; }

        /**
         * Sets the error flag.
         * @param has_error[in]  True if there was an error in the query.
         * Default false.
         * @param has_error
         */
        void set_error(const bool has_error)
        { error = has_error; }

        /**
         * @return True if result has an error.
         */
        bool get_error(void) const
        { return error; }

        /**
         * Sets the error message (only sent if there is an error).
         * @param message[ni] The error message.
         */
        void set_error_message(const std::string &message)
        { error_message = message; }

        /**
         * @return The error message.
         */
        const std::string  &get_error_message(void) const
        { return error_message; }

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
         * Currently this message is only sent, never received.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        FoundEntities result; ///< If no errors, these are Entities that matched the query
        bool security_violation; ///< If true, a failure occurred due to a security violation
        bool ambiguous;  ///< If true, a failure occurred due to the search string being too ambiguous

        bool error;  ///< When true, there was an error processing the request
        std::string error_message; ///< When true, this holds the cause of the error
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTMATCHNAMERESULT_H
