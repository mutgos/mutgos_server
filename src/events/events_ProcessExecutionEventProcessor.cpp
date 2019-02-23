/*
 * events_ProcessExecutionEventProcessor.cpp
 */

#include <string>
#include "text/text_StringConversion.h"

#include "events/events_CommonTypes.h"
#include "logging/log_Logger.h"

#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionProcessorSupport.h"
#include "events/events_SubscriptionsSatisfied.h"

#include "events/events_ProcessExecutionEventProcessor.h"
#include "events/events_ProcessExecutionSubscriptionParams.h"
#include "events/events_ProcessExecutionEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    ProcessExecutionEventProcessor::ProcessExecutionEventProcessor(
        SubscriptionData * const data_ptr)
        : SubscriptionProcessor(Event::EVENT_PROCESS_EXECUTION, data_ptr)
    {
    }

    // ----------------------------------------------------------------------
    ProcessExecutionEventProcessor::~ProcessExecutionEventProcessor()
    {
        SubscriptionIdSet subscription_ids;

        get_all_subscription_ids(entity_subscriptions, subscription_ids);
        get_all_subscription_ids(site_subscriptions, subscription_ids);
        get_all_subscription_ids(all_subscriptions, subscription_ids);

        for (PidSubscriptions::iterator pid_iter = pid_subscriptions.begin();
            pid_iter != pid_subscriptions.end();
            ++pid_iter)
        {
            get_all_subscription_ids(pid_iter->second, subscription_ids);
        }

        for (SubscriptionIdSet::const_iterator id_iter =
            subscription_ids.begin();
             id_iter != subscription_ids.end();
             ++id_iter)
        {
            subscription_data->remove_subscription(*id_iter);
        }
    }

    // ----------------------------------------------------------------------
    void ProcessExecutionEventProcessor::entity_deleted(
        const dbtype::Id &entity_id)
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
            internal_remove_subscription(
                (*subscription_iter)->get_subscription_id());
        }
    }

    // ----------------------------------------------------------------------
    void ProcessExecutionEventProcessor::site_deleted(
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
    void ProcessExecutionEventProcessor::process_event(Event *const event_ptr)
    {
        if (event_ptr)
        {
            // Subscriptions to remove due to them specifically watching
            // a PID that has gone away.
            SubscriptionList subs_remove;

            if (event_ptr->get_event_type() != Event::EVENT_PROCESS_EXECUTION)
            {
                LOG(error, "events", "process_event",
                    "Wrong event type attempted to be processed!  Type: " +
                    text::to_string(
                        event_ptr->get_event_type()));
            }
            else
            {
                boost::shared_lock<boost::shared_mutex> read_lock(
                    subscription_lock);

                // Right type of event, so cast and process it.
                ProcessExecutionEvent * const process_ptr =
                    static_cast<ProcessExecutionEvent *>(event_ptr);

                // First, find out potential subscriptions to evaluate.
                // There are likely to be duplicates.
                //
                const SubscriptionList &executable_list =
                    get_entity_subscriptions(
                        process_ptr->get_executable_id(),
                        entity_subscriptions);
                const SubscriptionList &executable_site_list =
                    get_site_subscriptions(
                        process_ptr->get_executable_id().get_site_id(),
                        site_subscriptions);
                const SubscriptionList &owner_list =
                    get_entity_subscriptions(
                        process_ptr->get_owner_id(),
                        entity_subscriptions);
                const SubscriptionList &owner_site_list =
                    get_site_subscriptions(
                        process_ptr->get_owner_id().get_site_id(),
                        site_subscriptions);
                PidSubscriptions::const_iterator pid_list_iter =
                    pid_subscriptions.find(process_ptr->get_process_id());


                // Next, evaluate all subscriptions and create a duplicate-free
                // list of the ones which match.  The SubscriptionsSatisfied
                // tracker will do this for us.
                //
                SubscriptionsSatisfied<ProcessExecutionEvent> tracker;

                evaluate_subscriptions(process_ptr, executable_list, tracker);
                evaluate_subscriptions(
                    process_ptr,
                    executable_site_list,
                    tracker);
                evaluate_subscriptions(process_ptr, owner_list, tracker);
                evaluate_subscriptions(
                    process_ptr,
                    owner_site_list,
                    tracker);
                evaluate_subscriptions(process_ptr, all_subscriptions, tracker);

                if (pid_list_iter != pid_subscriptions.end())
                {
                    evaluate_subscriptions(
                        process_ptr,
                        pid_list_iter->second,
                        tracker);

                    if (process_ptr->get_process_state() ==
                        executor::ProcessInfo::PROCESS_STATE_COMPLETED)
                    {
                        // PID is now invalid, so we need to remove
                        // related subscriptions.
                        // Copy because original will be modified as we
                        // unsubscribe.
                        subs_remove = pid_list_iter->second;
                    }
                }

                // Finally, call back all listeners whose subscriptions
                // matched.
                tracker.process_callbacks(process_ptr);
            }

            if (! subs_remove.empty())
            {
                boost::unique_lock<boost::shared_mutex> write_lock(
                    subscription_lock);

                // Delete the eligible PID-specific subscriptions and notify
                // owner.
                for (SubscriptionList::iterator subscription_iter =
                    subs_remove.begin();
                     subscription_iter != subs_remove.end();
                     ++subscription_iter)
                {
                    subscription_iter->second->do_delete_callback();
                    internal_remove_subscription(
                        subscription_iter->second->get_subscription_id());
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId ProcessExecutionEventProcessor::add_subscription(
        const SubscriptionParams &subscription,
        const SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        if (subscription.get_type() !=
            SubscriptionParams::SUBSCRIPTION_PROCESS_EXECUTION)
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
            ProcessExecutionSubscriptionParams *process_params_ptr =
                static_cast<ProcessExecutionSubscriptionParams *>(
                    subscription.clone());
            SubscriptionCallback *callback_ptr =
                new SubscriptionCallback(callback);

            id = subscription_data->add_subscription(
                process_params_ptr,
                Event::EVENT_ENTITY_CHANGED,
                callback_ptr);
            callback_ptr->set_subscription_id(id);

            if (id)
            {
                // Added successfully, now add it internally
                //
                boost::unique_lock<boost::shared_mutex> write_lock(
                    subscription_lock);

                const SpecificSubscriptionCallback callback_info =
                    std::make_pair(process_params_ptr, callback_ptr);

                if (process_params_ptr->get_process_id())
                {
                    // Watching for specific process ID.
                    pid_subscriptions[process_params_ptr->get_process_id()].
                        push_back(callback_info);
                }
                else
                {
                    const dbtype::Id &executable =
                        process_params_ptr->get_executable_id();
                    const dbtype::Id &owner =
                        process_params_ptr->get_owner_id();
                    bool specific = false;

                    if (not executable.is_default())
                    {
                        if (executable.is_entity_default())
                        {
                            // Executable site subscription
                            //
                            add_subscription_to_site(
                                callback_info,
                                executable.get_site_id(),
                                site_subscriptions);
                        }
                        else
                        {
                            // Executable entity subscription
                            //
                            add_subscription_to_entity(
                                callback_info,
                                executable,
                                entity_subscriptions);
                        }

                        specific = true;
                    }

                    if (not owner.is_default() and ((not specific) or
                        (owner != executable)))
                    {
                        if (owner.is_entity_default())
                        {
                            // Owner site subscription
                            //
                            add_subscription_to_site(
                                callback_info,
                                owner.get_site_id(),
                                site_subscriptions);
                        }
                        else
                        {
                            // Owner entity subscription
                            //
                            add_subscription_to_entity(
                                callback_info,
                                owner,
                                entity_subscriptions);
                        }

                        specific = true;
                    }

                    if (not specific)
                    {
                        // Did not end up in any of the other structures,
                        // so add it to all_subscriptions.
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
    bool ProcessExecutionEventProcessor::remove_subscription(
        const SubscriptionId subscription_id)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            subscription_lock);

        const bool result = internal_remove_subscription(subscription_id);

        return result;
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionEventProcessor::internal_remove_subscription(
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
                 SubscriptionParams::SUBSCRIPTION_ENTITY_CHANGED)
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
            ProcessExecutionSubscriptionParams *params_ptr =
                static_cast<ProcessExecutionSubscriptionParams *>(
                    subscription_info.first);

            // TODO implement
            if (params_ptr->get_process_id())
            {
                // Watching for specific process ID.
                //
                PidSubscriptions::iterator pid_iter =
                    pid_subscriptions.find(params_ptr->get_process_id());

                if (pid_iter == pid_subscriptions.end())
                {
                    LOG(error, "events", "internal_remove_subscription",
                        "Could not find PID " +
                            text::to_string(
                                params_ptr->get_process_id()) +
                        " for subscription ID " +
                        text::to_string(subscription_id));
                }
                else
                {
                    // Remove from PID, and then remove PID entry if empty.
                    //
                    delete_subscription_from_list(params_ptr, pid_iter->second);

                    if (pid_iter->second.empty())
                    {
                        pid_subscriptions.erase(pid_iter);
                    }
                }
            }
            else
            {
                const dbtype::Id &executable =
                    params_ptr->get_executable_id();
                const dbtype::Id &owner =
                    params_ptr->get_owner_id();
                bool specific = false;

                if (not executable.is_default())
                {
                    if (executable.is_entity_default())
                    {
                        // Executable site subscription
                        //
                        remove_site_subscription(
                            executable.get_site_id(),
                            params_ptr,
                            site_subscriptions);
                    }
                    else
                    {
                        // Executable entity subscription
                        //
                        remove_entity_subscription(
                            executable,
                            params_ptr,
                            entity_subscriptions);
                    }

                    specific = true;
                }

                if (not owner.is_default() and ((not specific) or
                    (owner != executable)))
                {
                    if (owner.is_entity_default())
                    {
                        // Owner site subscription
                        //
                        remove_site_subscription(
                            owner.get_site_id(),
                            params_ptr,
                            site_subscriptions);
                    }
                    else
                    {
                        // Owner entity subscription
                        //
                        remove_entity_subscription(
                            owner,
                            params_ptr,
                            entity_subscriptions);
                    }

                    specific = true;
                }

                if (not specific)
                {
                    // Did not end up in any of the other structures,
                    // so remove it from all_subscriptions.
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
}
}
