#include <string>

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "executor/executor_ThreadedExecutor.h"

#include "executor/executor_ProcessScheduler.h"
#include "executor/executor_ProcessServices.h"
#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ProcessInfo.h"
#include "executor/executor_Process.h"

namespace mutgos
{
namespace executor
{
    // ----------------------------------------------------------------------
    ThreadedExecutor::ThreadedExecutor(ProcessScheduler *const scheduler_ptr)
      : process_scheduler_ptr(scheduler_ptr),
        current_process_info_ptr(0),
        services(&current_process_info_ptr, process_scheduler_ptr),
        stop_flag(false)
    {
        if (not scheduler_ptr)
        {
            LOG(fatal, "executor", "ThreadedExecutor",
                "Process scheduler is null!");
        }
    }

    // ----------------------------------------------------------------------
    ThreadedExecutor::~ThreadedExecutor()
    {
        stop();
    }

    // ----------------------------------------------------------------------
    void ThreadedExecutor::stop(void)
    {
        stop_flag.store(true);
    }

    // ----------------------------------------------------------------------
    void ThreadedExecutor::operator()()
    {
        thread_main();
    }

    // ----------------------------------------------------------------------
    void ThreadedExecutor::thread_main(void)
    {
        bool process_is_killed = false;
        bool scheduler_is_shutting_down = false;
        Process::ProcessStatus process_status =
            Process::PROCESS_STATUS_FINISHED;
        Process *process_ptr = 0;
        PID pid = 0;
        RID rid = 0;
        ProcessMessage *message_ptr = 0;
        bool process_messages = true;

        memory::ThreadVirtualHeapManager::add_thread();

        while (not stop_flag.load())
        {
            // See if we have something to execute
            //
            current_process_info_ptr = process_scheduler_ptr->get_next_execute(
                process_is_killed,
                scheduler_is_shutting_down);

            if (current_process_info_ptr)
            {
                process_ptr = current_process_info_ptr->get_process();
                pid = current_process_info_ptr->get_pid();

                LOG(debug, "executor", "thread_main",
                    "Starting execution of PID "
                    + text::to_string(pid)
                    + ", name " + process_ptr->process_get_name(pid));

                if (process_is_killed)
                {
                    LOG(debug, "executor", "thread_main",
                        "Killing process: "
                        + text::to_string(pid));

                    process_ptr->process_killed(
                        current_process_info_ptr->get_pid(),
                        services);
                    process_status = Process::PROCESS_STATUS_FINISHED;
                }
                else
                {
                    // If there are messages waiting, execute the process
                    // that way, otherwise just do a plain execute.
                    //
                    if (current_process_info_ptr->messages_empty())
                    {
                        process_status = process_ptr->process_execute(
                            pid,
                            services);
                    }
                    else
                    {
                        while (process_messages and
                            (not current_process_info_ptr->messages_empty()))
                        {
                            message_ptr =
                                current_process_info_ptr->get_next_message(rid);

                            if (not message_ptr)
                            {
                                // This shouldn't happen, but this way the
                                // process always gets to execute.
                                process_status =
                                    Process::PROCESS_STATUS_EXECUTE_MORE;
                            }
                            else
                            {
                                if (rid)
                                {
                                    process_status =
                                        process_ptr->process_execute(
                                            pid,
                                            services,
                                            rid,
                                            *message_ptr);
                                }
                                else
                                {
                                    process_status =
                                        process_ptr->process_execute(
                                            pid,
                                            services,
                                            *message_ptr);
                                }

                                delete message_ptr;
                                message_ptr = 0;
                            }

                            // Confirm the process hasn't errored out before
                            // giving it more messages.
                            //
                            switch (process_status)
                            {
                                case Process::PROCESS_STATUS_ERROR:
                                case Process::PROCESS_STATUS_SUSPENDED:
                                case Process::PROCESS_STATUS_FINISHED:
                                {
                                    // Doesn't want any more messages.
                                    // Stop early.
                                    process_messages = false;
                                    break;
                                }

                                default:
                                {
                                }
                            }
                        }
                    }
                }

                LOG(debug, "executor", "ThreadedExecutor",
                    "Finished execution of PID "
                    + text::to_string(pid)
                    + ", name " + process_ptr->process_get_name(pid));

                process_scheduler_ptr->returned_from_execute(
                    current_process_info_ptr,
                    process_status);

                current_process_info_ptr = 0;
                process_ptr = 0;
                pid = 0;
                rid = 0;
                process_messages = true;
            }

            if (scheduler_is_shutting_down)
            {
                stop();
            }
        }

        memory::ThreadVirtualHeapManager::delete_thread();
    }
}
}
