/*
 * message_ClientMatchNameResult.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTMATCHNAMERESULT_H
#define MUTGOS_MESSAGE_CLIENTMATCHNAMERESULT_H

#include <string>
#include <vector>

#include "clientmessages/message_ClientMessage.h"

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace message
{
    /**
     * Provided as a response to the match name request.
     * @see ClientMatchNameRequest
     */
    class ClientMatchNameResult : public ClientMessage
    {
    public:
        typedef std::vector<dbtype::Id> MatchingIds;

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        ClientMatchNameResult(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientMatchNameResult(const ClientMatchNameResult &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientMatchNameResult();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return All matching IDs, or empty if error or no match.
         */
        const MatchingIds &get_matching_ids(void) const
        { return matching_ids; }

        /**
         * Sets the matching IDs.
         * @param ids[in] The matching IDs.
         */
        void set_matching_ids(const MatchingIds &ids)
        { matching_ids = ids; }

        /**
         * Used when it's known there will be at most one match.
         * @return The first matching ID, or default if none.
         */
        dbtype::Id get_matching_id(void) const;

        /**
         * Adds a matching ID.
         * @param id[in] The matching ID to add.
         */
        void add_matching_id(const dbtype::Id &id)
        { matching_ids.push_back(id); }

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
        MatchingIds matching_ids; ///< If no errors, these are the ID(s) that matched the query
        bool security_violation; ///< If true, a failure occurred due to a security violation
        bool ambiguous;  ///< If true, a failure occurred due to the search string being too ambiguous
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTMATCHNAMERESULT_H
