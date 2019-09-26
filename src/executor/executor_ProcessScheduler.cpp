#include <string>
#include <map>
#include <vector>
#include <list>
#include <unistd.h>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "text/text_StringConversion.h"
#include <boost/date_time/microsec_time_clock.hpp>

#include "osinterface/osinterface_OsTypes.h"

#include "logging/log_Logger.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "executor/executor_ProcessScheduler.h"
#include "executor/executor_ProcessServices.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ProcessResource.h"
#include "executor/executor_ProcessInfo.h"
#include "executor/executor_ProcessStats.h"
#include "dbtypes/dbtype_Id.h"

#include "events/events_ProcessExecutionEvent.h"
#include "events/events_EventAccess.h"

namespace mutgos
{
namespace executor
{
    typedef std::vector<ProcessResource *> ArrayOfResources;

    // ----------------------------------------------------------------------
    ProcessScheduler::ProcessScheduler(void)
      : shutting_down(false),
        process_run_queue_semaphore(0),
        process_run_queue(1),
        next_pid(1),
        next_rid(1),
        max_pid(30000), // TODO Avoids full semaphore for now on Linux??
        max_rid(std::numeric_limits<RID>::max() - 1)
    {
    }

    // ----------------------------------------------------------------------
    ProcessScheduler::~ProcessScheduler()
    {
        shutdown();
        // All data structures should be empty; nothing to cleanup.
    }

    // ----------------------------------------------------------------------
    void ProcessScheduler::shutdown(void)
    {
        LOG(info, "executor", "shutdown", "shutdown() called");

        if (lock())
        {
            if (not shutting_down)
            {
                LOG(info, "executor", "shutdown", "Killing all processes...");

                // Not already shutting down, so kill everything
                //
                shutting_down = true;

                for (PidToProcessMap::const_iterator iter = all_processes.begin();
                     iter != all_processes.end();
                     ++iter)
                {
                    // Killing a process does not immediately remove it from
                    // the map so this is safe.
                    kill_process(iter->first);
                }
            }

            unlock();

            LOG(info, "executor", "shutdown",
                "Waiting for all processes to end...");

            // Wait for all processes to exit
            //
            bool all_killed = false;

            while (not all_killed)
            {
                sleep(1);

                if (lock())
                {
                    all_killed = all_processes.empty();
                    unlock();
                }
            }

            LOG(info, "executor", "shutdown", "Shutdown complete.");
        }
    }

    // ----------------------------------------------------------------------
    void ProcessScheduler::os_time_has_jumped(bool backwards)
    {
        if (backwards)
        {
            // Forward jumping is OK - it means a quicker poll.  Backwards
            // could mean too long a poll.

            // Trigger the semaphore to break loose one thread.  This will
            // ensure timed sleeps will still get processed.  The other
            // threads will eventually get woken up as stuff comes in.
            //
            // Note this could in theory still result in a shutdown hang if the
            // time went backwards too far and the server was then
            // immediately shut down.  In practice this is unlikely to happen;
            // one thread will be kicked loose, and the others should become
            // active over a short period as stuff comes in from users.
            // In the worse case, the operator can manually kill MUTGOS after
            // a period of waiting, and there will be no corruption.

            process_run_queue_semaphore.post();
        }
    }

    // ----------------------------------------------------------------------
    PID ProcessScheduler::add_process(
        const dbtype::Id &executable_id,
        const dbtype::Id &owner_id,
        Process * const process_ptr)
    {
        PID pid = 0;
        ProcessInfo *process_info_ptr = 0;

        if (not process_ptr)
        {
            LOG(error, "executor", "add_process",
                "process_ptr is null!");
        }
        else if (lock())
        {
            if (not shutting_down)
            {
                // Find the next available PID
                //
                if (all_processes.size() >= max_pid)
                {
                    LOG(fatal, "executor", "add_process",
                        "No more PIDs available!");
                }
                else
                {
                    while (not pid)
                    {
                        if (all_processes.find(next_pid) == all_processes.end())
                        {
                            // Found next PID.
                            pid = next_pid;
                        }

                        ++next_pid;

                        if (next_pid >= max_pid)
                        {
                            next_pid = 1;
                        }
                    }
                }

                if (pid)
                {
                    // Create the process info data structure
                    process_info_ptr =
                        new ProcessInfo(pid, process_ptr, executable_id, owner_id);

                    // Add to data structures on the scheduler, but do not
                    // run the process yet.
                    //
                    all_processes.insert(std::make_pair(pid, process_info_ptr));
                    all_processes_entity[owner_id.get_site_id()]
                      [owner_id.get_entity_id()].push_back(process_info_ptr);
                }
            }

            unlock();
        }

        // These have to be done outside the lock to prevent a deadlock.
        //
        if (process_info_ptr)
        {
            // Call process back so it can add any resources up front.
            //
            ProcessServices services(&process_info_ptr, this);
            process_ptr->process_added(pid, services);

            // Let interested listeners know about the process creation
            //
            events::EventAccess::instance()->publish_event(
                new events::ProcessExecutionEvent(
                    pid,
                    executable_id,
                    owner_id,
                    process_ptr->process_get_name(pid),
                    ProcessInfo::PROCESS_STATE_CREATED));
        }

        return pid;
    }

    // ----------------------------------------------------------------------
    bool ProcessScheduler::kill_process(const PID pid)
    {
        bool result = false;

        if (lock())
        {
            PidToProcessMap::iterator process_iter = all_processes.find(pid);

            if (process_iter != all_processes.end())
            {
                // Found the process.  Set it to be killed, and schedule its
                // death, if it's not already in the process of going away.
                //
                const ProcessInfo::ProcessState state =
                    process_iter->second->get_process_state();

                if ((state != ProcessInfo::PROCESS_STATE_KILLED) and
                    (state != ProcessInfo::PROCESS_STATE_COMPLETED))
                {
                    process_iter->second->set_pending_killed(true);
                    schedule_process(process_iter->second);
                }

                result = true;
            }

            unlock();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ProcessScheduler::suspend_process(const PID pid)
    {
        // TODO After demo: This is incomplete - it needs to store the previous state.

        bool result = false;

        if (lock())
        {
            if (not shutting_down)
            {
                PidToProcessMap::iterator process_iter = all_processes.find(pid);

                if (process_iter != all_processes.end())
                {
                    // Found it!  If process is currently executing or scheduled
                    // to be executed, then mark it pending, otherwise set the
                    // state right away since it's not even on the queue.
                    //
                    concurrency::WriterLockToken token(*process_iter->second);

                    switch (process_iter->second->get_process_state(token))
                    {
                        case ProcessInfo::PROCESS_STATE_READY:
                        case ProcessInfo::PROCESS_STATE_EXECUTING:
                        case ProcessInfo::PROCESS_STATE_KILLED:
                        {
                            // Currently in queue or executing, so mark it pending
                            process_iter->second->set_pending_suspended(
                                true,
                                token);
                            break;
                        }

                        default:
                        {
                            // Not executing or in queue, so set state directly.
                            process_iter->second->set_pending_suspended(
                                false,
                                token);
                            process_iter->second->set_process_state(
                                ProcessInfo::PROCESS_STATE_SUSPENDED,
                                token);
                            break;
                        }
                    }

                    result = true;
                }
            }

            unlock();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ProcessScheduler::start_process(const PID pid)
    {
        bool result = false;

        if (lock())
        {
            if (not shutting_down)
            {
                PidToProcessMap::iterator process_iter = all_processes.find(pid);

                if (process_iter != all_processes.end())
                {
                    // Found the process.  Start it if not started or suspended.
                    //
                    switch (process_iter->second->get_process_state())
                    {
                        case ProcessInfo::PROCESS_STATE_CREATED:
                        case ProcessInfo::PROCESS_STATE_SUSPENDED:
                        {
                            schedule_process(process_iter->second);
                            break;
                        }

                        default:
                        {
                            // Process is already started.
                            break;
                        }
                    }

                    result = true;
                }
            }

            unlock();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ProcessScheduler::send_message(
        const PID pid,
        ProcessMessage * const message_ptr)
    {
        return send_message(pid, 0, message_ptr);
    }

    // ----------------------------------------------------------------------
    bool ProcessScheduler::send_message(
        const PID pid,
        const RID rid,
        ProcessMessage * const message_ptr)
    {
        bool result = false;

        if (not message_ptr)
        {
            LOG(error, "executor", "send_message",
                "message_ptr is null!");
        }
        else if (not pid)
        {
            LOG(error, "executor", "send_message",
                "pid is invalid!");
        }
        else if (lock())
        {
            // Find the process, append the message, and schedule process for
            // execution so message can be processed.
            //
            PidToProcessMap::iterator process_iter = all_processes.find(pid);
            bool need_schedule = false;

            if (process_iter == all_processes.end())
            {
                LOG(error, "executor", "send_message",
                    "PID " + text::to_string(pid)
                    + " not found.  Cannot send message.");
            }
            else
            {
                concurrency::WriterLockToken token(*process_iter->second);

                const ProcessInfo::ProcessState process_state =
                    process_iter->second->get_process_state(token);

                // See if process can receive messages
                //
                if (not can_receive_messages(process_state))
                {
                    LOG(debug, "executor", "send_message",
                        "PID " + text::to_string(pid)
                        + " cannot receive messages right now.  Skipping.");
                }
                else
                {
                    result = process_iter->second->add_message(
                        message_ptr,
                        rid,
                        token);

                    if (not rid)
                    {
                        // Determine if we can schedule process for execution.
                        switch (process_state)
                        {
                            case ProcessInfo::PROCESS_STATE_BLOCKED:
                            case ProcessInfo::PROCESS_STATE_CREATED:
                            {
                                // Process cannot be scheduled if in these
                                // states.  If blocked, for instance,
                                // since this message does not provide a RID,
                                // the wait list cannot be reduced and so it
                                // cannot be made ready from this message.
                                break;
                            }

                            default:
                            {
                                // All other states that are possible here can
                                // be scheduled.
                                need_schedule = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // If we're using a RID, see if we can schedule it based on
                        // how many resources are still waiting.
                        //
                        const bool done_blocking =
                            process_iter->second->blocked_resources_empty(token);

                        // Determine if we can schedule process for execution.
                        switch (process_state)
                        {
                            case ProcessInfo::PROCESS_STATE_BLOCKED:
                            {
                                // Only schedule if everything blocking the
                                // process has sent a message.
                                need_schedule = done_blocking;
                                break;
                            }

                            case ProcessInfo::PROCESS_STATE_CREATED:
                            {
                                // Process cannot be scheduled if
                                // uninitialized.
                                break;
                            }

                            default:
                            {
                                // All other states that are possible here can
                                // be scheduled.
                                need_schedule = true;
                                break;
                            }
                        }
                    }
                }
            }

            // Schedule the process if the code above determined it's nessecary.
            //
            if (result and need_schedule)
            {
                schedule_process(process_iter->second);
            }

            unlock();

        }

        if (not result)
        {
            delete message_ptr;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    ArrayOfPIDs ProcessScheduler::get_pids_for_id(
        const dbtype::Id &id)
    {
        ArrayOfPIDs pids;

        if (lock())
        {
            SiteIdToProcessesMap::const_iterator site_iter =
                all_processes_entity.find(id.get_site_id());

            if (site_iter != all_processes_entity.end())
            {
                // Site exists, might have a match.
                EntityIdToProcessMap::const_iterator entity_iter =
                    site_iter->second.find(id.get_entity_id());

                if (entity_iter != site_iter->second.end())
                {
                    // Found it!  Add all the PIDs.
                    //
                    for (ProcessVector::const_iterator process_iter =
                           entity_iter->second.begin();
                         process_iter != entity_iter->second.end();
                         ++process_iter)
                    {
                        pids.push_back((*process_iter)->get_pid());
                    }
                }
            }

            unlock();
        }

        return pids;
    }

    // ----------------------------------------------------------------------
    ArrayOfPIDs ProcessScheduler::get_pids_for_site(
        const dbtype::Id::SiteIdType site_id)
    {
        ArrayOfPIDs pids;

        if (lock())
        {
            SiteIdToProcessesMap::const_iterator site_iter =
                all_processes_entity.find(site_id);

            if (site_iter != all_processes_entity.end())
            {
                // Site exists, get all PIDs.
                for (EntityIdToProcessMap::const_iterator entity_iter =
                        site_iter->second.begin();
                    entity_iter != site_iter->second.end();
                    ++entity_iter)
                {
                    for (ProcessVector::const_iterator process_iter =
                             entity_iter->second.begin();
                         process_iter != entity_iter->second.end();
                         ++process_iter)
                    {
                        pids.push_back((*process_iter)->get_pid());
                    }
                }
            }

            unlock();
        }

        return pids;
    }

    // ----------------------------------------------------------------------
    ProcessScheduler::ProcessStatsVector ProcessScheduler::
        get_process_stats_for_site(const dbtype::Id::SiteIdType site_id)
    {
        ProcessStatsVector stats;

        if (lock())
        {
            SiteIdToProcessesMap::const_iterator site_iter =
                all_processes_entity.find(site_id);

            if (site_iter != all_processes_entity.end())
            {
                // Site exists, get all PIDs.
                for (EntityIdToProcessMap::const_iterator entity_iter =
                    site_iter->second.begin();
                     entity_iter != site_iter->second.end();
                     ++entity_iter)
                {
                    for (ProcessVector::const_iterator process_iter =
                        entity_iter->second.begin();
                         process_iter != entity_iter->second.end();
                         ++process_iter)
                    {
                        stats.push_back(get_process_stats(*process_iter));
                    }
                }
            }

            unlock();
        }

        return stats;
    }

    // ----------------------------------------------------------------------
    ProcessStats ProcessScheduler::get_process_stats(const PID pid)
    {
        ProcessStats stats;

        if (lock())
        {
            PidToProcessMap::iterator process_iter = all_processes.find(pid);

            if (process_iter != all_processes.end())
            {
                stats = get_process_stats(process_iter->second);
            }

            unlock();
        }


        return stats;
    }

    // ----------------------------------------------------------------------
    ProcessInfo *ProcessScheduler::get_next_execute(
        bool &is_killed,
        bool &is_shutting_down)
    {
        ProcessInfo *process_info_ptr = 0;
        bool shutdown_caller = false;

        // First, schedule anything due that is currently sleeping and needs
        // to wake up.
        //
        if (lock())
        {
            const std::chrono::steady_clock::time_point current_time =
                std::chrono::steady_clock::now();
            shutdown_caller = shutting_down;

            bool keep_looking = not process_timer_queue.empty();

            while (keep_looking)
            {
                // Using this instead of a for loop because scheduling
                // a process invalidates the iterator.
                TimeQueue::iterator timer_iter = process_timer_queue.begin();

                if (timer_iter == process_timer_queue.end())
                {
                    // No more timers at all.
                    keep_looking = false;
                }
                else if (timer_iter->first > current_time)
                {
                    // The remaining entries in the map are beyond the current
                    // time and cannot be scheduled yet.
                    keep_looking = false;
                }
                else
                {
                    // Time to wakeup the process.
                    schedule_process(timer_iter->second);
                }
            }

            unlock();
        }

        // Next, wait for a little bit on the semaphore
        //
        bool got_sem = false;

        try
        {
            // Wait 3 seconds for semaphore to be posted.
            // TODO One day this should wait up to the next timed sleep so it won't overshoot
            got_sem = process_run_queue_semaphore.timed_wait(
                boost::posix_time::microsec_clock::universal_time()
                + boost::posix_time::seconds(3));
        }
        catch (...)
        {
            LOG(fatal, "executor", "get_next_execute",
                "Exception while doing timed_wait() on semaphore!");
        }

        if (got_sem)
        {
            // We have a process to run, get it.
            process_run_queue.pop(process_info_ptr);
        }

        // Fill in parameters and update state as needed.
        //
        if (process_info_ptr)
        {
            concurrency::WriterLockToken token(*process_info_ptr);

            // Let caller know if process is being killed.
            is_killed = process_info_ptr->get_process_state(token) ==
                ProcessInfo::PROCESS_STATE_KILLED;

            // Set process to executing.
            process_info_ptr->set_process_state(
                ProcessInfo::PROCESS_STATE_EXECUTING,
                token);
        }
        else
        {
            // No processes were found, so let caller know if we're actually
            // shutting down.
            is_shutting_down = shutdown_caller;
        }

        return process_info_ptr;
    }

    // ----------------------------------------------------------------------
    void ProcessScheduler::returned_from_execute(
        ProcessInfo * const process_ptr,
        const Process::ProcessStatus status)
    {
        if (not process_ptr)
        {
            LOG(error, "executor", "returned_from_execute",
                "process_ptr is null!");
            return;
        }

        if (not lock())
        {
            LOG(fatal, "executor", "returned_from_execute",
                "Could not lock!");
            return;
        }

        bool reschedule = false;
        bool suspended = false;
        bool cleaned_up = false;

        // First, if there are any states that always cleanup, do those first.
        //
        switch (status)
        {
            case Process::PROCESS_STATUS_FINISHED:
            {
                // Process is done.  Do cleanup.
                //
                cleanup_process(process_ptr);
                cleaned_up = true;
                break;
            }

            case Process::PROCESS_STATUS_ERROR:
            {
                if (log::Logger::can_log(warning))
                {
                    const Process::ErrorMessageText errors =
                        process_ptr->get_process()->process_get_error_text(
                            process_ptr->get_pid());
                    std::string error_text = "PID "
                       + text::to_string(process_ptr->get_pid())
                       + ", name " + process_ptr->get_process()->process_get_name(
                          process_ptr->get_pid())
                       + ", finished with error:\n";

                    // Log error info before cleaning up.
                    //
                    for (Process::ErrorMessageText::const_iterator error_iter =
                            errors.begin();
                        error_iter != errors.end();
                        ++error_iter)
                    {
                        error_text += *error_iter + "\n";
                    }

                    LOG(warning, "executor", "returned_from_execute", error_text);
                }

                events::EventAccess::instance()->publish_event(
                    new events::ProcessExecutionEvent(
                        process_ptr->get_pid(),
                        process_ptr->get_db_executable_id(),
                        process_ptr->get_db_owner_id(),
                        process_ptr->get_process()->process_get_name(
                            process_ptr->get_pid()),
                        ProcessInfo::PROCESS_STATE_KILLED));

                cleanup_process(process_ptr);
                cleaned_up = true;
                break;
            }

            default:
            {
                // Other states handled later
            };
        }

        if (not cleaned_up)
        {
            // Process 'pending' flags and 'suspended' status, reschedule if
            // indicated.
            //
            concurrency::WriterLockToken token(*process_ptr);

            // Set the process to 'scheduling', since that's what we're
            // doing.  It will also let schedule_process() know it's no
            // longer executing and safe for rescheduling.
            process_ptr->set_process_state(
                ProcessInfo::PROCESS_STATE_SCHEDULING,
                token);

            if (process_ptr->get_pending_killed(token))
            {
                // Process needs to be killed.  Schedule that.
                // Calling schedule_process() will take care of the pending
                // killed flag.
                reschedule = true;
            }
            else if ((status == Process::PROCESS_STATUS_SUSPENDED) or
                process_ptr->get_pending_suspended(token))
            {
                // Process wants to be suspended.  Set state and don't
                // reschedule.
                process_ptr->set_pending_suspended(false, token);
                process_ptr->set_process_state(
                    ProcessInfo::PROCESS_STATE_SUSPENDED,
                    token);
                // Suspended processes can't get messages.
                process_ptr->clear_all_messages(token);
                suspended = true;
            }

            // If process wants to execute again or has messages waiting,
            // reschedule immediately.
            //
            if ((not suspended) and (not reschedule) and
                ((status == Process::PROCESS_STATUS_EXECUTE_MORE) or
                    (not process_ptr->messages_empty(token))))
            {
                reschedule = true;
            }

            // If process wants to sleep, put in timequeue and put in sleeping
            // state.
            //
            if ((not suspended) and (not reschedule) and
                (status == Process::PROCESS_STATUS_SLEEP))
            {
                if (not process_ptr->set_sleep_time_offset(
                    process_ptr->get_process()->process_get_sleep_time(
                        process_ptr->get_pid()), token))
                {
                    LOG(error, "executor", "returned_from_execute",
                        "Unable to get sleep time from process or calculate it: "
                        + text::to_string(
                            process_ptr->get_pid()));

                    // Kill process since we don't know how to handle this.
                    //
                    process_ptr->set_pending_killed(true, token);
                    reschedule = true;
                }
                else
                {
                    // Insert it into the timequeue
                    process_timer_queue.insert(std::make_pair(
                        process_ptr->get_wakeup_time(token),
                        process_ptr));
                }

                process_ptr->set_process_state(
                    ProcessInfo::PROCESS_STATE_SLEEPING,
                    token);
            }
            else if ((not suspended) and (not reschedule) and
                (status == Process::PROCESS_STATUS_WAIT_MESSAGE))
            {
                // If process wants to wait for any message, change state
                // and do nothing else since it wasn't rescheduled.
                //
                process_ptr->set_process_state(
                    ProcessInfo::PROCESS_STATE_WAIT_MESSAGE,
                    token);
            }
            else if (status == Process::PROCESS_STATUS_BLOCKED)
            {
                // If process is blocked, perform any reset needed of RIDs,
                // change state to BLOCKED if not rescheduled.
                //
                process_ptr->reset_blocked_resources(token);

                if ((not suspended) and (not reschedule))
                {
                    process_ptr->set_process_state(
                        ProcessInfo::PROCESS_STATE_BLOCKED,
                        token);
                }
            }
        }

        if (reschedule)
        {
            schedule_process(process_ptr);
        }

        unlock();
    }

    // ----------------------------------------------------------------------
    RID ProcessScheduler::get_next_rid(const PID pid)
    {
        RID result = 0;

        if (lock())
        {
            if (rid_to_pid.size() >= max_rid)
            {
                LOG(fatal, "executor", "get_next_rid",
                    "No more RIDs available!");
            }
            else if (all_processes.find(pid) == all_processes.end())
            {
                LOG(error, "executor", "get_next_rid",
                    "Invalid/Unknown PID specified: "
                    + text::to_string(pid));
            }
            else
            {
                while (not result)
                {
                    if (rid_to_pid.find(next_rid) == rid_to_pid.end())
                    {
                        // Found next RID.
                        result = next_rid;
                        rid_to_pid[next_rid] = pid;
                    }

                    ++next_rid;

                    if (next_rid >= max_rid)
                    {
                        next_rid = 1;
                    }
                }
            }

            unlock();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void ProcessScheduler::release_rid(const PID pid, const RID rid)
    {
        if (lock())
        {
            release_rid_internal(pid, rid);
            unlock();
        }
    }

    // ----------------------------------------------------------------------
    ProcessStats ProcessScheduler::get_process_stats(
        ProcessInfo *process_info_ptr) const
    {
        if (process_info_ptr)
        {
            return ProcessStats(
                process_info_ptr->get_pid(),
                process_info_ptr->get_process()->process_get_name(
                    process_info_ptr->get_pid()),
                process_info_ptr->get_db_owner_id(),
                process_info_ptr->get_db_executable_id(),
                process_info_ptr->get_process_state());
        }
        else
        {
            return ProcessStats();
        }
    }

    // ----------------------------------------------------------------------
    void ProcessScheduler::release_rid_internal(const PID pid, const RID rid)
    {
        RidToPidMap::iterator rid_iter = rid_to_pid.find(rid);

        if (rid_iter == rid_to_pid.end())
        {
            LOG(error, "executor", "release_rid_internal",
                "Invalid/Unknown RID specified: "
                + text::to_string(rid));
        }
        else if (rid_iter->second != pid)
        {
            LOG(error, "executor", "release_rid_internal",
                "PID " + text::to_string(pid)
                + " does not have a RID "
                + text::to_string(rid)
                + " or PID is invalid.");
        }
        else
        {
            rid_to_pid.erase(rid_iter);
        }
    }

    // -----------------------------------------------------------------------
    void ProcessScheduler::schedule_process(ProcessInfo *const process_ptr)
    {
        if (not process_ptr)
        {
            LOG(error, "executor", "schedule_process",
                "process_ptr is null!");
        }
        else
        {
            concurrency::WriterLockToken token(*process_ptr);

            // Determine overall situation of process_ptr.

            const bool pending_killed = process_ptr->get_pending_killed(token);
            bool in_queue = false;
            bool executing = false;
            bool sleeping = false;

            switch (process_ptr->get_process_state(token))
            {
                case ProcessInfo::PROCESS_STATE_KILLED:
                case ProcessInfo::PROCESS_STATE_READY:
                {
                    in_queue = true;
                    break;
                }

                case ProcessInfo::PROCESS_STATE_SLEEPING:
                {
                    sleeping = true;
                    break;
                }

                case ProcessInfo::PROCESS_STATE_EXECUTING:
                {
                    executing = true;
                    break;
                }

                default:
                {
                    // Initial bool values are correct
                    break;
                }
            }

            // If process_ptr is executing, this method is called when it
            // returns, so it will be taken care of then.
            if (pending_killed and (not executing))
            {
                // Update the process_ptr state to show it is being killed, if we
                // are not currently executing.
                process_ptr->set_process_state(
                    ProcessInfo::PROCESS_STATE_KILLED,
                    token);
                process_ptr->set_pending_killed(false, token);

                events::EventAccess::instance()->publish_event(
                    new events::ProcessExecutionEvent(
                        process_ptr->get_pid(),
                        process_ptr->get_db_executable_id(),
                        process_ptr->get_db_owner_id(),
                        process_ptr->get_process()->process_get_name(
                            process_ptr->get_pid()),
                        ProcessInfo::PROCESS_STATE_KILLED));
            }

            if ((not in_queue) and (not executing))
            {
                // Process needs to be added to queue.
                //

                if (not pending_killed)
                {
                    process_ptr->set_process_state(
                        ProcessInfo::PROCESS_STATE_READY,
                        token);
                }

                // Remove process from timequeue if needed
                if (sleeping)
                {
                    const std::chrono::steady_clock::time_point wakeup_time =
                        process_ptr->get_wakeup_time(token);
                    TimeQueue::iterator time_iter = process_timer_queue.find(
                        wakeup_time);

                    while (time_iter != process_timer_queue.end())
                    {
                        // Find the process in the timequeue and remove it

                        if (time_iter->second == process_ptr)
                        {
                            // Found it!
                            process_timer_queue.erase(time_iter);
                            time_iter = process_timer_queue.end();
                        }
                        else if (time_iter->first != wakeup_time)
                        {
                            // Gone past the wakeup time.  Not found (bad!)
                            //
                            LOG(error, "executor", "schedule_process",
                                "Could not find sleeping PID "
                                  + text::to_string(
                                    process_ptr->get_pid())
                                  + " in timequeue.");

                            time_iter = process_timer_queue.end();
                        }
                        else
                        {
                            // Not found; keep looking.
                            ++time_iter;

                            if (time_iter == process_timer_queue.end())
                            {
                                // Past the wakeup time, and didn't find it.
                                // Bad!
                                //
                                LOG(error, "executor", "schedule_process",
                                    "Could not find sleeping PID "
                                    + text::to_string(
                                        process_ptr->get_pid())
                                    + " in timequeue.");
                            }
                        }
                    }
                }

                if (process_run_queue.push(process_ptr))
                {
                    try
                    {
                        process_run_queue_semaphore.post();
                    }
                    catch (...)
                    {
                        // TODO Fix full semaphore situation
                        LOG(fatal, "executor", "schedule_process",
                            "Queue semaphore is full!");
                    }
                }
                else
                {
                    LOG(fatal, "executor", "schedule_process",
                        "Unable to queue process!");
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    bool ProcessScheduler::can_receive_messages(
        const ProcessInfo::ProcessState state) const
    {
        bool good = not shutting_down;

        switch (state)
        {
            case ProcessInfo::PROCESS_STATE_SUSPENDED:
            case ProcessInfo::PROCESS_STATE_KILLED:
            case ProcessInfo::PROCESS_STATE_COMPLETED:
            {
                good = false;
                break;
            }

            default:
            {
                // Default for 'good' already correct.
                break;
            }
        }

        return good;
    }

    // -----------------------------------------------------------------------
    void ProcessScheduler::cleanup_process(ProcessInfo *const process_info_ptr)
    {
        if (not process_info_ptr)
        {
            LOG(fatal, "executor", "cleanup_process",
                "process_info_ptr is null!");
            return;
        }

        if (not process_info_ptr->get_process())
        {
            LOG(fatal, "executor", "cleanup_process",
                "pointer to process itself is null!");
            return;
        }

        process_info_ptr->set_process_state(ProcessInfo::PROCESS_STATE_COMPLETED);

        Process * const process_ptr = process_info_ptr->get_process();
        const PID pid = process_info_ptr->get_pid();
        const bool want_delete = process_ptr->process_delete_when_finished(pid);
        const std::string process_name = process_ptr->process_get_name(pid);
        const dbtype::Id process_entity_id =
            process_info_ptr->get_db_owner_id();

        LOG(debug, "executor", "cleanup_process",
            "Cleaning up PID "
            + text::to_string(pid)
            + ", name "
            + process_name);

        events::EventAccess::instance()->publish_event(
            new events::ProcessExecutionEvent(
                pid,
                process_info_ptr->get_db_executable_id(),
                process_entity_id,
                process_name,
                ProcessInfo::PROCESS_STATE_COMPLETED));

        // To prevent a deadlock with resources triggering events on other
        // threads that try and use the scheduler, release our internal lock
        // while calling the process.
        unlock();

        // Let the process know it is being cleaned up.  This is the last
        // call allowed on the process.
        process_ptr->process_finished(pid);

        // Cleanup all resources the process may still have.
        // Only keep the lock token long enough to remove data from
        // ProcessInfo.  Everything needs to be unlocked while calling
        // the resource to avoid deadlocks, since the resource can do
        // anything it wants - including call the executor.
        //
        ArrayOfRIDs rids_to_clear;
        ArrayOfResources resources;

        // This is a scope for token.
        {
            concurrency::WriterLockToken token(*process_info_ptr);

            rids_to_clear = process_info_ptr->get_resource_ids(token);
            resources.reserve(rids_to_clear.size());

            for (ArrayOfRIDs::const_iterator rid_iter = rids_to_clear.begin();
                rid_iter != rids_to_clear.end();
                ++rid_iter)
            {
                resources.push_back(
                    process_info_ptr->remove_resource(*rid_iter, token));
            }
        }

        // We now have two parallel arrays.  Go through them and let the
        // resource know it has been removed, now that no locks are active.
        //
        for (size_t index = 0; index < resources.size(); ++index)
        {
            ProcessResource * const resource_ptr = resources[index];

            if (resource_ptr)
            {
                resource_ptr->resource_removed_from_process(
                    pid,
                    rids_to_clear[index],
                    true);
            }
        }

        if (not lock())
        {
            LOG(fatal, "executor", "cleanup_process", "Could not lock!");
        }

        // Now release the RID IDs for reuse
        //
        for (ArrayOfRIDs::const_iterator rid_iter = rids_to_clear.begin();
             rid_iter != rids_to_clear.end();
             ++rid_iter)
        {
            release_rid_internal(pid, *rid_iter);
        }

        // Delete process from our data structures.  It will never be present
        // in the run or timer queue since the process was just executed.
        //
        PidToProcessMap::iterator all_processes_iter = all_processes.find(pid);

        if (all_processes_iter == all_processes.end())
        {
            LOG(error, "executor", "cleanup_process",
                "Could not find PID "
                + text::to_string(pid)
                + " in all_processes!");
        }
        else
        {
            all_processes.erase(all_processes_iter);
        }

        SiteIdToProcessesMap::iterator site_iter = all_processes_entity.find(
            process_entity_id.get_site_id());

        if (site_iter == all_processes_entity.end())
        {
            LOG(error, "executor", "cleanup_process",
                "Could not find PID "
                + text::to_string(pid)
                + " in site portion of all_processes_entity!  ID: "
                + process_entity_id.to_string(true));
        }
        else
        {
            EntityIdToProcessMap::iterator entity_iter = site_iter->second.find(
                process_entity_id.get_entity_id());

            if (entity_iter == site_iter->second.end())
            {
                LOG(error, "executor", "cleanup_process",
                    "Could not find PID "
                    + text::to_string(pid)
                    + " in entity portion of all_processes_entity!  ID: "
                    + process_entity_id.to_string(true));
            }
            else
            {
                // Remove process from the vector, and see if anything can be
                // removed due to being empty.
                //
                bool removed = false;

                for (ProcessVector::iterator process_iter =
                        entity_iter->second.begin();
                    process_iter != entity_iter->second.end() and (not removed);
                    ++process_iter)
                {
                    if (*process_iter == process_info_ptr)
                    {
                        // Found it!
                        removed = true;
                        entity_iter->second.erase(process_iter);
                        break;
                    }
                }

                if (not removed)
                {
                    LOG(error, "executor", "cleanup_process",
                        "Could not find process for PID "
                        + text::to_string(pid)
                        + " in process pointer portion of all_processes_entity!"
                          "  ID: "
                        + process_entity_id.to_string(true));
                }

                if (entity_iter->second.empty())
                {
                    site_iter->second.erase(entity_iter);

                    if (site_iter->second.empty())
                    {
                        all_processes_entity.erase(site_iter);
                    }
                }
            }
        }

        // Delete ProcessInfo pointer since we're done with it.
        delete process_info_ptr;

        // Delete Process itself if indicated
        //
        if (want_delete)
        {
            delete process_ptr;
        }
    }

    // -----------------------------------------------------------------------
    bool ProcessScheduler::lock(void)
    {
        try
        {
            process_lock.lock();
            return true;
        }
        catch (...)
        {
            LOG(fatal, "executor", "lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void ProcessScheduler::unlock(void)
    {
        try
        {
            process_lock.unlock();
        }
        catch (...)
        {
            LOG(fatal, "executor", "unlock",
                "Trying to unlock threw an exception!");
        }
    }
}
}