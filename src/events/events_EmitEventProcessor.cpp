/*
 * events_EmitEventProcessor.cpp
 */

#include <string>
#include "text/text_StringConversion.h"

#include "events/events_CommonTypes.h"
#include "logging/log_Logger.h"

#include "events/events_EmitEventProcessor.h"
#include "events/events_EmitSubscriptionParams.h"
#include "events/events_SubscriptionsSatisfied.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    EmitEventProcessor::EmitEventProcessor(SubscriptionData * const data_ptr)
      : SubscriptionProcessor(Event::EVENT_EMIT, data_ptr)
    {
    }

    // ----------------------------------------------------------------------
    EmitEventProcessor::~EmitEventProcessor()
    {
        SubscriptionIdSet subscription_ids;

        get_all_subscription_ids(source_subscriptions, subscription_ids);
        get_all_subscription_ids(target_subscriptions, subscription_ids);

        source_subscriptions.clear();
        target_subscriptions.clear();

        for (SubscriptionIdSet::const_iterator id_iter =
                subscription_ids.begin();
            id_iter != subscription_ids.end();
            ++id_iter)
        {
            subscription_data->remove_subscription(*id_iter);
        }
    }

    // ----------------------------------------------------------------------
    void EmitEventProcessor::entity_deleted(const dbtype::Id &entity_id)
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
            // There is no easy way to get the subscriptions that
            // list the entity in the 'ignored' field, and generally the
            // associated program will terminate anyway so it's not worth
            // searching everything.
            //
            SubscriptionList &source_subs =
                get_entity_subscriptions(entity_id, source_subscriptions);
            SubscriptionList &target_subs =
                get_entity_subscriptions(entity_id, target_subscriptions);

            for (SubscriptionList::iterator source_iter = source_subs.begin();
                source_iter != source_subs.end();
                ++source_iter)
            {
                subscription_callback_matched.insert(source_iter->second);
            }

            for (SubscriptionList::iterator target_iter = target_subs.begin();
                 target_iter != target_subs.end();
                 ++target_iter)
            {
                subscription_callback_matched.insert(target_iter->second);
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
    void EmitEventProcessor::site_deleted(const dbtype::Id::SiteIdType site_id)
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
            source_subscriptions,
            site_id,
            subscription_callbacks_matched);
        get_all_site_callbacks(
            target_subscriptions,
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
    void EmitEventProcessor::process_event(Event * const event_ptr)
    {
        if (event_ptr)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            if (event_ptr->get_event_type() != Event::EVENT_EMIT)
            {
                LOG(error, "events", "process_event",
                    "Wrong event type attempted to be processed!  Type: " +
                    text::to_string(
                        event_ptr->get_event_type()));
            }
            else
            {
                // Right type of event, so cast and process it.
                EmitEvent * const emit_ptr =
                    static_cast<EmitEvent *>(event_ptr);

                // First, find out potential subscriptions to evaluate.
                // There are likely to be duplicates.
                //
                const SubscriptionList &source_list =
                    get_entity_subscriptions(
                        emit_ptr->get_source(),
                        source_subscriptions);
                const SubscriptionList &target_list =
                    get_entity_subscriptions(
                        emit_ptr->get_target(),
                        target_subscriptions);

                // Next, evaluate all subscriptions and create a duplicate-free
                // list of the ones which match.  The SubscriptionsSatisfied
                // tracker will do this for us.
                //
                SubscriptionsSatisfied<EmitEvent> tracker;

                evaluate_subscriptions(emit_ptr, source_list, tracker);
                evaluate_subscriptions(emit_ptr, target_list, tracker);

                // Finally, call back all listeners whose subscriptions
                // matched.
                tracker.process_callbacks(emit_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId EmitEventProcessor::add_subscription(
        const SubscriptionParams &subscription,
        const SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        if (subscription.get_type() != SubscriptionParams::SUBSCRIPTION_EMIT)
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
            EmitSubscriptionParams *emit_params_ptr =
                static_cast<EmitSubscriptionParams *>(subscription.clone());
            SubscriptionCallback *callback_ptr =
                new SubscriptionCallback(callback);

            id = subscription_data->add_subscription(
                emit_params_ptr,
                Event::EVENT_EMIT,
                callback_ptr);
            callback_ptr->set_subscription_id(id);

            if (id)
            {
                // Added successfully, now add it internally
                //
                boost::unique_lock<boost::shared_mutex> write_lock(
                    subscription_lock);

                // The subscription guarantees at least one of these is set
                //
                const SpecificSubscriptionCallback callback_info =
                    std::make_pair(emit_params_ptr, callback_ptr);

                if (not emit_params_ptr->get_source().is_default())
                {
                    add_subscription_to_entity(
                        callback_info,
                        emit_params_ptr->get_source(),
                        source_subscriptions);
                }

                if (not emit_params_ptr->get_target().is_default())
                {
                    add_subscription_to_entity(
                        callback_info,
                        emit_params_ptr->get_target(),
                        target_subscriptions);
                }

                LOG(debug, "events", "add_subscription",
                    "Added subscription with ID: " +
                    text::to_string(id));
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    bool EmitEventProcessor::remove_subscription(
        const SubscriptionId subscription_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        const bool result = internal_remove_subscription(subscription_id);

        return result;
    }

    // ----------------------------------------------------------------------
    bool EmitEventProcessor::internal_remove_subscription(
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
            SubscriptionParams::SUBSCRIPTION_EMIT)
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
            EmitSubscriptionParams *params_ptr =
                static_cast<EmitSubscriptionParams *>(subscription_info.first);

            remove_entity_subscription(
                params_ptr->get_source(),
                params_ptr,
                source_subscriptions);

            remove_entity_subscription(
                params_ptr->get_target(),
                params_ptr,
                target_subscriptions);

            // Now remove it from subscription data
            //
            success = subscription_data->remove_subscription(subscription_id);
        }

        return success;
    }
}
}