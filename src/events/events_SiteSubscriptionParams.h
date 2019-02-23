/*
 * events_SiteSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_SITESUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_SITESUBSCRIPTIONPARAMS_H

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
    class SiteEvent;

    /**
     * A Site Event subscription.  This allows a subscriber to be notified
     * about anything concerning major Site changes (creation, deletion, etc).
     *
     * Currently, there are no parameters available; all site events will
     * be received if subscribed.
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * cannot be unset.
     */
    class SiteSubscriptionParams : public SubscriptionParams
    {
    public:
        /**
         * Standard constructor.
         */
        SiteSubscriptionParams(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        SiteSubscriptionParams(const SiteSubscriptionParams &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~SiteSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        SiteSubscriptionParams &operator=(
            const SiteSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const SiteSubscriptionParams &rhs) const;

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
        virtual bool is_match(const SiteEvent *event_ptr) const;
    };
}
}

#endif //MUTGOS_EVENTS_SITESUBSCRIPTIONPARAMS_H
