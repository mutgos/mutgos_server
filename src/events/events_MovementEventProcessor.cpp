/*
 * events_MovementEventProcessor.cpp
 */

#include <string>
#include "text/text_StringConversion.h"

#include "events/events_CommonTypes.h"
#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Entity.h"

#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"
#include "events/events_SubscriptionsSatisfied.h"

#include "events/events_MovementEventProcessor.h"
#include "events/events_MovementSubscriptionParams.h"
#include "events/events_MovementEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    MovementEventProcessor::MovementEventProcessor(
        SubscriptionData * const data_ptr)
        : SubscriptionProcessor(Event::EVENT_MOVEMENT, data_ptr)
    {
    }

    // ----------------------------------------------------------------------
    MovementEventProcessor::~MovementEventProcessor()
    {
        SubscriptionIdSet subscription_ids;

        get_all_subscription_ids(who_subscriptions, subscription_ids);
        get_all_subscription_ids(from_subscriptions, subscription_ids);
        get_all_subscription_ids(to_subscriptions, subscription_ids);
        get_all_subscription_ids(how_subscriptions, subscription_ids);
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
    void MovementEventProcessor::entity_deleted(const dbtype::Id &entity_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        SubscriptionCallbackSet subscription_callback_matched;

        LOG(debug, "events", "entity_deleted",
            "Processing entity ID deleted: " + entity_id.to_string(true));

        // Get list of subscription IDs that reference the entity.
        // By definition, all these subscriptions reference the ID,
        // so they match.
        //
        add_matched_for_deleted(
            who_subscriptions,
            entity_id,
            subscription_callback_matched);
        add_matched_for_deleted(
            from_subscriptions,
            entity_id,
            subscription_callback_matched);
        add_matched_for_deleted(
            to_subscriptions,
            entity_id,
            subscription_callback_matched);
        add_matched_for_deleted(
            how_subscriptions,
            entity_id,
            subscription_callback_matched);

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
    void MovementEventProcessor::site_deleted(
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
            who_subscriptions,
            site_id,
            subscription_callbacks_matched);
        get_all_site_callbacks(
            from_subscriptions,
            site_id,
            subscription_callbacks_matched);
        get_all_site_callbacks(
            to_subscriptions,
            site_id,
            subscription_callbacks_matched);
        get_all_site_callbacks(
            how_subscriptions,
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
    void MovementEventProcessor::process_event(Event * const event_ptr)
    {
        if (event_ptr)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            if (event_ptr->get_event_type() != Event::EVENT_MOVEMENT)
            {
                LOG(error, "events", "process_event",
                    "Wrong event type attempted to be processed!  Type: " +
                    text::to_string(
                        event_ptr->get_event_type()));
            }
            else
            {
                // Right type of event, so cast and process it.
                MovementEvent * const movement_ptr =
                    static_cast<MovementEvent  *>(event_ptr);

                // First, find out potential subscriptions to evaluate.
                // There are likely to be duplicates.
                //
                const SubscriptionList &who_list =
                    get_entity_subscriptions(
                        movement_ptr->get_who(),
                        who_subscriptions);
                const SubscriptionList &from_list =
                    get_entity_subscriptions(
                        movement_ptr->get_from(),
                        from_subscriptions);
                const SubscriptionList &to_list =
                    get_entity_subscriptions(
                        movement_ptr->get_to(),
                        to_subscriptions);
                const SubscriptionList &how_list =
                    get_entity_subscriptions(
                        movement_ptr->get_how(),
                        how_subscriptions);
                const SubscriptionList &site_list =
                    get_site_subscriptions(
                        movement_ptr->get_who().get_site_id(),
                        site_subscriptions);

                // Next, evaluate all subscriptions and create a duplicate-free
                // list of the ones which match.  The SubscriptionsSatisfied
                // tracker will do this for us.
                //
                SubscriptionsSatisfied<MovementEvent> tracker;

                evaluate_subscriptions(movement_ptr, who_list, tracker);
                evaluate_subscriptions(movement_ptr, from_list, tracker);
                evaluate_subscriptions(movement_ptr, to_list, tracker);
                evaluate_subscriptions(movement_ptr, how_list, tracker);
                evaluate_subscriptions(movement_ptr, site_list, tracker);
                evaluate_subscriptions(movement_ptr, all_subscriptions, tracker);

                // Finally, call back all listeners whose subscriptions
                // matched.
                tracker.process_callbacks(movement_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId MovementEventProcessor::add_subscription(
        const SubscriptionParams &subscription,
        const SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        if (subscription.get_type() !=
            SubscriptionParams::SUBSCRIPTION_MOVEMENT)
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
            MovementSubscriptionParams *movement_params_ptr =
                static_cast<MovementSubscriptionParams *>(
                    subscription.clone());
            SubscriptionCallback *callback_ptr =
                new SubscriptionCallback(callback);

            id = subscription_data->add_subscription(
                movement_params_ptr,
                Event::EVENT_MOVEMENT,
                callback_ptr);
            callback_ptr->set_subscription_id(id);

            if (id)
            {
                // Added successfully, now add it internally
                //
                boost::unique_lock<boost::shared_mutex> write_lock(
                    subscription_lock);

                const SpecificSubscriptionCallback callback_info =
                    std::make_pair(movement_params_ptr, callback_ptr);

                if (movement_params_ptr->get_site())
                {
                    // Subscribes to all Entity changes
                    add_subscription_to_site(
                        callback_info,
                        movement_params_ptr->get_site(),
                        site_subscriptions);
                }
                else
                {
                    bool specific = false;

                    // Subscribed to who
                    //
                    if (not movement_params_ptr->get_who().empty())
                    {
                        specific = true;

                        for (dbtype::Entity::IdVector::const_iterator
                                who_iter = movement_params_ptr->get_who().begin();
                            who_iter != movement_params_ptr->get_who().end();
                            ++who_iter)
                        {
                            add_subscription_to_entity(
                                callback_info,
                                *who_iter,
                                who_subscriptions);
                        }
                    }

                    // Subscribed to from
                    //
                    if (not movement_params_ptr->get_from().empty())
                    {
                        specific = true;

                        for (dbtype::Entity::IdVector::const_iterator
                                 from_iter = movement_params_ptr->get_from().begin();
                             from_iter != movement_params_ptr->get_from().end();
                             ++from_iter)
                        {
                            add_subscription_to_entity(
                                callback_info,
                                *from_iter,
                                from_subscriptions);
                        }
                    }

                    // Subscribed to 'to'
                    //
                    if (not movement_params_ptr->get_to().empty())
                    {
                        specific = true;

                        for (dbtype::Entity::IdVector::const_iterator
                                 to_iter = movement_params_ptr->get_to().begin();
                             to_iter != movement_params_ptr->get_to().end();
                             ++to_iter)
                        {
                            add_subscription_to_entity(
                                callback_info,
                                *to_iter,
                                to_subscriptions);
                        }
                    }

                    // Subscribed to how
                    //
                    if (not movement_params_ptr->get_movement_how().is_default())
                    {
                        specific = true;

                        add_subscription_to_entity(
                            callback_info,
                            movement_params_ptr->get_movement_how(),
                            how_subscriptions);
                    }

                    if (not specific)
                    {
                        add_subscription_to_list(
                            callback_info,
                            all_subscriptions);
                    }
                }
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    bool MovementEventProcessor::remove_subscription(
        const SubscriptionId subscription_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        const bool result = internal_remove_subscription(subscription_id);

        return result;
    }

    // ----------------------------------------------------------------------
    bool MovementEventProcessor::internal_remove_subscription(
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
                 SubscriptionParams::SUBSCRIPTION_MOVEMENT)
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
            MovementSubscriptionParams *params_ptr =
                static_cast<MovementSubscriptionParams *>(
                    subscription_info.first);


            if (params_ptr->get_site())
            {
                // Subscribes to all Entity changes
                remove_site_subscription(
                    params_ptr->get_site(),
                    params_ptr,
                    site_subscriptions);
            }
            else
            {
                bool specific = false;

                // Subscribed to who
                //
                if (not params_ptr->get_who().empty())
                {
                    specific = true;

                    for (dbtype::Entity::IdVector::const_iterator
                             who_iter = params_ptr->get_who().begin();
                         who_iter != params_ptr->get_who().end();
                         ++who_iter)
                    {
                        remove_entity_subscription(
                            *who_iter,
                            params_ptr,
                            who_subscriptions);
                    }
                }

                // Subscribed to from
                //
                if (not params_ptr->get_from().empty())
                {
                    specific = true;

                    for (dbtype::Entity::IdVector::const_iterator
                             from_iter = params_ptr->get_from().begin();
                         from_iter != params_ptr->get_from().end();
                         ++from_iter)
                    {
                        remove_entity_subscription(
                            *from_iter,
                            params_ptr,
                            from_subscriptions);
                    }
                }

                // Subscribed to 'to'
                //
                if (not params_ptr->get_to().empty())
                {
                    specific = true;

                    for (dbtype::Entity::IdVector::const_iterator
                             to_iter = params_ptr->get_to().begin();
                         to_iter != params_ptr->get_to().end();
                         ++to_iter)
                    {
                        remove_entity_subscription(
                            *to_iter,
                            params_ptr,
                            to_subscriptions);
                    }
                }

                // Subscribed to how
                //
                if (not params_ptr->get_movement_how().is_default())
                {
                    specific = true;

                    remove_entity_subscription(
                        params_ptr->get_movement_how(),
                        params_ptr,
                        how_subscriptions);
                }

                if (not specific)
                {
                    delete_subscription_from_list(
                        params_ptr,
                        all_subscriptions);
                }
            }

            // Now remove it from subscription data
            //
            success = subscription_data->remove_subscription(subscription_id);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void MovementEventProcessor::add_matched_for_deleted(
        SiteIdToEntitySubscriptions &entity_subscriptions,
        const dbtype::Id &entity_id,
        SubscriptionCallbackSet callback_set)
    {
        SubscriptionList &sub_list =
            get_entity_subscriptions(entity_id, entity_subscriptions);

        for (SubscriptionList::iterator sub_iter = sub_list.begin();
             sub_iter != sub_list.end();
             ++sub_iter)
        {
            callback_set.insert(sub_iter->second);
        }
    }
}
}
