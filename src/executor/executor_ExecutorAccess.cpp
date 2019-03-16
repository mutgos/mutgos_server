
#include <boost/thread/thread.hpp>

#include "utilities/mutgos_config.h"

#include "dbtypes/dbtype_Id.h"

#include "executor/executor_Process.h"
#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ThreadedExecutor.h"
#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_ProcessScheduler.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace executor
{
    // Statics
    //
    ExecutorAccess *ExecutorAccess::singleton_ptr = 0;


    // ----------------------------------------------------------------------
    ExecutorAccess* ExecutorAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new ExecutorAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void ExecutorAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::startup(void)
    {
        // Start the threads.
        //
        if (process_executors.empty())
        {
            for (MG_UnsignedInt count = 0;
                count < config::executor::thread_count();
                ++count)
            {
                ThreadedExecutor *executor_ptr = new ThreadedExecutor(
                    &process_scheduler);
                boost::thread *thread_ptr =
                    new boost::thread(boost::ref(*executor_ptr));

                process_executors.push_back(thread_ptr);
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void ExecutorAccess::shutdown(void)
    {
        process_scheduler.shutdown();

        // Wait for all the threads to exit
        //
        while (not process_executors.empty())
        {
            boost::thread *thread_ptr = process_executors.back();
            thread_ptr->join();
            delete thread_ptr;
            process_executors.pop_back();
        }
    }

    // ----------------------------------------------------------------------
    PID ExecutorAccess::add_process(
        const dbtype::Id &executable_id,
        const dbtype::Id &owner_id,
        Process *process_ptr)
    {
        PID pid = 0;

        if (not process_ptr)
        {
            LOG(error, "executor", "add_process", "process is null!");
        }
        else
        {
            pid = process_scheduler.add_process(
                executable_id,
                owner_id,
                process_ptr);
        }

        return pid;
    }

    // ----------------------------------------------------------------------
    PID ExecutorAccess::add_process(Process *process_ptr)
    {
        return add_process(dbtype::Id(), dbtype::Id(), process_ptr);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::kill_process(const PID pid)
    {
        return process_scheduler.kill_process(pid);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::suspend_process(const PID pid)
    {
        return process_scheduler.suspend_process(pid);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::start_process(const PID pid)
    {
        return process_scheduler.start_process(pid);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::send_message(
        const PID pid,
        ProcessMessage * const message_ptr)
    {
        return process_scheduler.send_message(pid, message_ptr);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::send_message(
        const PID pid,
        const RID rid,
        ProcessMessage * const message_ptr)
    {
        return process_scheduler.send_message(pid, rid, message_ptr);
    }

    // ----------------------------------------------------------------------
    bool ExecutorAccess::cleanup_processes(const dbtype::Id &id)
    {
        const ArrayOfPIDs pids = process_scheduler.get_pids_for_id(id);

        for (ArrayOfPIDs::const_iterator pid_iter = pids.begin();
            pid_iter != pids.end();
            ++pid_iter)
        {
            process_scheduler.kill_process(*pid_iter);
        }

        return not pids.empty();
    }

    // ----------------------------------------------------------------------
    ExecutorAccess::ProcessStatsVector ExecutorAccess::get_process_stats_for_site(
        const dbtype::Id::SiteIdType site_id)
    {
        return process_scheduler.get_process_stats_for_site(site_id);
    }

    // ----------------------------------------------------------------------
    ExecutorAccess::ExecutorAccess(void)
    {
    }

    // ----------------------------------------------------------------------
    ExecutorAccess::~ExecutorAccess()
    {
        shutdown();
    }
}
}
