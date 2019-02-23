/*
 * events_EmitSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_EMITSUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_EMITSUBSCRIPTIONPARAMS_H

#include "events/events_SubscriptionParams.h"

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class EmitEvent;

    /**
     * An Emit subscription.  This allows the subscriber to get notified
     * of selected emits, typically only ones directed at an Entity in some
     * way.  To avoid performance issues, this does not allow a complete
     * wildcard - at least one of the parameters must be filled in to be valid.
     * For the same reason, filtering only by site is not possible.
     *
     * Fields that are left at defaults (or empty) are considered wildcards.
     * For an example, not filling in who emitted the event will match
     * anything directed at a particular destination.
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * may not always be unsettable.
     */
    class EmitSubscriptionParams : public SubscriptionParams
    {
    public:

        /**
         * Constructor with nothing set (invalid).
         */
        EmitSubscriptionParams(void);

        /**
         * Constructor that sets everything.
         * @param source[in] The source of the Emit event to filter by.
         * @param target[in] The destination of the Emit event to filter by.
         * @param my_id[in] Optional.  The ID of the Entity subscribed (if
         * applicable).  Used for exclude checking.
         */
        EmitSubscriptionParams(
            const dbtype::Id &source,
            const dbtype::Id &target,
            const dbtype::Id &my_id);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        EmitSubscriptionParams(const EmitSubscriptionParams &rhs);

        /**
         * Virtual destructor.
         */
        virtual ~EmitSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        EmitSubscriptionParams &operator=(const EmitSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const EmitSubscriptionParams &rhs) const;

        /**
         * Sets the source to filter by.  Default is the same as unset.
         * @param source[in] The Entity ID to filter by.
         */
        void set_source(const dbtype::Id &source)
          { emit_source = source; }

        /**
         * @return The source to filter by.
         */
        const dbtype::Id &get_source(void) const
          { return emit_source; }

        /**
         * Sets the target to filter by.  Default is the same as unset.
         * @param target[in] The Entity ID to filter by.
         */
        void set_target(const dbtype::Id &target)
          { emit_target = target; }

        /**
         * @return The target to filter by.
         */
        const dbtype::Id &get_target(void) const
          { return emit_target; }

        /**
         * @return Optional.  The ID of the Entity subscribed (if applicable).
         */
        const dbtype::Id &get_my_id(void) const
          { return emit_my_id; }

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
        virtual bool is_match(const EmitEvent *event_ptr) const;

    private:
        dbtype::Id emit_source; ///< The source of the emit event
        dbtype::Id emit_target; ///< The target of the emit event
        dbtype::Id emit_my_id; ///< The Entity ID of the listener, used for exclude checking
    };
}
}

#endif //MUTGOS_EVENTS_EMITSUBSCRIPTIONPARAMS_H
