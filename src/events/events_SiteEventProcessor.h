/*
 * events_SiteEventProcessor.h
 */

#ifndef MUTGOS_EVENTS_SITEEVENTPROCESSOR_H
#define MUTGOS_EVENTS_SITEEVENTPROCESSOR_H

#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"
#include "events/events_SiteSubscriptionParams.h"
#include "events/events_SiteEvent.h"


namespace mutgos
{
namespace events
{
    /**
     * Processes SiteEvents and notifies listeners of subscription matches.
     */
    class SiteEventProcessor :
        public SubscriptionProcessor,
        public SubscriptionProcessorSupport<SiteSubscriptionParams, SiteEvent>
    {
    public:
        /**
         * Creates an SiteEventProcessor.
         * @param data_ptr[in] Pointer to master subscription data.  It will
         * be held but ownership will not be transferred.
         */
        SiteEventProcessor(SubscriptionData * const data_ptr);

        /**
         * Required virtual destructor.
         */
        virtual ~SiteEventProcessor();

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
        SubscriptionList all_subscriptions; ///< Watch everything
    };
}
}

#endif //MUTGOS_EVENTS_SITEEVENTPROCESSOR_H
