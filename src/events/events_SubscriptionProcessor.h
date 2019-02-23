/*
 * events_SubscriptionProcessor.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONPROCESSOR_H
#define MUTGOS_EVENTS_SUBSCRIPTIONPROCESSOR_H

#include <vector>
#include <map>

#include <boost/thread/shared_mutex.hpp>

#include "dbtypes/dbtype_Id.h"
#include "events/events_SubscriptionCallback.h"
#include "events/events_SubscriptionParams.h"
#include "events/events_SubscriptionData.h"
#include "events/events_Event.h"
#include "events/events_CommonTypes.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class SubscriptionData;

    /**
     * The base class for all subscription processors.
     *
     * This class will own the pointers to subscription parameters and
     * callbacks.  This implies when removing a subscription, the subclass
     * must remove it from its data structures before removing it here.
     *
     * For the template, provide the specific type of the SubscriptionParams
     * this processor handles.
     */
    class SubscriptionProcessor
    {
    public:
        /** TODO Order for handling Entity deletion:
         *   Go through list of subscriptions for site.
         *   Put into map the ones that reference the ID (use pair), and add to vector of IDs
         *       If optimized good enough, may not have to go through all subscriptions
         *  For each:
         *   Go through and delete the lists of subscriptions optimized for the ID to speed up deletion
         *   Call do_callback_deleted() on SubscriptionCallbacks
         *   Call internal delete by ID, then remove_subscription_master()
         *
         */


        /** Subscription IDs that have been deleted as a result of site or
            Entity removal */
        typedef std::vector<SubscriptionId> IdsDeleted;

        /**
         * Constructor.
         * @param event_type The Event this processor handles.
         * @param data_ptr[in] The master subscription data.  This pointer
         * will be kept but ownership will not be transferred.
         */
        SubscriptionProcessor(
            const Event::EventType event_type,
            SubscriptionData * const data_ptr);

        /**
         * Destructor.  Subclass will delete all subscriptions handled by this
         * processor.
         */
        virtual ~SubscriptionProcessor();

        /**
         * @return The Event this processor handles.
         */
        Event::EventType get_event_type_handled(void) const
          { return event_type_handled; }

        /**
         * Called when an Entity has been deleted from the database.  This will
         * go through and remove any subscriptions that specifically
         * referenced it.
         * It will also perform any callbacks to notify listeners.
         * @param entity_id[in] The Entity that has been removed.
         */
        virtual void entity_deleted(const dbtype::Id &entity_id) =0;

        /**
         * Called when a Site has been deleted from the database.  This will
         * go through and remove any subscriptions that specifically
         * referenced it.
         * It will also perform any callbacks to notify listeners.
         * @param site_id[in] The site ID that has been removed.
         */
        virtual void site_deleted(const dbtype::Id::SiteIdType site_id) =0;

        /**
         * Determines what subscriptions are satisfied by the Event and
         * call back the listeners.
         * @param event_ptr[in] The event to process.
         */
        virtual void process_event(Event * const event_ptr) =0;

        /**
         * Adds the given subscription to this processor.
         * @param subscription[in] The subscription to add.  It will be copied.
         * @param callback[in] The callback to add.  It will be copied.
         * @return The ID of the subscription, or 0 if error or out of IDs.
         */
        virtual SubscriptionId add_subscription(
            const SubscriptionParams &subscription,
            const SubscriptionCallback &callback) =0;

        /**
         * Removes the subscription from this processor.
         * @param subscription_id[in] The ID of the subscription to remove.
         * @return True if found and removed, false if not.
         */
        virtual bool remove_subscription(const SubscriptionId subscription_id) =0;

    protected:
        SubscriptionData * const subscription_data; ///< Pointer to master subscription data.  Already thread safe.

        boost::shared_mutex subscription_lock; ///< The lock for accessing data on subclasses.

    private:
        const Event::EventType event_type_handled; ///< What event we handle.
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONPROCESSOR_H
