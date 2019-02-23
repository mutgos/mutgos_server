/*
 * events_SubscriptionData.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONDATA_H
#define MUTGOS_EVENTS_SUBSCRIPTIONDATA_H

#include <map>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include "dbtypes/dbtype_Id.h"

#include "events/events_CommonTypes.h"
#include "events/events_SubscriptionParams.h"
#include "events/events_SubscriptionCallback.h"
#include "executor/executor_ProcessInfo.h"
#include "events/events_Event.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class SubscriptionProcessor;

    /**
     * This thread-safe class holds common data about all subscriptions, such
     * as callback info, which subscriptions a PID owns, which processor owns
     * a subscription, etc.
     *
     * The individual subscription processors will each have a pointer to this
     * class.
     */
    class SubscriptionData
    {
    public:
        /** First is the subscription itself, second is the callback */
        typedef std::pair<SubscriptionParams *, SubscriptionCallback>
            SubscriptionParamCallback;

        /**
         * Constructor.
         */
        SubscriptionData(void);

        /**
         * Destructor.  Will delete any pointers still held by this class.
         */
        ~SubscriptionData();

        /**
         * Adds a new subscription to this class's data structures.  A unique
         * subscription ID will be picked and assigned.
         * @param subscription_ptr[in] Pointer to the subscription data.  This
         * method will take ownership of the pointer success or failure.
         * @param subscription_type[in] The type of event the subscription is
         * for, which determines the processor that handles it.
         * @param callback[in] The callback information, to call the listener
         * when the subscription matches something.  This method will take
         * ownership of the pointer success or failure.
         * @return A unique subscription ID to identify the subscription, or
         * 0 if error.  If error, pointers will be deleted.
         */
        SubscriptionId add_subscription(
            SubscriptionParams * const subscription_ptr,
            const Event::EventType subscription_type,
            SubscriptionCallback * const callback_ptr);

        /**
         * Removes a subscription from this class's data structures.
         * Any pointers passed in via add_subscription() will be deleted
         * during this call!
         * @param id[in] The subscription ID to remove.
         * @return True if successfully removed, false if not found.
         */
        bool remove_subscription(const SubscriptionId id);

        /**
         * @param id[in] The subscription ID to check.
         * @return True if subscription exists.
         */
        bool subscription_exists(const SubscriptionId id);

        /**
         * @param pid[in] The PID to get subscriptions for.
         * @return The subscriptions associated with the PID, or empty
         * if none or PID not found.
         */
        SubscriptionIdList get_subscriptions_for_process(
            const executor::PID pid);

        /**
         * Get the information provided to this class via add_subscription()
         * concerning the given subscription.
         * @param id[in] The ID of the subscription to retrieve.
         * @return The subscription data.  If the ID is not found, the
         * data will be invalid (null subscription pointer, the callback
         * will not be valid, etc).
         */
        SubscriptionParamCallback get_subscription_info(
            const SubscriptionId id);

        /**
         * @param id[in] The ID of the subscription to retrieve.
         * @return The event type the subscription is for, or END_INVALID if
         * not found.  This determines which processor handles it.
         */
        Event::EventType get_subscription_type(const SubscriptionId id);

        /**
         * Registers the given processor for the event type it processes.  If
         * the event type is already registered, it will be deleted and
         * overwritten.
         * @param processor_ptr[in] The pointer to the processor.  This class
         * will take ownership of the pointer.
         */
        void register_subscription_processor(
            SubscriptionProcessor * const processor_ptr);

        /**
         * It should be assumed all event types have an associated processor.
         * @param event_type The event type to get a processor for.
         * @return The pointer to the processor.  Do not delete the pointer.
         */
        inline SubscriptionProcessor *get_subscription_processor(
            const Event::EventType event_type)
          { return subscription_processors[event_type]; }

    private:
        /**
         * Simple container class to hold data about subscriptions.
         */
        class SubscriptionDetails
        {
        public:
            /**
             * Defaults constructor for STL.
             */
            SubscriptionDetails(void)
                : pid(0),
                  event_type(Event::EVENT_END_INVALID),
                  params_ptr(0),
                  callback_ptr(0)
            { }

            /**
             * Creates a new SubscriptionDetails.  Pointers must be manually
             * managed.
             * @param subscription_pid[in] The owning PID of the subscription,
             * or 0 if no in-game process owns it.
             * @param subscription_type[in] The type of event the subscription
             * is for.
             * @param subscription_params[in] The subscription parameters.
             * @param subscription_callback[in] The callback for the listener.
             */
            SubscriptionDetails(
                const executor::PID subscription_pid,
                const Event::EventType subscription_type,
                SubscriptionParams * const subscription_params,
                SubscriptionCallback * const subscription_callback)
                : pid(subscription_pid),
                  event_type(subscription_type),
                  params_ptr(subscription_params),
                  callback_ptr(subscription_callback)
            { }

            executor::PID pid; ///< PID who owns subscription, or 0 if none
            Event::EventType event_type; ///< Type of event subscription is for
            SubscriptionParams * params_ptr; ///< Subscription parameters
            SubscriptionCallback * callback_ptr; ///< Listener callback
        };

        /** Map of PID to subscriptin IDs belonging to it */
        typedef std::map<executor::PID, SubscriptionIdList> PidToSubscriptions;
        /** Map of subscription ID to data about the subscription */
        typedef std::map<SubscriptionId, SubscriptionDetails>
            SubscriptionIdToData;

        /**
         * Assumes a write lock has already been acquired.
         * @return The next unique, unused subscription ID.
         */
        SubscriptionId get_next_subscription_id(void);

        const SubscriptionIdList empty_id_list; ///< Used when method returns a referenc and it has no match
        const SubscriptionParamCallback empty_subscription; ///< when method returns a referenc and it has no match

        PidToSubscriptions pid_subscriptions; ///< Subscriptions owned by a Process
        SubscriptionIdToData subscription_data; ///< Information about each subscription

        SubscriptionId next_unique_subscription_id; ///< Next ID when adding new subscription
        const SubscriptionId max_subscription_id; ///< Highest subscription ID

        /** Populated during initialization, this maps event type to
            subscription processor */
        SubscriptionProcessor *subscription_processors[Event::EVENT_END_INVALID + 1];

        boost::shared_mutex subscription_lock; ///< The lock for accessing data
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONDATA_H
