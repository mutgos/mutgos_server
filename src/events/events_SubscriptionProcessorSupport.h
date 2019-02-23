/*
 * events_SubscriptionProcessorSupport.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONPROCESSORSUPPORT_H
#define MUTGOS_EVENTS_SUBSCRIPTIONPROCESSORSUPPORT_H

#include <map>
#include <set>

#include "dbtypes/dbtype_Id.h"

#include "events/events_SubscriptionCallback.h"
#include "events/events_SubscriptionsSatisfied.h"

#include "events/events_SubscriptionData.h"

namespace mutgos
{
namespace events
{
    /**
     * A class used by subscription processors that contains common
     * data structures and algorithms.  It is only used internally by
     * subscription processors.
     * @tparam S The specific SubscriptionParam class the processor supports.
     * @tparam E The specific Event class the processor supports.
     */
    template <class S, class E> class SubscriptionProcessorSupport
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~SubscriptionProcessorSupport()
        { }

    protected:
        /** Set of subscription IDs */
        typedef std::set<SubscriptionId> SubscriptionIdSet;
        /** First is the subscription pointer (specific subscription class),
            second is the callback pointer */
        typedef typename std::pair<S *, SubscriptionCallback *>
            SpecificSubscriptionCallback;
        /** Set of subscription callback pointers, mostly used when
            a referenced site or entity is being deleted */
        typedef std::set<SubscriptionCallback *> SubscriptionCallbackSet;
        /** List of subscription pointers.  Used only by subclasses. */
        typedef std::vector<SpecificSubscriptionCallback> SubscriptionList;
        /** Maps site ID to a list of subscription pointers */
        typedef std::map<dbtype::Id::SiteIdType, SubscriptionList>
            SiteIdToSubscriptionsList;
        /** Maps Entity ID to a subscription list */
        typedef std::map<dbtype::Id, SubscriptionList>
            EntityIdToSubscriptionList;
        /** Maps Site ID to Entity ID to a subscription list */
        typedef std::map<dbtype::Id::SiteIdType, EntityIdToSubscriptionList>
            SiteIdToEntitySubscriptions;

        /**
         * Constructor.
         */
        SubscriptionProcessorSupport(void)
          { }

        /**
         * Removes the given subscription for a list (actually a vector) of
         * subscriptions in an optimal way.
         * It is assumed write locking has been done.
         * @param subscription_ptr[in] The subscription pointer to remove.
         * @param list[in,out] The list to remove from.  The list will be
         * modified.
         * @return True if subscription found and removed, false otherwise.
         */
        bool delete_subscription_from_list(
            const S * const subscription_ptr,
            SubscriptionList &list) const
        {
            bool found = false;

            // Find the subscription, if present, and put the last subscription
            // in place and then delete the last slot, to avoid the
            // reshuffling.
            //
            const size_t size = list.size();
            size_t position = 0;

            for (; position < size; ++position)
            {
                if (list[position].first == subscription_ptr)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                if ((position + 1) == size)
                {
                    // Subscription is at the end.  Simply delete the last
                    // entry.
                    list.erase(list.begin() + position);
                }
                else
                {
                    // Subscription is not at the end.  Put end Subscription
                    // in this subscription's place.
                    //
                    list[position] = list[size - 1];
                    list.erase(list.begin() + (size - 1));
                }
            }

            return found;
        }

        /**
         * Adds a subscription to a site-wide subscription data structure.
         * @param subscription_data[in] The subscription to add.  This will
         * be (shallow) copied.
         * @param site_id[in] The site ID to add the subscription to.
         * @param site_data[out] The lookup table to add the subscription to.
         */
        void add_subscription_to_site(
            const SpecificSubscriptionCallback &subscription_data,
            const dbtype::Id::SiteIdType site_id,
            SiteIdToSubscriptionsList &site_data) const
        {
            site_data[site_id].push_back(subscription_data);
        }

        /**
         * Adds a subscription to an entity-specific subscription data
         * structure.
         * @param subscription_data[in] The subscription to add.  This will be
         * (shallow) copied.
         * @param entity_id[in] The entity ID to add the subscription to.
         * @param site_entity_data[out] The lookup table to add the
         * subscription to.
         */
        void add_subscription_to_entity(
            const SpecificSubscriptionCallback &subscription_data,
            const dbtype::Id &entity_id,
            SiteIdToEntitySubscriptions &site_entity_data) const
        {
            site_entity_data[entity_id.get_site_id()][entity_id].push_back(
                subscription_data);
        }

        /**
         * Adds a subscription to a subscription list.
         * @param subscription_data[in] The subscription to add.  This will
         * be (shallow) copied.
         * @param list[out] The list to add the subscription to.
         */
        void add_subscription_to_list(
            const SpecificSubscriptionCallback &subscription_data,
            SubscriptionList &list)
        {
            list.push_back(subscription_data);
        }

        /**
         * Gets all subscription IDs in site_entity_data.
         * This is typically only done during destruction of the processor.
         * @param site_entity_data[in] Where to get the subscription IDs from.
         * @param subscription_ids[out] The subscription IDs contained in
         * site_entity_data.  The set will not be cleared before use.
         */
        void get_all_subscription_ids(
            const SiteIdToEntitySubscriptions &site_entity_data,
            SubscriptionIdSet &subscription_ids) const
        {
            for (typename SiteIdToEntitySubscriptions::const_iterator site_iter =
                    site_entity_data.begin();
                site_iter != site_entity_data.end();
                ++site_iter)
            {
                // For each site.
                //
                for (typename EntityIdToSubscriptionList::const_iterator
                        entity_iter = site_iter->second.begin();
                    entity_iter != site_iter->second.end();
                    ++entity_iter)
                {
                    // For each Entity
                    get_all_subscription_ids(entity_iter->second, subscription_ids);
                }
            }
        }

        /**
         * Gets all subscription IDs in site_data.
         * This is typically only done during destruction of the processor.
         * @param site_data[in] Where to get the subscription IDs from.
         * @param subscription_ids[out] The subscription IDs contained in
         * site_data.  The set will not be cleared before use.
         */
        void get_all_subscription_ids(
            const SiteIdToSubscriptionsList &site_data,
            SubscriptionIdSet &subscription_ids) const
        {
            for (typename SiteIdToSubscriptionsList::const_iterator site_iter =
                     site_data.begin();
                 site_iter != site_data.end();
                 ++site_iter)
            {
                // For each site.
                get_all_subscription_ids(site_iter->second, subscription_ids);
            }
        }

        /**
         * Gets all subscription IDs in subscription_list.
         * This is typically only done during destruction of the processor.
         * @param list_data[in] Where to get the subscription IDs from.
         * @param subscription_ids[out] The subscription IDs contained in
         * list_data.  The set will not be cleared before use.
         */
        void get_all_subscription_ids(
            const SubscriptionList &list_data,
            SubscriptionIdSet &subscription_ids) const
        {
            for (typename SubscriptionList::const_iterator
                     subscription_iter = list_data.begin();
                 subscription_iter != list_data.end();
                 ++subscription_iter)
            {
                // For each Subscription
                subscription_ids.insert(
                    subscription_iter->second->get_subscription_id());
            }
        }

        /**
         * For every Entity from the provided site, add its callback to
         * subscription_callbacks.
         * @param site_entity_data[in] Where to get the Entity subscriptions
         * from.
         * @param site_id[in] The site id whose subscriptions are desired.
         * @param subscription_callbacks[out] The callbacks contained
         * in site_entity_data for site_id.  The set will not be cleared
         * before use.
         */
        void get_all_site_callbacks(
            const SiteIdToEntitySubscriptions &site_entity_data,
            const dbtype::Id::SiteIdType site_id,
            SubscriptionCallbackSet subscription_callbacks)
        {
            typename SiteIdToEntitySubscriptions::const_iterator site_iter =
                site_entity_data.find(site_id);

            if (site_iter != site_entity_data.end())
            {
                // For each Entity in the site, add all subscriptions to the
                // matched set.
                //
                for (typename EntityIdToSubscriptionList::const_iterator
                         entity_iter = site_iter->second.begin();
                     entity_iter != site_iter->second.end();
                     ++entity_iter)
                {
                    get_all_callbacks(
                        entity_iter->second,
                        subscription_callbacks);
                }
            }
        }

        /**
         * For every subscription for the provided site, add its callback to
         * subscription_callbacks
         * @param site_subscription_data[in] Where to get site subscriptions
         * from.
         * @param site_id[in] The site id whose subscriptions are desired.
         * @param subscription_callbacks[out] The callbacks contained
         * in site_subscription_data for site_id.  The set will not be cleared
         * before use.
         */
        void get_all_site_callbacks(
            const SiteIdToSubscriptionsList &site_subscription_data,
            const dbtype::Id::SiteIdType site_id,
            SubscriptionCallbackSet subscription_callbacks)
        {
            typename SiteIdToSubscriptionsList::const_iterator
                site_iter = site_subscription_data.find(site_id);

            if (site_iter != site_subscription_data.end())
            {
                get_all_callbacks(site_iter->second, subscription_callbacks);
            }
        }

        /**
         * For every subscription provided in the list, add its callback to
         * subscription_callbacks.
         * @param subscription_data[in] Where to get the subscriptions from.
         * @param subscription_callbacks[out] The callbacks contained
         * in subscription_data.  The set will not be cleared before use.
         */
        void get_all_callbacks(
            const SubscriptionList &subscription_data,
            SubscriptionCallbackSet subscription_callbacks)
        {
            for (typename SubscriptionList::const_iterator
                     subscription_iter = subscription_data.begin();
                 subscription_iter != subscription_data.end();
                 ++subscription_iter)
            {
                subscription_callbacks.insert(subscription_iter->second);
            }
        }

        /**
         * Gets subscriptions from a site-wide subscription data structure.
         * @param site_id[in] The site ID to get subscription data for.
         * @param site_data[in] The lookup table to search.
         * @return A reference to the found subscription list, or an
         * empty list if not found.  If list is empty, do NOT modify it.
         */
        SubscriptionList &get_site_subscriptions(
            const dbtype::Id::SiteIdType site_id,
            SiteIdToSubscriptionsList &site_data)
        {
            typename SiteIdToSubscriptionsList::iterator iter =
                site_data.find(site_id);

            if (iter == site_data.end())
            {
                // Not found.
                return empty_subscription_list;
            }
            else
            {
                return iter->second;
            }

            return empty_subscription_list;
        }

        /**
         * Gets subscriptions from an entity-specific subscription data
         * structure.
         * @param entity_id[in] The entity ID to get subscription data for.
         * @param site_entity_data[in] The lookup table to search.
         * @return A reference to the found subscription list, or an
         * empty list if not found.  If list is empty, do NOT modify it.
         */
        SubscriptionList &get_entity_subscriptions(
            const dbtype::Id &entity_id,
            SiteIdToEntitySubscriptions &site_entity_data)
        {
            typename SiteIdToEntitySubscriptions::iterator site_iter =
                site_entity_data.find(entity_id.get_site_id());

            if (site_iter == site_entity_data.end())
            {
                // Site not found.
                return empty_subscription_list;
            }
            else
            {
                // Found site, look for Entity.
                //
                typename EntityIdToSubscriptionList::iterator entity_iter =
                    site_iter->second.find(entity_id);

                if (entity_iter == site_iter->second.end())
                {
                    // Entity not found.
                    return empty_subscription_list;
                }
                else
                {
                    // Found Entity, return list.
                    return entity_iter->second;
                }
            }

            return empty_subscription_list;
        }

        /**
         * Removes a subscription from a site-wide subscription data structure.
         * @param site_id[in] The site ID containing the subscription.
         * @param subscription_ptr[in] The subscription to remove.
         * @param site_data[in,out] The lookup table to delete the subscription
         * from.  If deleting the subscription causes the list to be empty,
         * the site will be removed.
         * @return True if subscription was found and removed.
         */
        bool remove_site_subscription(
            const dbtype::Id::SiteIdType site_id,
            S * const subscription_ptr,
            SiteIdToSubscriptionsList &site_data)
        {
            bool found = false;

            SubscriptionList &subscriptions =
                get_site_subscriptions(site_id, site_data);

            if (not subscriptions.empty())
            {
                // Found the site, now find and remove the subscription.
                // If the deletion makes the list empty, then remove the site.
                //
                found = delete_subscription_from_list(
                    subscription_ptr,
                    subscriptions);

                if (subscriptions.empty())
                {
                    site_data.erase(site_id);
                }
            }

            return found;
        }

        /**
         * Removes a subscription from an entity-specific subscription data
         * structure.
         * @param entity_id[in] The entity ID containing the subscription.
         * @param subscription_ptr[in] The subscription to remove.
         * @param site_entity_data[in,out] The lookup table to delete the
         * subscription from.  If deleting the subscription causes the list
         * to be empty, the Entity will be removed.  In turn, if deleting the
         * Entity causes the site to be empty, the site will be removed.
         * @return True if subscription was found and removed.
         */
        bool remove_entity_subscription(
            const dbtype::Id &entity_id,
            S * const subscription_ptr,
            SiteIdToEntitySubscriptions &site_entity_data) const
        {
            bool found = false;

            typename SiteIdToEntitySubscriptions::iterator site_iter =
                site_entity_data.find(entity_id.get_site_id());

            if (site_iter != site_entity_data.end())
            {
                // Found site, look for Entity.
                //
                typename EntityIdToSubscriptionList::iterator entity_iter =
                    site_iter->second.find(entity_id);

                if (entity_iter != site_iter->second.end())
                {
                    // Found Entity, remove subscription.
                    // If the deletion makes the list empty, then remove the
                    // Entity and possibly the site.
                    //
                    found = delete_subscription_from_list(
                        subscription_ptr,
                        entity_iter->second);

                    if (entity_iter->second.empty())
                    {
                        site_iter->second.erase(entity_iter);

                        if (site_iter->second.empty())
                        {
                            site_entity_data.erase(site_iter);
                        }
                    }
                }
            }

            return found;
        }

        /**
         * Given a list of subscriptions, evaluate them (if not already
         * evaluated) and add the results to the SubscriptionsSatisfied tracker.
         * @param event_ptr[in] The event to evaluate.
         * @param subscriptions[in] The subscriptions to evaluate.
         * @param tracker[in,out] The SubscriptionsSatisfied tracker, which
         * is used throughout the evaluation of a single event.
         */
        void evaluate_subscriptions(
            E * const event_ptr,
            const SubscriptionList &subscriptions,
            SubscriptionsSatisfied<E> &tracker) const
        {
            for (typename SubscriptionList::const_iterator iter =
                    subscriptions.begin();
                iter != subscriptions.end();
                ++iter)
            {
                evaluate_subscription(event_ptr, *iter, tracker);
            }
        }

        /**
         * Given a subscription, evaluate (if not already evaluated) and add
         * the results to the SubscriptionsSatisfied tracker.
         * @param event_ptr[in] The event to evaluate.
         * @param subscriptions[in] The subscription to evaluate.
         * @param tracker[in,out] The SubscriptionsSatisfied tracker, which
         * is used throughout the evaluation of a single event.
         * @return True if subscription has newly matched the event, false
         * otherwise.
         */
        inline bool evaluate_subscription(
            E * const event_ptr,
            const SpecificSubscriptionCallback &subscription,
            SubscriptionsSatisfied<E> &tracker) const
        {
            if (not tracker.is_subscription_processed(subscription.first))
            {
                if (subscription.first->is_match(event_ptr))
                {
                    tracker.add_subscription_satisfied(
                        subscription.first,
                        subscription.second);
                    return true;
                }
                else
                {
                    tracker.add_subscription_not_satisfied(subscription.first);
                    return false;
                }
            }

            return false;
        }

    private:
        SubscriptionList empty_subscription_list; ///< Used when something is not found.
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONPROCESSORSUPPORT_H
