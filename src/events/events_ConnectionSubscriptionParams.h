/*
 * events_ConnectionSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_CONNECTIONSUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_CONNECTIONSUBSCRIPTIONPARAMS_H

#include <vector>
#include <string>

#include "dbtypes/dbtype_Id.h"
#include "events/events_SubscriptionParams.h"
#include "dbtypes/dbtype_Entity.h"


namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class ConnectionEvent;

    /**
     * A connection change event subscription.  This allows the subscriber to
     * be notified about anything concerning a connection, such as when a
     * Player connects or disconnects.
     *
     * Hostname and IP addresses (part of the 'source' of the connection) use
     * 'contained in' matching.  For instance, a hostname of 'myisp.com' will
     * match 'user.myisp.com', 'home.myisp.com', etc.
     *
     * When an attribute of this subscription parameters is left blank,
     * it means 'any'.  When an attribute allows or has more than one
     * entry, it can be assumed all the entries are ORed together (example for
     * interested Entity IDs:  #123 OR #456 OR #789).
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * cannot be unset.
     */
    class ConnectionSubscriptionParams : public SubscriptionParams
    {
    public:
        /**
         * The type of connection event type being watched for.
         */
        enum ConnectionTypeSubscription
        {
            /** Connections */
            CONNECTION_TYPE_CONNECT,
            /** Disconnections */
            CONNECTION_TYPE_DISCONNECT,
            /** Connections and disconnections */
            CONNECTION_TYPE_ALL
        };

        /** List of hostnames or IPs, or other connection sources */
        typedef std::vector<std::string> SourceList;

        /**
         * Standard constructor.
         */
        ConnectionSubscriptionParams(void);

        /**
         * Constructor that sets eveerything.
         * @param type[in] Type of connection event(s) of interest.
         * @param ids[in] Entity IDs whose connection events are of interest.
         * @param sites[in] The site IDs of interest.  Overrides IDs.
         * @param sources[in] Source search strings (partial hostnames, IPs,
         * etc) whose connection events are of interest.
         */
        ConnectionSubscriptionParams(
            const ConnectionTypeSubscription type,
            const dbtype::Entity::IdVector &ids,
            const dbtype::Id::SiteIdVector &sites,
            const SourceList &sources);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        ConnectionSubscriptionParams(const ConnectionSubscriptionParams &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ConnectionSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        ConnectionSubscriptionParams &operator=(
            const ConnectionSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const ConnectionSubscriptionParams &rhs) const;

        /**
         * Sets the connection type(s) of interest.
         * @param type[in] Type of connection event(s) of interest.
         */
        void set_connection_type(const ConnectionTypeSubscription type)
          { connection_type = type; }

        /**
         * @return The connection type(s) of interest.
         */
        ConnectionTypeSubscription get_connection_type(void) const
          { return connection_type; }

        /**
         * Adds an entity ID of interest.  This can only be called if there are
         * no site IDs added.
         * @param id[in] The entity ID of interest.
         */
        void add_entity_id(const dbtype::Id &id)
          { connection_entity_ids.push_back(id); }

        /**
         * @return The entity IDs of interest.
         */
        const dbtype::Entity::IdVector &get_entity_ids(void) const
          { return connection_entity_ids; }

        /**
         * Adds a site ID of interest.  If this is called, entity IDs cannot
         * be added.
         * @param site_id[in] The site ID of interest.
         */
        void add_site_id(const dbtype::Id::SiteIdType site_id)
          { connection_site_ids.push_back(site_id); }

        /**
         * @return The site IDs of interest.
         */
        const dbtype::Id::SiteIdVector &get_site_ids(void) const
          { return connection_site_ids; }

        /**
         * Adds a partial source of interest.
         * @param source[in] The partial source to add.
         */
        void add_source(const std::string &source);

        /**
         * @return The partial sources of interest.
         */
        const SourceList &get_sources(void) const
          { return connection_sources; }

        /**
         * Validates that the subscription is valid (has all needed fields
         * filled in and that they are properly filled in.
         * @return True if subscription is valid, false if there is a problem.
         */
        virtual bool validate(void) const;

        /**
         * @return A copy of this subscription.  Caller is responsible for
         * managing the pointer.
         */
        virtual SubscriptionParams *clone(void) const;

        /**
         * @param id[in] The ID to check.
         * @return True if the subscription parameters specifically reference
         * the given ID anywhere.
         */
        virtual bool references_id(const dbtype::Id &id) const;

        /**
         * @param site_id[in] The site ID to check.
         * @return True if the subscription parameters specifically reference
         * the given site ID anywhere, including in entity IDs.
         */
        virtual bool references_site(
            const dbtype::Id::SiteIdType site_id) const;

        /**
         * @return The subscription as a string, for diagnostic/logging
         * purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * Evaluates the event and determine if it matches this subscription.
         * @param event_ptr[in] The event to evaluate.
         * @return True if subscription matches event.
         */
        virtual bool is_match(const ConnectionEvent *event_ptr) const;

    private:
        ConnectionTypeSubscription connection_type; ///< The type of connection event of interest
        dbtype::Entity::IdVector connection_entity_ids; ///< IDs of entities associated with event of interest
        dbtype::Id::SiteIdVector connection_site_ids; ///< Site IDs of interest
        SourceList connection_sources; ///< Partial match connection sources of interest
    };
}
}

#endif //MUTGOS_EVENTS_CONNECTIONSUBSCRIPTIONPARAMS_H
