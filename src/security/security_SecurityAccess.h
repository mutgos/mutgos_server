/*
 * security_SecurityAccess.h
 */

#ifndef MUTGOS_SECURITY_SECURITYACCESS_H
#define MUTGOS_SECURITY_SECURITYACCESS_H

#include <map>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include "security_OperationsCapabilities.h"
#include "security_Context.h"

#include "dbtypes/dbtype_Id.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "events/events_EventListener.h"
#include "events/events_CommonTypes.h"

namespace mutgos
{
namespace security
{
    // Forward declarations
    //
    class SecurityChecker;

    // TODO Will need to do recursive (environment/inherited) security checks at some point.

    /**
     * Other namespaces can use this interface to interact with the
     * security subsystem, make security checks, etc.
     */
    class SecurityAccess : public events::EventListener
    {
    public:
        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static SecurityAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * This is thread safe.
         * @return The singleton instance, or null if not created.
         */
        static inline SecurityAccess *instance(void)
        { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * It will also register itself as a listener as needed in other
         * subsystems, create any processor classes, start any threads, etc.
         * Attempting to use other methods prior to calling startup() may
         * result in a crash.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Basically the opposite of what startup() does.
         */
        void shutdown(void);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            const bool throw_exception_on_denied = true);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_type[in] The type of entity being checked.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            const dbtype::EntityType entity_type,
            const bool throw_exception_on_denied = true);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const bool throw_exception_on_denied = true);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param application[in] The application or property path on the
         * entity_target being checked.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const std::string &application,
            const bool throw_exception_on_denied = true);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The Entity being checked.
         * @param entity_field[in] The field on the entity_target being
         * checked.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            const dbtype::EntityField entity_field,
            const bool throw_exception_on_denied = true);

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The destination or target Entity being
         * checked.
         * @param entity_source[in] The source of the entity_target, or the
         * source Entity being moved to entity_target.
         * @param throw_exception_on_denied[in] If true (the default), a
         * SecurityException will be thrown if the security check fails,
         * otherwise it will return false.  No exception will ever be returned
         * if true would be returned.
         * @return True if security check passes, false if access denied.
         * @throws SecurityException If conditions are met
         * (see throw_exception_on_denied).
         */
        bool security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            dbinterface::EntityRef &entity_source,
            const bool throw_exception_on_denied = true);

        /**
         * Given a context with a filled out requester, program, and run as
         * requester flag, populate with relevant and allowed capabilities.
         * This can be safely called more than once and will only update
         * capabilities if they are not currently set.
         * @param context[in,out] The context whose capabilities need to be
         * populated.
         */
        void populate_context_capabilities(Context &context);

        /**
         * CALLED BY EVENT SUBSYSTEM ONLY.
         * Called when an event matches a listener's subscription.
         * Only one thread will call this at a time.
         * @param id[in] The subscription ID that matched.
         * @param event[in] The event that matched.
         */
        virtual void subscribed_event_matched(
            const events::SubscriptionId id,
            events::Event &event);

        /**
         * CALLED BY EVENT SUBSYSTEM ONLY.
         * Called when a subscription is deleted by the infrastructure.
         * Currently, this can only happen because an Entity ID in the
         * subscription has been deleted, or a site has been removed.
         * @param ids_deleted[in] The subscription IDs being deleted.
         */
        virtual void subscription_deleted(
            const events::SubscriptionIdList &ids_deleted);

    private:
        typedef std::vector<SecurityChecker *> SecurityVector;
        typedef std::vector<dbtype::Id> CapabilityGroups;
        typedef std::vector<CapabilityGroups> CapabilityGroupsLookup;
        typedef std::map<dbtype::Id::SiteIdType, CapabilityGroupsLookup>
            SiteToCapabilities;

        /**
         * Singleton constructor.
         */
        SecurityAccess(void);

        /**
         * Singleton destructor.
         */
        virtual ~SecurityAccess();

        /**
         * Used to evaluate intermediate results from SecurityCheckers.
         * This will look at the current result, take in the latest result,
         * and decide what the final result is and if more SecurityCheckers
         * need to be called to determine the final result.
         * @param new_result[in] The newest result from a SecurityChecker
         * evaluation.
         * @param current_result[in,out] The current, calculated result.  Must
         * start out as 'SKIP'.  In the case of the security check being
         * passed, it will always end as ACCEPT.
         * @return True if more security checkers need to be called to
         * determine the final result, or false if no more need to be called.
         */
        bool check_result(
            const Result new_result,
            Result &current_result);

        /**
         * Called during initialization, this populates operation_security
         * with all the security checkers.
         */
        void populate_security(void);

        /**
         * The opposite of populate_security().  It will remove and free
         * up memory of every entry in operation_security.  Used during
         * shutdown.
         */
        void depopulate_security(void);

        /**
         * Subscribes to needed events on the Events subsystem.
         * The proper locking is assumed to have been performed.
         */
        void subscribe(void);

        /**
         * Unsubscribes to events that were subscribed to in subscribe().
         * The proper locking is assumed to have been performed.
         */
        void unsubscribe(void);

        /**
         * Checks a site to see if the given context (requester, program) has
         * any capabilities.  If so, add them to the context.  If the
         * capability cache is missing for the site, it will be populated
         * while in this method.
         * @param site_id[in] Site ID to check capabilities.
         * @param read_lock[in,out] The read lock for this class.  Must start
         * out locked.  May be unlocked while in this method, but will always
         * be returned as locked.
         * @param context[in,out] The context to add relevant capabilities to.
         */
        void populate_context_capabilities(
            const dbtype::Id::SiteIdType site_id,
            boost::shared_lock<boost::shared_mutex> &read_lock,
            Context &context);

        /**
         * Populates the cache for a particular site's capability.
         * Assumes a write lock has been acquired.
         * @param site_id[in] The site ID to populate.
         * @param capability[in] The capability info to (re)cache.
         */
        void populate_capability_cache(
            const dbtype::Id::SiteIdType site_id,
            const Capability capability);

        /**
         * Determines what groups the input refers to in its member list,
         * and adds them to referenced_groups.  Disabled entries are skipped.
         * @param input_group[in] The ID of the Capability/group to process.
         * @param referenced_groups[out] Any groups referred to by input_group
         * will be appended to this output, if they are not already present.
         */
        void add_referenced_groups(
            const dbtype::Id &input_group,
            CapabilityGroups &referenced_groups);


        static SecurityAccess *singleton_ptr; ///< Singleton pointer.

        events::SubscriptionId capability_subscription_id; ///< Subscription to watch for changed capabilities
        events::SubscriptionId site_deletion_subscription_id; ///< Subscription to watch for deleted sites

        SecurityVector *operation_security[OPERATION_END_INVALID]; ///< Lookup of security checkers by operation
        /** Unusually large attribute documentation:  Maps site ID to cache of
            what capability and groups that list who has what capability.  The
            first entry in CapabilityGroups is always the capability ID.
            Anything after that are groups referenced by the Capability
            Entity. An empty entry in CapabilityGroupsLookup means it has yet
            to be cached. */
        SiteToCapabilities site_to_capabilities;
        boost::shared_mutex security_lock; ///< The lock for accessing data
    };
}
}

#endif //MUTGOS_SECURITY_SECURITYACCESS_H
