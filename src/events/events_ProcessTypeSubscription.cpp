/*
 * events_ProcessTypeSubscription.cpp
 */

#include "events/events_ProcessTypeSubscription.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    bool process_type_subscription_match(
        const bool native_process,
        const ProcessTypeSubscription subscription)
    {
        if (subscription == PROCESS_SUB_NATIVE_AND_INTERPRETED)
        {
            // Always matches
            return true;
        }
        else
        {
            if (native_process)
            {
                return (subscription == PROCESS_SUB_NATIVE_ONLY);
            }
            else
            {
                return (subscription == PROCESS_SUB_INTERPRETED_ONLY);
            }
        }
    }
}
}