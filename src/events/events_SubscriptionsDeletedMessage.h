/*
 * events_SubscriptionsDeletedMessage.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONSDELETEDMESSAGE_H
#define MUTGOS_EVENTS_SUBSCRIPTIONSDELETEDMESSAGE_H

#include "executor/executor_ProcessMessage.h"

#include "events/events_CommonTypes.h"

namespace mutgos
{
namespace events
{
    /**
     * This message is used to indicate subscriptions were unsubscribed
     * by the infrastructure automatically.  Currently, this can only
     * happen because an Entity ID in the subscription has been deleted, or
     * a site has been deleted.
     *
     * Only subscriptions relevant to the Process will be included.
     */
    class SubscriptionsDeletedMessage : public executor::ProcessMessage
    {
    public:
        /**
         * Default constructor (of limited use).
         */
        SubscriptionsDeletedMessage(void)
            : executor::ProcessMessage(
            executor::ProcessMessage::MESSAGE_SUBSCRIPTION_DELETED)
          { }

        /**
         * Creates a message with multiple deleted subscription IDs.
         * @param ids[in] The deleted subscription IDs.
         */
        SubscriptionsDeletedMessage(const SubscriptionIdList &ids)
            : executor::ProcessMessage(
            executor::ProcessMessage::MESSAGE_SUBSCRIPTION_DELETED),
              deleted_subscription_ids(ids)
          { }

        /**
         * Creates a message with a single deleted subscription ID.
         * @param id[in] The deleted subscription ID.
         */
        SubscriptionsDeletedMessage(const SubscriptionId id)
            : executor::ProcessMessage(
            executor::ProcessMessage::MESSAGE_SUBSCRIPTION_DELETED)
          { deleted_subscription_ids.push_back(id); }

        /**
         * @return The deleted subscription IDs.
         */
        const SubscriptionIdList &get_deleted_subscription_ids(void) const
          { return deleted_subscription_ids; }

    private:
        SubscriptionIdList deleted_subscription_ids; ///< Which subscriptions were deleted
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONSDELETEDMESSAGE_H
