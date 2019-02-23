/*
 * comm_CommonTypes.h
 *
 * Types common to the Comm subsystem.
 * Used to prevent an inclusion loop.
 */

#ifndef MUTGOS_COMM_COMMONTYPES_H
#define MUTGOS_COMM_COMMONTYPES_H

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace comm
{
    /** A numerical ID for a channel, unique per connection */
    typedef MG_UnsignedInt ChannelId;

    /** Serial number for a message to/from a client.  0 is always invalid. */
    typedef MG_UnsignedInt MessageSerialId;

    /** A numerical ID for a session, unique per session */
    typedef MG_UnsignedInt SessionId;
}
}

#endif //MUTGOS_COMM_COMMONTYPES_H
