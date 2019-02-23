/*
 * events_CommonTypes.h
 *
 * Types common to the Events subsystem.
 * Used to prevent an inclusion loop.
 */

#ifndef MUTGOS_EVENTS_COMMONTYPES_H
#define MUTGOS_EVENTS_COMMONTYPES_H

#include <vector>

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace events
{
    /** A subscription ID */
    typedef MG_UnsignedInt SubscriptionId;
    /** List of subscription IDs */
    typedef std::vector<SubscriptionId> SubscriptionIdList;
}
}

#endif //MUTGOS_EVENTS_COMMONTYPES_H
