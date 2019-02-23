/*
 * events_MovementEventProcessor.h
 */

#ifndef MUTGOS_EVENTS_MOVEMENTEVENTPROCESSOR_H
#define MUTGOS_EVENTS_MOVEMENTEVENTPROCESSOR_H

#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"

#include "events/events_MovementSubscriptionParams.h"
#include "events/events_MovementEvent.h"

namespace mutgos
{
namespace events
{
    /**
     * Processes MovementEvents and notifies listeners of subscription
     * matches.
     */
    class MovementEventProcessor :
        public SubscriptionProcessor,
        public SubscriptionProcessorSupport<
            MovementSubscriptionParams,
            MovementEvent>
    {
    public:
        /**
         * Creates an MovementEventProcessor.
         * @param data_ptr[in] Pointer to master subscription data.  It will
         * be held but ownership will not be transferred.
         */
        MovementEventProcessor(SubscriptionData * const data_ptr);

        /**
         * Required virtual destructor.
         */
        virtual ~MovementEventProcessor();

        /**
         * Called when an Entity has been deleted from the database.  This will
         * go through and remove any subscriptions that specifically
         * referenced it.
         * It will also perform any callbacks to notify listeners.
         * @param entity_id[in] The Entity that has been removed.
         */
        virtual void entity_deleted(const dbtype::Id &entity_id);

        /**
         * Called when a Site has been deleted from the database.  This will
         * go through and remove any subscriptions that specifically
         * referenced it.
         * It will also perform any callbacks to notify listeners.
         * @param site_id[in] The site ID that has been removed.
         */
        virtual void site_deleted(const dbtype::Id::SiteIdType site_id);

        /**
         * Determines what subscriptions are satisfied by the Event and
         * call back the listeners.
         * @param event_ptr[in] The event to process.
         */
        virtual void process_event(Event * const event_ptr);

        /**
         * Adds the given subscription to this processor.
         * @param subscription[in] The subscription to add.  It will be copied.
         * @param callback[in] The callback to add.  It will be copied.

         * @return The ID of the subscription, or 0 if error or out of IDs.
         */
        virtual SubscriptionId add_subscription(
            const SubscriptionParams &subscription,
            const SubscriptionCallback &callback);

        /**
         * Removes the subscription from this processor.
         * @param subscription_id[in] The ID of the subscription to remove.
         * @return True if found and removed, false if not.
         */
        virtual bool remove_subscription(const SubscriptionId subscription_id);

    private:
        /**
         * Deletes the given subscription from the internal data structures
         * and SubscriptionData.
         * This assumes a write lock has already been acquired!
         * @param subscription_id[in] The subscription ID to delete.
         * @return True if success.
         */
        bool internal_remove_subscription(
            const SubscriptionId subscription_id);

        /**
         * Used when processing entity_deleted(), this will look up the entity
         * in the provided subscription data structure and add all callbacks
         * that reference the entity to callback_set.
         * @param entity_subscriptions[in] The subscription data structure to
         * look for the Entity.
         * @param entity_id[in] The entity that has been deleted.
         * @param callback_set[out] Any matching callbacks are added to this
         * set.  The set is not cleared.
         */
        void add_matched_for_deleted(
            SiteIdToEntitySubscriptions &entity_subscriptions,
            const dbtype::Id &entity_id,
            SubscriptionCallbackSet callback_set);

        SiteIdToEntitySubscriptions who_subscriptions; ///< Watch for who moves
        SiteIdToEntitySubscriptions from_subscriptions; ///< Watch for originating location
        SiteIdToEntitySubscriptions to_subscriptions;  ///< Watch for where they move to
        SiteIdToEntitySubscriptions how_subscriptions; ///< Watch for cause of movement
        SiteIdToSubscriptionsList site_subscriptions; ///< Watch for specific sites
        SubscriptionList all_subscriptions; ///< Watch everything
    };
}
}

#endif //MUTGOS_EVENTS_MOVEMENTEVENTPROCESSOR_H
