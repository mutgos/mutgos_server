/*
 * events_SiteEventProcessor.cpp
 */

#include <string>
#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

#include "events_CommonTypes.h"
#include "events_SiteEventProcessor.h"
#include "events_SubscriptionProcessor.h"
#include "events_SubscriptionProcessorSupport.h"
#include "events_SubscriptionsSatisfied.h"

#include "events_SiteSubscriptionParams.h"
#include "events_SiteEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    SiteEventProcessor::SiteEventProcessor(SubscriptionData * const data_ptr)
        : SubscriptionProcessor(Event::EVENT_SITE, data_ptr)
    {
    }

    // ----------------------------------------------------------------------
    SiteEventProcessor::~SiteEventProcessor()
    {
        SubscriptionIdSet subscription_ids;

        get_all_subscription_ids(all_subscriptions, subscription_ids);

        for (SubscriptionIdSet::const_iterator id_iter =
            subscription_ids.begin();
             id_iter != subscription_ids.end();
             ++id_iter)
        {
            subscription_data->remove_subscription(*id_iter);
        }
    }

    // ----------------------------------------------------------------------
    void SiteEventProcessor::entity_deleted(const dbtype::Id &entity_id)
    {
        // Handled by process_event()
    }

    // ----------------------------------------------------------------------
    void SiteEventProcessor::site_deleted(const dbtype::Id::SiteIdType site_id)
    {
        // Handled by process_event()
    }

    // ----------------------------------------------------------------------
    void SiteEventProcessor::process_event(Event *const event_ptr)
    {
        if (event_ptr)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            if (event_ptr->get_event_type() != Event::EVENT_SITE)
            {
                LOG(error, "events", "process_event",
                    "Wrong event type attempted to be processed!  Type: " +
                    text::to_string(
                        event_ptr->get_event_type()));
            }
            else
            {
                // Right type of event, so cast and process it.
                SiteEvent * const site_ptr =
                    static_cast<SiteEvent *>(event_ptr);

                // Next, evaluate all subscriptions and create a duplicate-free
                // list of the ones which match.  The SubscriptionsSatisfied
                // tracker will do this for us.
                //
                SubscriptionsSatisfied<SiteEvent> tracker;
                evaluate_subscriptions(site_ptr, all_subscriptions, tracker);

                // Finally, call back all listeners whose subscriptions
                // matched.
                tracker.process_callbacks(site_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId SiteEventProcessor::add_subscription(
        const events::SubscriptionParams &subscription,
        const events::SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        if (subscription.get_type() !=
            SubscriptionParams::SUBSCRIPTION_SITE)
        {
            LOG(error, "events", "add_subscription",
                "Subscription is for a type we don't manage: " +
                text::to_string(subscription.get_type()));
        }
        else if (not subscription.validate())
        {
            // Not valid
            LOG(warning, "events", "add_subscription",
                "Subscription failed validation.  Not adding.");
        }
        else
        {
            SiteSubscriptionParams *site_params_ptr =
                static_cast<SiteSubscriptionParams *>(
                    subscription.clone());
            SubscriptionCallback *callback_ptr =
                new SubscriptionCallback(callback);

            id = subscription_data->add_subscription(
                site_params_ptr,
                Event::EVENT_CONNECTION,
                callback_ptr);
            callback_ptr->set_subscription_id(id);

            if (id)
            {
                // Added successfully, now add it internally
                //
                boost::unique_lock<boost::shared_mutex> write_lock(
                    subscription_lock);

                const SpecificSubscriptionCallback callback_info =
                    std::make_pair(site_params_ptr, callback_ptr);

                // Currently, all subscriptions of this type subscribe to
                // everything.
                add_subscription_to_list(
                    callback_info,
                    all_subscriptions);
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    bool SiteEventProcessor::remove_subscription(
        const SubscriptionId subscription_id)
    {
        bool success = true;
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);
        SubscriptionData::SubscriptionParamCallback subscription_info =
            subscription_data->get_subscription_info(subscription_id);

        if (not subscription_info.first)
        {
            // Not found.
            success = false;
        }
        else if (subscription_info.first->get_type() !=
                 SubscriptionParams::SUBSCRIPTION_SITE)
        {
            // Not a subscription we manage.
            LOG(error, "events", "remove_subscription",
                "Subscription ID is for a type we don't manage: " +
                text::to_string(subscription_id));
            success = false;
        }
        else
        {
            LOG(debug, "events", "remove_subscription",
                "Removing subscription ID " +
                text::to_string(subscription_id));

            // Found and is the right type.  Cast it and remove from our data
            // structures first.
            //
            SiteSubscriptionParams *params_ptr =
                static_cast<SiteSubscriptionParams *>(subscription_info.first);

            // Remove from all subscriptions
            delete_subscription_from_list(
                params_ptr,
                all_subscriptions);

            // Now remove it from subscription data
            //
            success = subscription_data->remove_subscription(subscription_id);
        }

        return success;
    }
}
}
