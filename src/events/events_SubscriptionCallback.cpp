/*
 * events_SubscriptionCallback.cpp
 */

#include "logging/log_Logger.h"

#include "events/events_EventListener.h"
#include "events/events_EventMatchedMessage.h"
#include "events/events_SubscriptionsDeletedMessage.h"
#include "executor/executor_ExecutorAccess.h"

#include "events/events_SubscriptionCallback.h"


namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    bool SubscriptionCallback::do_callback(Event * const event_ptr) const
    {
        bool success = subscription_id;

        if (not success)
        {
            LOG(error, "events", "do_callback", "Subscription ID was not set!");
        }
        else if (not event_ptr)
        {
            success = false;
            LOG(error, "events", "do_callback", "event_ptr is null!");
        }
        else
        {
            if (pid_callback)
            {
                // Send a message
                //
                success = executor::ExecutorAccess::instance()->send_message(
                    pid_callback,
                    new EventMatchedMessage(subscription_id, event_ptr));
            }
            else if (listener_callback_ptr)
            {
                // Direct callback
                //
                listener_callback_ptr->subscribed_event_matched(
                    subscription_id,
                    *event_ptr);

                delete event_ptr;
            }
            else
            {
                // Nothing was specified, so we should fail since nothing
                // could be called.
                //
                success = false;

                LOG(warning, "events", "do_callback",
                    "Did not set either PID or listener callback.");

                delete event_ptr;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SubscriptionCallback::do_delete_callback(void) const
    {
        bool success = subscription_id;

        if (not success)
        {
            LOG(error, "events", "do_delete_callback",
                "Subscription ID was not set!");
        }
        else
        {
            if (pid_callback)
            {
                // Send a message
                //
                success = executor::ExecutorAccess::instance()->send_message(
                    pid_callback,
                    new SubscriptionsDeletedMessage(subscription_id));
            }
            else if (listener_callback_ptr)
            {
                // Direct callback
                //
                SubscriptionIdList subscriptions_deleted;
                subscriptions_deleted.push_back(subscription_id);

                listener_callback_ptr->subscription_deleted(
                    subscriptions_deleted);
            }
            else
            {
                // Nothing was specified, so we should fail since nothing
                // could be called.
                //
                success = false;

                LOG(warning, "events", "do_delete_callback",
                    "Did not set either PID or listener callback.");
            }
        }

        return success;
    }
}
}
