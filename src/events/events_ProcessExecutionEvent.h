/*
 * events_ProcessExecutionEvent.h
 */

#ifndef MUTGOS_EVENTS_PROCESSEXECUTIONEVENT_H
#define MUTGOS_EVENTS_PROCESSEXECUTIONEVENT_H

#include "dbtypes/dbtype_Id.h"

#include "events/events_Event.h"
#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents a process being created, completed, killed, etc.
     * This is expected to be a very frequent event.
     * Not all process states will be available.
     * All fields will be available for every event.
     * @see ProcessExecutionSubscriptionParams for details on which process
     * states will be provided.
     */
    class ProcessExecutionEvent : public Event
    {
    public:
        /**
         * Creates a ProcessExecutionEvent.
         * @param pid[in] The PID of the process.
         * @param exe_id[in] The database ID of the Entity containing
         * executable data for the process, or default if native.  If this is
         * default, the process is assumed native.
         * @param process_owner[in] The owner of the process, or default
         * if some sort of system process.
         * @param name[in] The name of the process.
         * @param state[in] The current process state.
         */
        ProcessExecutionEvent(
            const executor::PID pid,
            const dbtype::Id &exe_id,
            const dbtype::Id &process_owner,
            const std::string &name,
            const executor::ProcessInfo::ProcessState state)
              : Event(Event::EVENT_PROCESS_EXECUTION),
                process_id(pid),
                executable_id(exe_id),
                native_executable(exe_id.is_default()),
                owner_id(process_owner),
                process_name(name),
                process_state(state)
          { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ProcessExecutionEvent(const ProcessExecutionEvent &rhs)
            : Event(rhs),
              process_id(rhs.process_id),
              executable_id(rhs.executable_id),
              native_executable(rhs.native_executable),
              owner_id(rhs.owner_id),
              process_name(rhs.process_name),
              process_state(rhs.process_state)
        { }

        /**
         * Required virtual destructor.
         */
        virtual ~ProcessExecutionEvent()
          { }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return The process ID.
         */
        const executor::PID get_process_id(void) const
          { return process_id; }

        /**
         * @return If not native, the ID of the Entity that contains the
         * program code.
         */
        const dbtype::Id &get_executable_id(void) const
          { return executable_id; }

        /**
         * @return True if process is 'native' (not interpreted).  Native
         * processes do not have an executable ID.
         */
        const bool is_native(void) const
          { return native_executable; }

        /**
         * @return The ID of the owner of the process.
         */
        const dbtype::Id get_owner_id(void) const
          { return owner_id; }

        /**
         * @return The name of the process.
         */
        const std::string &get_process_name(void) const
          { return process_name; }

        /**
         * @return The current state of the process.
         */
        const executor::ProcessInfo::ProcessState get_process_state(void) const
          { return process_state; }

    private:
        const executor::PID process_id; ///< Process ID
        const dbtype::Id executable_id; ///< For non-native, ID of executable data
        const bool native_executable; ///< True if a native process
        const dbtype::Id owner_id; ///< Optional process owner
        const std::string process_name; ///< Process name
        const executor::ProcessInfo::ProcessState process_state; ///< Process state
    };
}
}

#endif //MUTGOS_EVENTS_PROCESSEXECUTIONEVENT_H
