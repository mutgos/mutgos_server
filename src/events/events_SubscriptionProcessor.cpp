/*
 * events_SubscriptionProcessor.cpp
 */

#include "events_SubscriptionProcessor.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    SubscriptionProcessor::SubscriptionProcessor(
        const Event::EventType event_type,
        SubscriptionData * const data_ptr)
        : subscription_data(data_ptr),
          event_type_handled(event_type)
    {
    }

    // ----------------------------------------------------------------------
    SubscriptionProcessor::~SubscriptionProcessor()
    {
    }
}
}
