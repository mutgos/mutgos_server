#ifndef MUTGOS_EXECUTOR_EXECUTORACCESS_H
#define MUTGOS_EXECUTOR_EXECUTORACCESS_H

#include <vector>

#include <boost/thread/thread.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "osinterface/osinterface_TimeJumpListener.h"

#include "dbtypes/dbtype_Id.h"

#include "executor/executor_CommonTypes.h"
#include "executor/executor_ProcessScheduler.h"
#include "executor/executor_ProcessStats.h"

namespace mutgos
{
namespace executor
{
    // Forward declarations
    //
    class Process;
    class ProcessMessage;
    class ProcessScheduler;

    // TODO In execution, catch all exceptions and log, possibly safe shutdown.

    /**
     * Other namespaces can use this interface to interact with the Executor,
     * primarily to add processes/tasks to be executed when a thread is
     * available, or to send messages to a process.
     *
     * In other words, this is the primary interface to interact with the
     * process scheduler and executor.
     *
     * Processes that are actively running need to use the provided
     * ExecutionInterface to communicate with the Executor, instead.
     */
    class ExecutorAccess : public osinterface::TimeJumpListener
    {
    public:

        typedef ProcessScheduler::ProcessStatsVector ProcessStatsVector;

        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static ExecutorAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static ExecutorAccess *instance(void)
          { return singleton_ptr;  }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * Not thread safe.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Not thread safe.
         */
        void shutdown(void);

        /**
         * Called when a massive (more than a few seconds) system time jump has
         * been detected.
         * @param backwards[in] True if the jump was backwards.
         */
        virtual void os_time_has_jumped(bool backwards);

        /**
         * Adds the given process to the Executor.  The process will not
         * start until start_process() is explicitly called.
         * @param executable_id[in] An optional ID from something in the
         * database associated with the process, such as the program Entity.
         * May be a default (invalid) ID if a native (C++) process.
         * @param owner_id[in] An optional ID from something in the
         * database this process is running for.  It could be the process
         * 'owner' or something else.
         * @param process_ptr[in] Pointer to the process that will be executing.
         * The pointer must not be deleted until after the process ends, though
         * the process can specify that it be deleted automatically by the
         * Executor when finished.
         * @return The PID of the newly created process, or 0 if error.
         */
        PID add_process(
            const dbtype::Id &executable_id,
            const dbtype::Id &owner_id,
            Process *process_ptr);

        /**
         * Convenience method to add a process without any associated (invalid)
         * database IDs.
         * @param process_ptr[in] Pointer to the process that will be executing.
         * The pointer must not be deleted until after the process ends, though
         * the process can specify that it be deleted automatically by the
         * Executor when finished.
         * @return The PID of the newly created process, or 0 if error.
         */
        PID add_process(Process *process_ptr);

        /**
         * Requests the given process be killed.  This is a 'polite' kill, with
         * the process being notified and given a chance to clean up.
         * @param pid[in] The PID of the process to kill.
         * @return True if successfully found the process and started to
         * kill it, or false if process not found or not killable.
         */
        bool kill_process(const PID pid);

        /**
         * Requests the given process be put into a 'suspended' state.  When
         * suspended, the process ignores all events and is never eligible
         * to be executed.
         * @param pid[in] The PID of the process to suspend.
         * @return True if able to suspend process, false if process not
         * found.
         */
        bool suspend_process(const PID pid);

        /**
         * Starts/Resumes a suspended or added process.
         * @param pid[in] The process to start/resume.
         * @return True if process now started (or was not suspended), false if
         * process not found or being killed.
         */
        bool start_process(const PID pid);

        /**
         * Sends a message to the given process (not from a resource).
         * @param pid[in] The PID of the process to send a message to.
         * @param message_ptr[in] The message to send.  The Executor will
         * take ownership of the pointer success or fail.
         * @return True if found the process and queued the message, false
         * if error (process not found?).
         */
        bool send_message(
            const PID pid,
            ProcessMessage * const message_ptr);

        /**
         * Sends a message to the given process via a resource it was waiting
         * on.
         * @param pid[in] The PID of the process to send a message to.
         * @param rid[in] The RID of the resource sending the message.
         * @param message_ptr[in] The message to send.  The Executor will
         * take ownership of the pointer success or fail.
         * @returnTrue if found the process and resource, and queued the message,
         * false if error (process or resource not found?).
         */
        bool send_message(
            const PID pid,
            const RID rid,
            ProcessMessage * const message_ptr);

        /**
         * Cleans up (kills) processes associated with the given ID.
         * @param id[in] The ID associated with the processes to clean up.
         * @return True if any processes are cleaned up.
         */
        bool cleanup_processes(const dbtype::Id &id);

        /**
         * @param site_id[in] The site ID to get process stats for.
         * @return Detailed process stats for every process running at the
         * given site ID, or empty if none or not found.
         */
        ProcessStatsVector get_process_stats_for_site(
            const dbtype::Id::SiteIdType site_id);

        // TODO Add call for forcible removal of RID

        // TODO Later on, queries to get lists of process for @ps, etc
        // TODO: Add support for event subscriptions

    private:

        typedef std::vector<boost::thread *> Executors;

        /**
         * Private singleton constructor.
         */
        ExecutorAccess(void);

        /**
         * Private singleton destructor.
         */
        ~ExecutorAccess();

        static ExecutorAccess *singleton_ptr; ///< Singleton pointer

        ProcessScheduler process_scheduler;
        Executors process_executors;
    };
}
}

#endif // MUTGOS_EXECUTOR_EXECUTORACCESS_H
