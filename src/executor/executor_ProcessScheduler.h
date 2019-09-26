#ifndef MUTGOS_EXECUTOR_PROCESSSCHEDULER_H
#define MUTGOS_EXECUTOR_PROCESSSCHEDULER_H

#include <string>
#include <map>
#include <vector>
#include <list>
#include <chrono>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "osinterface/osinterface_TimeJumpListener.h"

#include "executor/executor_ProcessInfo.h"
#include "executor/executor_ProcessStats.h"
#include "executor/executor_Process.h"
#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace executor
{
    // Forward declarations
    class ProcessMessage;

    /**
     * Internal class that manages processes, determines which processes are
     * ready to run, queues up messages for delivery, and provides methods to
     * query and manipulate processes.
     * The scheduler does not directly execute processes; another class
     * interfaces with this one to get Processes and run them.  The
     * ExecutionInterface is responsible for any manipulation of process
     * states and resources not implemented here.
     *
     * This class is completely thread safe.  All methods can be called
     * at any time, however there is no guarantee of concurrency for every
     * method.
     */
    class ProcessScheduler : public osinterface::TimeJumpListener
    {
    public:
        typedef std::vector<ProcessStats> ProcessStatsVector;

        /**
         * Constructor.  This initializes all data structures.  When
         * returned, the scheduler is immediately ready for use.
         */
        ProcessScheduler(void);

        /**
         * Destructor.  Also cleans up any processes still known to the
         * scheduler by running shutdown().
         */
        virtual ~ProcessScheduler();

        /**
         * Cleans up all running processes and does not accept new processes.
         * When this method returns, all processes (including those actively
         * running) have been cleaned up.
         */
        void shutdown(void);

        /**
         * Called when a massive (more than a few seconds) system time jump has
         * been detected.
         * @param backwards[in] True if the jump was backwards.
         */
        virtual void os_time_has_jumped(bool backwards);

        /**
         * Adds the given process, but does not run it.
         * start_process() is used to run the process when ready.
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
            Process * const process_ptr);

        /**
         * Requests the given process be killed.  This is a 'polite' kill, with
         * the process being notified and given a chance to clean up.
         * @param pid[in] The PID of the process to kill.
         * @return True if successfully found the process and started to
         * kill it, or false if process not found.
         */
        bool kill_process(const PID pid);

        /**
         * Requests the given process be put into a 'suspended' state.  When
         * suspended, the process ignores all events and is never eligible
         * to be executed.  Normally this is not a useful call, especially
         * if a process is sleeping - it will miss its timer.
         * @param pid[in] The PID of the process to suspend.
         * @return True if able to suspend process, false if process not
         * found.
         */
        bool suspend_process(const PID pid);

        /**
         * Starts/Resumes a suspended or added process.
         * @param pid[in] The process to start/resume.
         * @return True if process now started (or was not suspended), false if
         * process not found.
         */
        bool start_process(const PID pid);

        /**
         * Sends a message to the given process (not from a resource).
         * @param pid[in] The PID of the process to send a message to.
         * @param message_ptr[in] The message to send.  The scheduler will
         * always take ownership of the pointer.
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
         * @param rid[in] The RID of the resource sending the message.  Optional
         * (may be invalid).
         * @param message_ptr[in] The message to send.  The scheduler will
         * always take ownership of the pointer.
         * @returnTrue if found the process and resource, and queued the message,
         * false if error (process or resource not found?).
         */
        bool send_message(
            const PID pid,
            const RID rid,
            ProcessMessage * const message_ptr);

        /**
         * @param id[in] The ID associated with one or more processes as the
         * owner.
         * @return All processes associated with the ID.  May be empty if none
         * or error.
         */
        ArrayOfPIDs get_pids_for_id(const dbtype::Id &id);

        /**
         * @param site_id[in] The ID associated with the site to get PIDs for.
         * @return All processes associated with the site.
         */
        ArrayOfPIDs get_pids_for_site(const dbtype::Id::SiteIdType site_id);

        /**
         * @param site_id[in] The ID associated with the site to get process
         * stats for.
         * @return All process stats associated with the site.
         */
        ProcessStatsVector get_process_stats_for_site(
            const dbtype::Id::SiteIdType site_id);

        /**
         * @param pid[in] The PID to get process stats for.
         * @return Process stats for the given PID, or an invalid/default stats
         * if PID not found.
         */
        ProcessStats get_process_stats(const PID pid);

        /**
         * Called by whoever actually executes a process, this method
         * will block for a period of time until any process is ready for
         * execution, at which point the process will be provided to the caller.
         * The caller is responsible for actually executing the process.
         *
         * ProcessInfo will contain the needed information about what the
         * executor should do with the process (execute, deliver message, etc).
         * When execution has completed, do not call this again; instead call
         * returned_from_execute() first.
         *
         * As this method also does housekeeping, it must be called frequently.
         * It is safe to call this on multiple threads.
         *
         * @param is_killed[out] Set to true when the returned process is being
         * killed.
         * @param is_shutting_down[out] Set to true when the process scheduler
         * is being shut down.  This is a hint to the caller it should also
         * terminate.
         * @return The ProcessInfo of the process to execute, or a null pointer
         * if nothing is ready to execute.  If null, please loop around and call
         * this method again.
         */
        ProcessInfo *get_next_execute(bool &is_killed, bool &is_shutting_down);

        /**
         * Called when a process has completed executing (for now, or forever).
         * Calling this makes the process eligible for execution again, and
         * does any cleanup based on the status.  Once this has been called for
         * a process, the executor can go back to calling get_next_execute().
         * @param process_ptr[in] Pointer of the ProcessInfo that has finished
         * executing.
         * @param status[in] Status returned from the process after executing.
         * Based on the status, the scheduler may call certain methods on the
         * process to get additional information.
         */
        void returned_from_execute(
            ProcessInfo * const process_ptr,
            const Process::ProcessStatus status);

        /**
         * Gets the next RID.
         * @param pid[in] The PID the next RID is for.
         * @return The next RID, or an invalid RID (0) if error.
         */
        RID get_next_rid(const PID pid);

        /**
         * Marks a RID gotten with get_next_rid() as no longer in use.
         * @param pid[in] The PID the next RID is for.
         * @param rid[in] The RID no longer in use.
         */
        void release_rid(const PID pid, const RID rid);

    private:
        /** Maps PID to the process */
        typedef std::map<PID, ProcessInfo *> PidToProcessMap;
        /** Array of processes */
        typedef std::vector<ProcessInfo *> ProcessVector;
        /** Maps Entity ID portion of an ID to its processes */
        typedef std::map<dbtype::Id::EntityIdType, ProcessVector>
            EntityIdToProcessMap;
        /** Maps Site ID portion of an ID to the processes */
        typedef std::map<dbtype::Id::SiteIdType, EntityIdToProcessMap>
            SiteIdToProcessesMap;
        /** Key is the absolute time to run the process */
        typedef std::multimap<std::chrono::steady_clock::time_point, ProcessInfo *>
            TimeQueue;
        /** Lock free queue of processes waiting to be executed */
        typedef boost::lockfree::queue<ProcessInfo *> RunQueue;
        /** Maps an active RID to an active PID. ProcessInfo can map in reverse */
        typedef std::map<RID, PID> RidToPidMap;

        /**
         * @param process_info_ptr[in] The process to get stats for.
         * @return Stats associated with the given process, or default
         * If pointer is null.
         */
        ProcessStats get_process_stats(ProcessInfo *process_info_ptr) const;

        /**
         * Marks a RID gotten with get_next_rid() as no longer in use.  This is
         * the internal version that assumes a lock has already been gotten.
         * @param pid[in] The PID the next RID is for.
         * @param rid[in] The RID no longer in use.
         */
        void release_rid_internal(const PID pid, const RID rid);

        /**
         * Ensures the given process has been scheduled for execution, updating
         * the process state as required.  If a process is already scheduled or
         * being executed, do nothing.  It will also check the pending killed
         * flag and adjust the process state as needed.
         * This assumes the class instance has already been locked.
         * @param process_ptr[in] The process to schedule.
         */
        void schedule_process(ProcessInfo * const process_ptr);

        /**
         * This assumes the class instance has already been locked.
         * @param state[in] The state of a process.
         * @return True if the process's state allows it to receive a message.
         */
        bool can_receive_messages(const ProcessInfo::ProcessState state) const;

        /**
         * Calls the appropriate method on the process to do cleanup internal
         * to the process, then deletes the process from all scheduler data
         * structures, and deletes ProcessInfo (and optionally the process
         * itself).
         * This can only be called when process has just finished executing or
         * after being killed; because of this, it does not cleanup the
         * timequeue used for sleeping processes.
         * This assumes the class instance has already been locked.
         * @param process_info_ptr[in] The process to be cleaned up.  This
         * pointer will be INVALID when this method returns.
         */
        void cleanup_process(ProcessInfo * const process_info_ptr);

        /**
         * Exclusively locks the class instance.
         * @return True if successfully locked.
         */
        bool lock(void);

        /**
         * Unlocks the class instance if it was lock()ed.
         */
        void unlock(void);

        /** The lock for using any attribute on this class. */
        boost::recursive_mutex process_lock;

        bool shutting_down; ///< True if shutting down (no new processes)

        /** All processes the scheduler is responsible for */
        PidToProcessMap all_processes;
        /** All processes, organized by the associated 'process owner' ID */
        SiteIdToProcessesMap all_processes_entity;

        // TODO Will need to handle semaphore overflow ( > 32,000) at some point
        /** Semaphore associated with the run queue so threads can easily block
            and wait for the next process to run.  Thead safe. */
        boost::interprocess::interprocess_semaphore process_run_queue_semaphore;
        /** The process run queue; anything on this queue is ready to be
            executed.  Thread safe. */
        RunQueue process_run_queue;

        /** Processes that are currently sleeping, organized by UTC time to wake
            them */
        TimeQueue process_timer_queue;


        // The RID and PID stuff is locked using process_lock

        PID next_pid; ///< Next available PID, but needs to be dupe checked
        RID next_rid; ///< Next available RID, but needs to be dupe checked
        const PID max_pid;  ///< Maximum PID identifier
        const RID max_rid;  ///< Maximum RID identifier
        RidToPidMap rid_to_pid; ///< Maps an active RID to a PID.
    };
}
}

#endif //MUTGOS_EXECUTOR_PROCESSSCHEDULER_H
