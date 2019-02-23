/*
 * events_ConnectionEventProcessor.cpp
 */

#include <string>
#include "text/text_StringConversion.h"

#include "events/events_CommonTypes.h"
#include "logging/log_Logger.h"

#include "events/events_ConnectionEventProcessor.h"
#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"
#include "events/events_ConnectionSubscriptionParams.h"
#include "events/events_ConnectionEvent.h"
#include "events/events_SubscriptionsSatisfied.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    ConnectionEventProcessor::ConnectionEventProcessor(
        SubscriptionData * const data_ptr)
        : SubscriptionProcessor(Event::EVENT_CONNECTION, data_ptr)
    {
    }

    // ----------------------------------------------------------------------
    ConnectionEventProcessor::~ConnectionEventProcessor()
    {
        SubscriptionIdSet subscription_ids;

        get_all_subscription_ids(entity_subscriptions, subscription_ids);
        get_all_subscription_ids(site_subscriptions, subscription_ids);
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
    void ConnectionEventProcessor::entity_deleted(const dbtype::Id &entity_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        SubscriptionCallbackSet subscription_callback_matched;

        LOG(debug, "events", "entity_deleted",
            "Processing entity ID deleted: " + entity_id.to_string(true));

        // Temporary scope for the subscription references
        {
            // Get list of subscription IDs that reference the entity.
            // By definition, all these subscriptions reference the ID,
            // so they match.
            //
            SubscriptionList &entity_subs =
                get_entity_subscriptions(entity_id, entity_subscriptions);

            for (SubscriptionList::iterator sub_iter = entity_subs.begin();
                 sub_iter != entity_subs.end();
                 ++sub_iter)
            {
                subscription_callback_matched.insert(sub_iter->second);
            }
        }

        LOG(debug, "events", "entity_deleted",
            "Entity ID " + entity_id.to_string(true) + " had " +
            text::to_string(
                subscription_callback_matched.size())
            + " subscriptions.  Removing them now...");

        // For each subscription, have the callback inform the owner of its
        // deletion due to a referenced Entity going away, then delete
        // the subscription.  Note that after the subscription is deleted,
        // the callback pointer becomes invalid!!
        //
        for (SubscriptionCallbackSet::iterator subscription_iter =
               subscription_callback_matched.begin();
             subscription_iter != subscription_callback_matched.end();
             ++subscription_iter)
        {
            (*subscription_iter)->do_delete_callback();

            // This could be more efficient since we could delete the internal
            // data structure in a trivial manner, but this will guarantee all
            // traces are removed in the event this class becomes more complex.
            internal_remove_subscription(
                (*subscription_iter)->get_subscription_id());
        }
    }

    // ----------------------------------------------------------------------
    void ConnectionEventProcessor::site_deleted(
        const dbtype::Id::SiteIdType site_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        LOG(debug, "events", "site_deleted",
            "Processing site ID deleted: " +
            text::to_string(site_id));

        // By the time a site is deleted, there should be no active
        // subscriptions anyway, so efficiency is not a prime concern here.
        //
        SubscriptionCallbackSet subscription_callbacks_matched;

        get_all_site_callbacks(
            entity_subscriptions,
            site_id,
            subscription_callbacks_matched);
        get_all_site_callbacks(
            site_subscriptions,
            site_id,
            subscription_callbacks_matched);

        LOG(debug, "events", "site_deleted",
            "Site ID " +
            text::to_string(site_id)
            + " had " +
            text::to_string(
                subscription_callbacks_matched.size())
            + " subscriptions.  Removing them now...");

        // For each subscription, have the callback inform the owner of its
        // deletion due to a referenced Site/Entity going away, then delete
        // the subscription.  Note that after the subscription is deleted,
        // the callback pointer becomes invalid!!
        //
        for (SubscriptionCallbackSet::iterator subscription_iter =
               subscription_callbacks_matched.begin();
             subscription_iter != subscription_callbacks_matched.end();
             ++subscription_iter)
        {
            (*subscription_iter)->do_delete_callback();

            // This could be more efficient since we could delete the internal
            // data structure in a trivial manner, but this will guarantee all
            // traces are removed in the event this class becomes more complex.
            internal_remove_subscription(
                (*subscription_iter)->get_subscription_id());
        }
    }

    // ----------------------------------------------------------------------
    void ConnectionEventProcessor::process_event(Event * const event_ptr)
    {
        if (event_ptr)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            if (event_ptr->get_event_type() != Event::EVENT_CONNECTION)
            {
                LOG(error, "events", "process_event",
                    "Wrong event type attempted to be processed!  Type: " +
                    text::to_string(
                        event_ptr->get_event_type()));
            }
            else
            {
                // Right type of event, so cast and process it.
                ConnectionEvent * const connect_ptr =
                    static_cast<ConnectionEvent *>(event_ptr);

                // First, find out potential subscriptions to evaluate.
                // There are likely to be duplicates.
                //
                const SubscriptionList &entity_list =
                    get_entity_subscriptions(
                        connect_ptr->get_entity_id(),
                        entity_subscriptions);
                const SubscriptionList &site_list =
                    get_site_subscriptions(
                        connect_ptr->get_entity_id().get_site_id(),
                        site_subscriptions);

                // Next, evaluate all subscriptions and create a duplicate-free
                // list of the ones which match.  The SubscriptionsSatisfied
                // tracker will do this for us.
                //
                SubscriptionsSatisfied<ConnectionEvent> tracker;

                evaluate_subscriptions(connect_ptr, entity_list, tracker);
                evaluate_subscriptions(connect_ptr, site_list, tracker);
                evaluate_subscriptions(connect_ptr, all_subscriptions, tracker);

                // Finally, call back all listeners whose subscriptions
                // matched.
                tracker.process_callbacks(connect_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId ConnectionEventProcessor::add_subscription(
        const SubscriptionParams &subscription,
        const SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        if (subscription.get_type() !=
                SubscriptionParams::SUBSCRIPTION_CONNECTION)
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
            ConnectionSubscriptionParams *connect_params_ptr =
                static_cast<ConnectionSubscriptionParams *>(
                    subscription.clone());
            SubscriptionCallback *callback_ptr =
                new SubscriptionCallback(callback);

            id = subscription_data->add_subscription(
                connect_params_ptr,
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
                    std::make_pair(connect_params_ptr, callback_ptr);
                const dbtype::Entity::IdVector &entity_ids =
                    connect_params_ptr->get_entity_ids();
                const dbtype::Id::SiteIdVector &site_ids =
                    connect_params_ptr->get_site_ids();

                if (entity_ids.empty() and site_ids.empty())
                {
                    // Subscribes to all connections
                    add_subscription_to_list(
                        callback_info,
                        all_subscriptions);
                }
                else if (not entity_ids.empty())
                {
                    // Subscribes to Entities
                    //
                    for (dbtype::Entity::IdVector::const_iterator entity_iter =
                            entity_ids.begin();
                        entity_iter != entity_ids.end();
                        ++entity_iter)
                    {
                        add_subscription_to_entity(
                            callback_info,
                            *entity_iter,
                            entity_subscriptions);
                    }
                }
                else
                {
                    // Subscribes to sites
                    //
                    for (dbtype::Id::SiteIdVector::const_iterator site_iter =
                            site_ids.begin();
                        site_iter != site_ids.end();
                        ++site_iter)
                    {
                        add_subscription_to_site(
                            callback_info,
                            *site_iter,
                            site_subscriptions);
                    }
                }
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    bool ConnectionEventProcessor::remove_subscription(
        const SubscriptionId subscription_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        const bool result = internal_remove_subscription(subscription_id);

        return result;
    }

    // ----------------------------------------------------------------------
    bool ConnectionEventProcessor::internal_remove_subscription(
        const SubscriptionId subscription_id)
    {
        bool success = true;

        SubscriptionData::SubscriptionParamCallback subscription_info =
            subscription_data->get_subscription_info(subscription_id);

        if (not subscription_info.first)
        {
            // Not found.
            success = false;
        }
        else if (subscription_info.first->get_type() !=
                 SubscriptionParams::SUBSCRIPTION_CONNECTION)
        {
            // Not a subscription we manage.
            LOG(error, "events", "internal_remove_subscription",
                "Subscription ID is for a type we don't manage: " +
                text::to_string(subscription_id));
            success = false;
        }
        else
        {
            LOG(debug, "events", "internal_remove_subscription",
                "Removing subscription ID " +
                text::to_string(subscription_id));

            // Found and is the right type.  Cast it and remove from our data
            // structures first.
            //
            ConnectionSubscriptionParams *params_ptr =
                static_cast<ConnectionSubscriptionParams *>(
                    subscription_info.first);

            const dbtype::Entity::IdVector &entity_ids =
                params_ptr->get_entity_ids();
            const dbtype::Id::SiteIdVector &site_ids =
                params_ptr->get_site_ids();

            if (entity_ids.empty() and site_ids.empty())
            {
                // Remove from all connections
                delete_subscription_from_list(
                    params_ptr,
                    all_subscriptions);
            }
            else if (not entity_ids.empty())
            {
                // Remove from Entities
                //
                for (dbtype::Entity::IdVector::const_iterator entity_iter =
                    entity_ids.begin();
                     entity_iter != entity_ids.end();
                     ++entity_iter)
                {
                    remove_entity_subscription(
                        *entity_iter,
                        params_ptr,
                        entity_subscriptions);
                }
            }
            else
            {
                // Remove from sites
                //
                for (dbtype::Id::SiteIdVector::const_iterator site_iter =
                    site_ids.begin();
                     site_iter != site_ids.end();
                     ++site_iter)
                {
                    remove_site_subscription(
                        *site_iter,
                        params_ptr,
                        site_subscriptions);
                }
            }

            // Now remove it from subscription data
            //
            success = subscription_data->remove_subscription(subscription_id);
        }

        return success;
    }
}
}
