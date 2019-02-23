#ifndef MUTGOS_EXECUTOR_PROCESS_H
#define MUTGOS_EXECUTOR_PROCESS_H

#include <string>
#include <list>

#include "osinterface/osinterface_OsTypes.h"
#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace executor
{
    // Forward declarations.
    class ProcessMessage;
    class ProcessServices;

    /**
     * A pure virtual (interface) class that anything wanting to be ran
     * by the ExecutorInterface must implement.
     * Only one method will be called by the Executor at a time.
     */
    class Process
    {
    public:
        enum ProcessStatus
        {
            /** Process is completely finished.  Do not schedule again and
                initiate cleanup. */
            PROCESS_STATUS_FINISHED,
            /** Process wants to sleep for a time, but also get events.
                Note that any event coming in will cancel the sleep */
            PROCESS_STATUS_SLEEP,
            /** Process wants to wait until it gets a message before executing
                again*/
            PROCESS_STATUS_WAIT_MESSAGE,
            /** Process wants to be suspended - ignores all events.
                Note that the process will have no way to resume itself! */
            PROCESS_STATUS_SUSPENDED,
            /** Process is blocked on it's resources and cannot proceed at this
                time */
            PROCESS_STATUS_BLOCKED,
            /** Process has encountered an error condition and must finished */
            PROCESS_STATUS_ERROR,
            /** Process has more work to do.  Put back in queue. */
            PROCESS_STATUS_EXECUTE_MORE
        };

        /** User readable string(s), one line per element, concerning why the
         * process errored out */
        typedef std::list<std::string> ErrorMessageText;

        /**
         * Constructor.
         */
        Process(void);

        /**
         * Required virtual destructor for pure virtual classes.
         */
        virtual ~Process();

        /**
         * Optionally implemented by any class who wants to be a process and
         * executed by the ExecutorInterface, this is called when the Process
         * is initially added to the executor via ExecutorAccess::add_process().
         * It provides the Process a chance to add resources, etc before
         * execution officially begins.
         * @param pid[in] The PID of the newly added Process.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         */
        virtual void process_added(
            const PID pid,
            ProcessServices &services);

        /**
         * Implemented by any class who wants to be a process and be executed by
         * the ExecutorInterface, this is called when the class instance is
         * allowed to 'do work'.  When in this method, the process is being
         * exclusively run on one of potentially many threads.  The process cannot
         * execute forever; when it hits a blocking condition or has executed
         * 'long enough', it must yield to other processes by returning from this
         * method.
         * Even when waiting on messages or resources, in rare situations this
         * method may be called anyway.  If so, just return the correct
         * status to go back to waiting for messages.
         * @param pid[in] The PID of the process being executed.  Useful in case
         * the same class instance executes different processes (note that in the
         * case of running multiple processes, this method may be called
         * reentrantly).
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @return A status indicating if the process has completed its work or
         * would like to be called again later.
         */
        virtual ProcessStatus process_execute(
            const PID pid,
            ProcessServices &services) =0;

        /**
         * Similar to the standard process_execute(), this variant is called
         * in response to a message to be processed.
         * Optional because not all processes handle messages.
         * @param pid[in] The PID of the process being executed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @param message[in] The message to be processed.
         * @return The status indicating if process has completed its work.
         * @see process_execute(const PID pid);
         */
        virtual ProcessStatus process_execute(
            const PID pid,
            ProcessServices &services,
            ProcessMessage &message);

        /**
         * Similar to the standard process_execute(), this variant is called
         * in response to a message to be processed, originating from a
         * resource.
         * Optional because not all processes handle messages from resources.
         * @param pid[in] The PID of the process being executed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @param rid[in] The RID of the resource who sent the message.
         * @param message[in] The message to be processed.
         * @return The status indicating if process has completed its work.
         * @see process_execute(const PID pid);
         */
        virtual ProcessStatus process_execute(
            const PID pid,
            ProcessServices &services,
            const RID rid,
            ProcessMessage &message);

        /**
         * Similar to the standard process_execute(), this variant is called
         * in response to a resource asking for itself to be removed from
         * this process.
         * Optional because not all processes handle messages from resources.
         * @param pid[in] The PID of the process being executed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @param rid[in] The RID of the resource being removed.
         * @return The status indicating if process has completed its work.
         * @see process_execute(const PID pid);
         */
        // TODO Not currently implemented, will need to tweak later.
        virtual ProcessStatus process_execute(
            const PID pid,
            ProcessServices &services,
            const RID rid);

        /**
         * This might be called many times, even while the process is
         * executing.
         * @param pid[in] The PID of the process whose name is desired.
         * @return The name of the process.  Must not be empty.
         */
        virtual std::string process_get_name(const PID pid) =0;

        /**
         * Implemented by subclass.
         * @param pid[in] The PID of the process being queried.
         * @return True if class instance is to be deleted (freed) after
         * the process has been killed or finished executing.
         */
        virtual bool process_delete_when_finished(const PID pid) =0;

        /**
         * Called only when the process_execute() method returns PROCESS_SLEEP.
         * @param pid[in]  The PID of the process being queried.
         * @return The time the process should sleep, in ms.  Please note this
         * is an approximate value.
         */
        virtual osinterface::OsTypes::UnsignedInt process_get_sleep_time(
            const PID pid);

        /**
         * Called only after process returns PROCESS_ERROR.
         * @param pid[in] The PID of the process for the error text.
         * @return If process_execute() indicates the process errored out, this
         * will return the reason(s) for the error.
         */
        virtual ErrorMessageText process_get_error_text(const PID pid);

        /**
         * Called when the process has been forcibly killed.
         * This will never be called when process_execute() is active.
         * This may be called before any execute() call has been made, if
         * the system is shutting down.
         * @param pid[in] The PID of the process being killed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         */
        virtual void process_killed(
            const PID pid,
            ProcessServices &services);

        /**
         * Called when the the executor has finished with the process.  This is
         * called at some point after process_execute() has returned PROCESS_FINISHED
         * or PROCESS_ERROR.  It is also called after process_killed().
         * Typically this is where any resources are cleaned up.
         * If process_delete_when_finished() returns true, this is the last
         * call before destruction of the class instance.
         * @param pid[in] The PID of the finished process.
         */
        virtual void process_finished(const PID pid);
    };
}
}

#endif //MUTGOS_EXECUTOR_PROCESS_H
