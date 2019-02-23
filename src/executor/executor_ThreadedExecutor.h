#ifndef MUTGOS_EXECUTOR_THREADEDEXECUTOR_H
#define MUTGOS_EXECUTOR_THREADEDEXECUTOR_H

#include <boost/atomic.hpp>

#include "executor/executor_ProcessServices.h"

namespace mutgos
{
namespace executor
{
    // Forward declarations
    class ProcessScheduler;
    class ProcessInfo;

    /**
     * This runs as a thread (more than one instance and therefore thread
     * is allowed) and is what actually executes processes.
     */
    class ThreadedExecutor
    {
    public:
        /**
         * Constructor.
         * @param scheduler_ptr[in] The process scheduler instance.
         */
        ThreadedExecutor(ProcessScheduler * const scheduler_ptr);

        /**
         * Destructor.
         */
        ~ThreadedExecutor();

        /**
         * Signals the running thread to shut down gracefully, as soon as it
         * can.  This does not block.  Join the thread to know when it's
         * completed.
         */
        void stop(void);

        /**
         * Used by Boost threads to start our threaded code.
         */
        void operator()();

    private:
        /**
         * Main loop for the thread.  Pulls off processes and executes them.
         */
        void thread_main();

    private:
        ProcessScheduler * const process_scheduler_ptr; ///< Process scheduler
        ProcessInfo *current_process_info_ptr; ///< Currently executing process
        ProcessServices services; ///< Services to pass to executing process
        boost::atomic<bool> stop_flag; ///< If true, exit thread_main()
    };
}
}
#endif //MUTGOS_EXECUTOR_THREADEDEXECUTOR_H
