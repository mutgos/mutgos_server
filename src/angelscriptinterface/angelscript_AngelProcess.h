/*
 * angelscript_AngelProcess.h
 */

#ifndef MUTGOS_ANGELSCRIPT_ANGELPROCESS_H
#define MUTGOS_ANGELSCRIPT_ANGELPROCESS_H

#include <string>
#include <angelscript.h>

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/memory_MemHeapState.h"
#include "executor/executor_CommonTypes.h"
#include "executor/executor_Process.h"

#include "angelscript_ScriptContext.h"

namespace mutgos
{
namespace security
{
    // Forward declaraction.
    class Context;
}

namespace events
{
    // Forward declaraction.
    class TextChannel;
}

namespace angelscript
{
    // Forward declaraction.
    class AString;

    /**
     * Represents an AngelScript execution in the process executor.  It
     * handles setup, timeslicing, memory overflow, and I/O.
     */
    class AngelProcess : public executor::Process
    {
    public:
        /**
         * Constructs a new AngelScript process.
         * @param security_context[in] The security context.  Control of the
         * pointer passes to this class.
         * @param output_channel[in] The output channel, if any.  Channels own
         * their own pointers, but this will notify the channel when it has
         * been released on this end.
         * @param input_channel[in] The input channel, if any.  Channels own
         * their own pointers, but this will notify the channel when it has
         * been released on this end.
         * @param engine[in] The AngelScript engine that will be executing
         * the script.  The pointer must not be deleted until released by
         * this class.
         * @param context[in] The AngelScript context that will be executing
         * the script.  The pointer must not be deleted until released by
         * this class.
         * @param program_arguments[in] The arguments to be provided to the
         * script.
         */
        AngelProcess(
            security::Context * const security_context,
            events::TextChannel * const output_channel,
            events::TextChannel * const input_channel,
            asIScriptEngine * const engine,
            asIScriptContext * const context,
            const std::string &program_arguments);

        /**
         * Required virtual destructor.
         */
        virtual ~AngelProcess();

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
            const executor::PID pid,
            executor::ProcessServices &services);

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
            const executor::PID pid,
            executor::ProcessServices &services);

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
            const executor::PID pid,
            executor::ProcessServices &services,
            const executor::RID rid,
            executor::ProcessMessage &message);

        /**
         * This might be called many times, even while the process is
         * executing.
         * @param pid[in] The PID of the process whose name is desired.
         * @return The name of the process.  Must not be empty.
         */
        virtual std::string process_get_name(const executor::PID pid);

        /**
         * Implemented by subclass.
         * @param pid[in] The PID of the process being queried.
         * @return True if class instance is to be deleted (freed) after
         * the process has been killed or finished executing.
         */
        virtual bool process_delete_when_finished(const executor::PID pid);

        /**
         * Called only after process returns PROCESS_ERROR.
         * @param pid[in] The PID of the process for the error text.
         * @return If process_execute() indicates the process errored out, this
         * will return the reason(s) for the error.
         */
        virtual ErrorMessageText process_get_error_text(const executor::PID pid);

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
            const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Called when the the executor has finished with the process.  This is
         * called at some point after process_execute() has returned PROCESS_FINISHED
         * or PROCESS_ERROR.  It is also called after process_killed().
         * Typically this is where any resources are cleaned up.
         * If process_delete_when_finished() returns true, this is the last
         * call before destruction of the class instance.
         * @param pid[in] The PID of the finished process.
         */
        virtual void process_finished(const executor::PID pid);

    private:

        /**
         * Called by AngelScript's debugger functionality, this is used to
         * monitor how many lines have been executed in order to do timeslices,
         * kill long running processes, and kill processes that have used too
         * much memory.
         * @param ctx[in] Script context.
         * @param dbg[in] User-provided pointer (not used).
         */
        void debug_line_callback(asIScriptContext *ctx, void *dbg);

        /**
         * Compiles the script into bytecode (if needed), and adds the bytecode
         * to the engine.
         * @return True if success.
         */
        bool add_script(void);

        /**
         * Common code that compiles a script (as needed) and runs it for
         * a timeslice, or runs the next timeslice if already started.
         * @return The process status code to return back to the scheduler.
         */
        ProcessStatus run_script(void);

        ScriptContext my_context; ///< The AngelScript and security context
        memory::MemHeapState heap_state; ///< 'Virtual' heap state storage between timeslices (allocation, etc).

        std::string arguments; ///< Argument string.  Will be cleared once program starts.

        bool compiled; ///< True if we have compiled and set up the bytecode
        bool overallocated; ///< True if process has allocated memory beyond what is allowed
        MG_VeryLongUnsignedInt total_instructions_executed; ///< How many lines (instructions) executed in total for this process
        MG_VeryLongUnsignedInt slice_instructions_executed; ///< How many lines (instructions) executed this timeslice for this process

        std::string process_name; ///< Name of the process, for logging and ps.
        ErrorMessageText error_messages; ///< Any error messages to be returned if process in an error state.

        AString * argument_ptr; ///< Holds the argument for 'main' so that it can be GCed on exit

        events::TextChannel * const output_channel_ptr; ///< Pointer to outgoing channel
        events::TextChannel * const input_channel_ptr; ///< Pointer to incoming channel

        asIScriptEngine * const engine_ptr; ///< Pointer to script engine
        asIScriptContext * const context_ptr; ///< Pointer to script context
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_ANGELPROCESS_H
