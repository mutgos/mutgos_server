/*
 * events_ProcessExecutionEventProcessor.h
 */

#ifndef MUTGOS_EVENTS_PROCESSEXECUTIONEVENTPROCESSOR_H
#define MUTGOS_EVENTS_PROCESSEXECUTIONEVENTPROCESSOR_H

#include <map>

#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"

#include "events/events_ProcessExecutionSubscriptionParams.h"
#include "events/events_ProcessExecutionEvent.h"

namespace mutgos
{
namespace events
{
    /**
     * Processes ProcessExecutionEvents and notifies listeners of subscription
     * matches.
     *
     * Of note, if a subscription param indicates it is watching for a
     * specific PID, and the PID completes/killed, the subscription will
     * be deleted (and the owner notified).
     */
    class ProcessExecutionEventProcessor :
        public SubscriptionProcessor,
        public SubscriptionProcessorSupport<
            ProcessExecutionSubscriptionParams,
            ProcessExecutionEvent>
    {
    public:
        /**
         * Creates an ProcessExecutionEventProcessor.
         * @param data_ptr[in] Pointer to master subscription data.  It will
         * be held but ownership will not be transferred.
         */
        ProcessExecutionEventProcessor(SubscriptionData * const data_ptr);

        /**
         * Required virtual destructor.
         */
        virtual ~ProcessExecutionEventProcessor();

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

        typedef std::map<executor::PID, SubscriptionList> PidSubscriptions;

        SiteIdToEntitySubscriptions entity_subscriptions; ///< Watch for specific executables or owners
        SiteIdToSubscriptionsList site_subscriptions; ///< Watch for specific sites (executables or owners)
        PidSubscriptions pid_subscriptions; ///< Watch for specific PIDs
        SubscriptionList all_subscriptions; ///< Watch everything
    };
}
}

#endif //MUTGOS_EVENTS_PROCESSEXECUTIONEVENTPROCESSOR_H
