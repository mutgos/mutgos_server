/*
 * executor_ProcessStats.h
 */

#ifndef MUTGOS_EXECUTOR_PROCESSSTATS_H
#define MUTGOS_EXECUTOR_PROCESSSTATS_H

#include <string>

#include "dbtypes/dbtype_Id.h"

#include "executor/executor_CommonTypes.h"
#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace executor
{
    // Container class that holds aggregated information about a process,
    // used primarily for display to a user.
    //
    class ProcessStats
    {
    public:
        /**
         * Constructor that sets everything.
         * @param pid[in] The PID the stats are about.
         * @param name[in] The name of the process.
         * @param owner[in] Who owns the process
         * @param executable[in] If not 'native', the softcode program entity
         * ID.  If 'native', this must be defaulted.
         * @param state[in] The current process state.
         */
        ProcessStats(
            const PID pid,
            const std::string &name,
            const dbtype::Id &owner,
            const dbtype::Id &executable,
            const ProcessInfo::ProcessState state)
          : my_pid(pid),
            process_name(name),
            owner_id(owner),
            executable_id(executable),
            process_state(state)
          { }

        /**
         * Default constructor.
         */
        ProcessStats(void)
            : my_pid(0),
              process_state(ProcessInfo::PROCESS_STATE_KILLED)
          { }

        /**
         * Destructor.
         */
        ~ProcessStats()
        { }

        /**
         * @return The PID the stats are about.
         */
        PID get_pid(void) const
          { return my_pid; }

        /**
         * @return The process name.
         */
        const std::string &get_name(void) const
          { return process_name; }

        /**
         * @return Entity ID that owns the process
         */
        const dbtype::Id &get_owner_id(void) const
          { return owner_id; }

        /**
         * @return If not 'native', the softcode program entity ID.  If
         * 'native', this must be defaulted.
         */
        const dbtype::Id &get_executable_id(void) const
          { return executable_id; }

        /**
         * @return The current process state.
         */
        ProcessInfo::ProcessState get_process_state(void) const
          { return process_state; }

    private:
        PID my_pid; ///< The PID the stats are about.
        std::string process_name; ///< Friendly name of process.
        dbtype::Id owner_id; ///< Who owns the process
        dbtype::Id executable_id;  ///< If not 'native', the softcode program entity ID
        ProcessInfo::ProcessState process_state; ///< The current process state
    };
}
}

#endif //MUTGOS_EXECUTOR_PROCESSSTATS_H
