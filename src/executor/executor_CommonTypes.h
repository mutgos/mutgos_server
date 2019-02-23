/*
 * executor_CommonTypes.h
 *
 * Types common to the Executor subsystem.
 * Used to prevent an inclusion loop.
 */

#ifndef MUTGOS_EXECUTOR_COMMONTYPES_H
#define MUTGOS_EXECUTOR_COMMONTYPES_H

#include <vector>
#include <list>

#include "osinterface/osinterface_OsTypes.h"

namespace mutgos
{
namespace executor
{
    /** Globally unique identifier of a process.  0 signifies invalid process.*/
    typedef osinterface::OsTypes::UnsignedInt PID;
    /** Globally unique process resource identifier. 0 is invalid. */
    typedef osinterface::OsTypes::UnsignedInt RID;
    /** Vector of PIDs */
    typedef std::vector<PID> ArrayOfPIDs;
    /** List of PIDs */
    typedef std::list<PID> ListOfPIDs;
    /** Vector of RIDs */
    typedef std::vector<RID> ArrayOfRIDs;
    /** List of RIDs */
    typedef std::list<RID> ListOfRIDs;
}
}

#endif //MUTGOS_EXECUTOR_COMMONTYPES_H
