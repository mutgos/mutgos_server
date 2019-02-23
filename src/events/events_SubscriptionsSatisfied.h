/*
 * events_SubscriptionsSatisfied.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONSSATISFIED_H
#define MUTGOS_EVENTS_SUBSCRIPTIONSSATISFIED_H

#include <set>
#include <vector>

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class SubscriptionParams;
    class SubscriptionCallback;

    /**
     * Helper class used by subscription processors.  It will help them keep
     * track of what subscriptions have been satisfied by an event, and will
     * notify all the callbacks (message or pointer) when told.
     *
     * The class is partially needed because optimizations may cause certain
     * subscriptions to be checked more than once when processing an event.
     * Since actually checking if a subscription is satisfied can be expensive,
     * this class can keep track of what's been processed in a faster set
     * lookup.
     *
     * This class will never own any of the pointers provided, and is
     * designed for temporary instantiation, per event processed.  Pointers
     * must remain valid until the class is destructed.
     *
     * It is not thread safe.
     *
     * @tparam E The specific Event class the processor supports.
     */
    template <class E> class SubscriptionsSatisfied
    {
    public:
        /**
         * Constructor.
         */
        SubscriptionsSatisfied(void)
          { }

        /**
         * Destructor.
         */
        ~SubscriptionsSatisfied()
          { }

        /**
         * @param subscription_ptr[in] Pointer of subscription to check.
         * @return True if subscription has already been processed.
         */
        bool is_subscription_processed(
            SubscriptionParams * const subscription_ptr)
          { return subscriptions_processed.find(subscription_ptr) !=
                subscriptions_processed.end(); }

        /**
         * Adds a subscription that has been processed and satisfied by the
         * event.
         * @param subscription_ptr[in] The subscription that has been
         * satisfied.
         * @param callback_ptr[in] The associated callback to the subscription.
         */
        void add_subscription_satisfied(
            SubscriptionParams * const subscription_ptr,
            SubscriptionCallback * const callback_ptr)
        {
            subscriptions_processed.insert(subscription_ptr);
            callbacks_satisfied.push_back(callback_ptr);
        }

        /**
         * Adds a subscription that has been processed and not satisfied by the
         * event.
         * @param subscription_ptr[in] The subscription that has not been
         * satisfied.
         */
        void add_subscription_not_satisfied(
            SubscriptionParams * const subscription_ptr)
        {
            subscriptions_processed.insert(subscription_ptr);
        }

        /**
         * After all subscriptions have been processed, calling this will
         * notify all listeners whose subscriptions were satisfied
         * @param event_ptr[in] The event to notify the satisfied listeners
         * with.  The event will be copied as needed.
         */
        void process_callbacks(const E * const event_ptr)
        {
            for (CallbacksSatisfied::iterator iter =
                    callbacks_satisfied.begin();
                iter != callbacks_satisfied.end();
                ++iter)
            {
                (*iter)->do_callback(new E(*event_ptr));
            }
        }

    private:
        /** All the callbacks satisfied by this event */
        typedef std::vector<SubscriptionCallback *> CallbacksSatisfied;
        /** All the subscriptions satisfied by this event */
        typedef std::set<SubscriptionParams *> SubscriptionsProcessed;

        CallbacksSatisfied callbacks_satisfied; ///< All callbacks satisfied by an event
        SubscriptionsProcessed subscriptions_processed; ///< All subscriptions processed
    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONSSATISFIED_H
