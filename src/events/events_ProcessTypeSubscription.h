/*
 * events_ProcessTypeSubscription.h
 */

#ifndef MUTGOS_EVENTS_PROCESSTYPESUBSCRIPTIONENUM_H
#define MUTGOS_EVENTS_PROCESSTYPESUBSCRIPTIONENUM_H

namespace mutgos
{
namespace events
{
    /**
     * A reusable enum for indicating, in subscriptions, which type of
     * process(es) the subscription is interested in.
     */
    enum ProcessTypeSubscription
    {
        /** Wants native processes only */
        PROCESS_SUB_NATIVE_ONLY,
        /** Wants interpreted processes only */
        PROCESS_SUB_INTERPRETED_ONLY,
        /** Wants native and interpreted processes (everything) */
        PROCESS_SUB_NATIVE_AND_INTERPRETED
    };

    /**
     * Helper method that determines if a process type matches the subscription.
     * @param native_process[in] True if the process being matched is native,
     * false if interpreted.
     * @param subscription[in] The subscription for process types to
     * evalute/match against.
     * @return True if the process type matches the subscription, false if not.
     */
    bool process_type_subscription_match(
        const bool native_process,
        const ProcessTypeSubscription subscription);
}
}

#endif //MUTGOS_EVENTS_PROCESSTYPESUBSCRIPTIONENUM_H
