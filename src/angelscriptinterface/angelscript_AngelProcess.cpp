/*
 * angelscript_AngelProcess.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/memory_MemHeapState.h"
#include "utilities/mutgos_config.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalFormattedText.h"

#include "executor/executor_CommonTypes.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessServices.h"
#include "security/security_Context.h"

#include "channels/events_TextChannel.h"
#include "channels/events_ChannelFlowMessage.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "angelscript_AngelScriptAccess.h"
#include "angelscript_ScriptUtilities.h"
#include "angelscript_AngelProcess.h"
#include "angelscript_CompiledBytecodeStream.h"
#include "angelscript_AString.h"

namespace
{
    const std::string SCRIPT_MODULE_NAME = "mutgos_script";
}

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    // TODO Will need better logging
    AngelProcess::AngelProcess(
        security::Context * const security_context,
        events::TextChannel * const output_channel,
        events::TextChannel * const input_channel,
        asIScriptEngine * const engine,
        asIScriptContext * const context,
        const std::string &program_arguments)
        : my_context(security_context),
          arguments(program_arguments),
          compiled(false),
          overallocated(false),
          total_instructions_executed(0),
          slice_instructions_executed(0),
          argument_ptr(0),
          output_channel_ptr(output_channel),
          input_channel_ptr(input_channel),
          engine_ptr(engine),
          context_ptr(context)
    {
        if ((not security_context) or (not engine) or (not context))
        {
            LOG(fatal, "angelscript", "AngelProcess",
                "security_context, engine, or context are null!  "
                "Crash will follow.");

            error_messages.push_back(
                "AngelProcess had unexpected null pointers.");
        }

        heap_state.set_max_mem(config::angelscript::max_heap() * 1024);
        my_context.set_output_channel(output_channel);

        ScriptUtilities::set_my_script_context(engine, &my_context);

        // Should always be valid
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                security_context->get_program());

        // Cache name, which is just the name of the Entity representing the
        // program to execute.
        //
        if (program_ref.valid())
        {
            process_name = program_ref->get_entity_name();
        }
        else
        {
            process_name = "<INVALID>";
        }

        if (context->SetLineCallback(
            asMETHOD(AngelProcess, debug_line_callback),
            this,
            asCALL_THISCALL) < 0)
        {
            LOG(error, "angelscript", "AngelProcess",
                "Could not register line callback!");
        }
    }

    // ----------------------------------------------------------------------
    AngelProcess::~AngelProcess()
    {
        // Cleanup the stuff we did in this class
        //
        context_ptr->ClearLineCallback();
        context_ptr->Abort();

        if (argument_ptr)
        {
            // We had to hold a reference to avoid AS from GCing it.  We're
            // done executing the script, so release our reference so it'll
            // get cleaned up.
            argument_ptr->release_ref();
        }

        context_ptr->Unprepare();
        ScriptUtilities::cleanup_my_script_context(engine_ptr);
        engine_ptr->DiscardModule(SCRIPT_MODULE_NAME.c_str());

        // Return the engine and context back to the holder, generally in the
        // same condition we found it.
        AngelScriptAccess::instance()->release_engine_context(
            engine_ptr,
            context_ptr);
    }

    // ----------------------------------------------------------------------
    void AngelProcess::process_added(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        if (output_channel_ptr)
        {
            if (not services.add_resource(output_channel_ptr))
            {
                LOG(error, "angelscript", "process_added",
                    "Unable to add output channel resource!");
            }
        }

        if (input_channel_ptr)
        {
            if (not services.add_blocking_resource(input_channel_ptr))
            {
                LOG(error, "angelscript", "process_added",
                    "Unable to add input channel resource!");
            }
        }
    }

    // ----------------------------------------------------------------------
    AngelProcess::ProcessStatus AngelProcess::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        return run_script();
    }

    // ----------------------------------------------------------------------
    AngelProcess::ProcessStatus AngelProcess::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        const executor::RID rid,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_ERROR;

        // If we get a message showing a resource has been closed, then we
        // abort processing.  Otherwise, ignore the message and continue
        // running the script.
        //
        if (message.message_get_type() ==
            executor::ProcessMessage::MESSAGE_CHANNEL_FLOW)
        {
            events::ChannelFlowMessage * const flow_message =
                dynamic_cast<events::ChannelFlowMessage *>(&message);

            if (not flow_message)
            {
                LOG(error, "angelscript", "process_execute(RID)",
                    "Unable to cast flow message!");
                error_messages.push_back(
                    "Internal error (unable to cast flow message)");
            }
            else
            {
                if (flow_message->get_channel_status() !=
                    events::ChannelFlowMessage::CHANNEL_FLOW_OPEN)
                {
                    // Anything other than an open channel is an error for
                    // now.
                    LOG(error, "angelscript", "process_execute(RID)",
                        "Channel flow no longer open for "
                        + flow_message->get_channel_name());
                    error_messages.push_back(
                        "Channel not open: " + flow_message->get_channel_name());
                }
                else
                {
                    status = run_script();
                }
            }
        }
        else
        {
            status = run_script();
        }

        return status;
    }

    // ----------------------------------------------------------------------
    std::string AngelProcess::process_get_name(const executor::PID pid)
    {
        return process_name;
    }

    // ----------------------------------------------------------------------
    bool AngelProcess::process_delete_when_finished(const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    AngelProcess::ErrorMessageText AngelProcess::process_get_error_text(
        const executor::PID pid)
    {
        return error_messages;
    }

    // ----------------------------------------------------------------------
    void AngelProcess::process_killed(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // Nothing to do here yet.
    }

    // ----------------------------------------------------------------------
    void AngelProcess::process_finished(const executor::PID pid)
    {
        // Nothing to do here yet.
    }

    // ----------------------------------------------------------------------
    void AngelProcess::debug_line_callback(asIScriptContext *ctx, void *dbg)
    {
        ++slice_instructions_executed;

        if (slice_instructions_executed % 10)
        {
            overallocated =
                memory::ThreadVirtualHeapManager::check_overallocation(false);
        }

        if ((slice_instructions_executed > config::angelscript::timeslice()) or
            overallocated)
        {
            // Time to pause temporarily and let someone else execute,
            // or abort if overallocated.
            ctx->Suspend();
            slice_instructions_executed = 0;
        }
    }

    // ----------------------------------------------------------------------
    bool AngelProcess::add_script(void)
    {
        bool success = false;

        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                my_context.get_security_context().get_program());

        if (not program_ref.valid())
        {
            LOG(warning, "angelscript", "add_script",
                "Script entity is no longer valid: " +
                my_context.get_security_context().get_program().to_string(true));

            error_messages.push_back("Script no longer exists.");
        }
        else
        {
            // Compile as needed
            //
            char *bytecode = 0;
            size_t bytecode_size = 0;

            if (not AngelScriptAccess::instance()->compile_script(
                program_ref,
                *engine_ptr,
                output_channel_ptr,
                true,
                bytecode,
                bytecode_size))
            {
                LOG(info, "angelscript", "add_script",
                    "Failed to compile script " +
                    my_context.get_security_context().get_program().to_string(true));

                error_messages.push_back("Script failed to compile.");
            }
            else
            {
                CompiledBytecodeStream bytecode_stream(bytecode, bytecode_size);
                bytecode = 0;

                // Set up engine with script specifics
                //
                asIScriptModule * const mod_ptr = engine_ptr->GetModule(
                    SCRIPT_MODULE_NAME.c_str(),
                    asGM_ALWAYS_CREATE);

                if (not mod_ptr)
                {
                    LOG(error, "angelscript", "add_script",
                        "Could not get module.");

                    error_messages.push_back(
                        "Internal error (could not get module).");
                }
                else
                {
                    const int rc = mod_ptr->LoadByteCode(&bytecode_stream);

                    if (rc < 0)
                    {
                        // Failed to load byte code for some reason
                        LOG(error, "angelscript", "add_script",
                            "Could not load bytecode for script " +
                            my_context.get_security_context().get_program().
                                to_string(true) + ", error code " +
                            text::to_string(rc));

                        error_messages.push_back("Bytecode corrupt.");
                    }
                    else
                    {
                        success = true;
                    }
                }
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    AngelProcess::ProcessStatus AngelProcess::run_script(void)
    {
        ProcessStatus status = PROCESS_STATUS_ERROR;
        bool first_run = false;

        // Compile/load bytecode if first timeslice
        //
        if (not compiled)
        {
            // Need to compile it first
            if (add_script())
            {
                compiled = true;
                first_run = true;
            }
        }

        // If compilation succeeded, run the timeslice.
        //
        if (compiled)
        {
            memory::ThreadVirtualHeapManager::set_thread_heap_state(heap_state);

            if (first_run)
            {
                // Need to get the method to execute, prepare the context,
                // and set the argument(s).
                //
                asIScriptModule * const mod_ptr = engine_ptr->GetModule(
                    SCRIPT_MODULE_NAME.c_str(),
                    asGM_ONLY_IF_EXISTS);

                if (not mod_ptr)
                {
                    LOG(error, "angelscript", "run_script", "module is null!");
                    error_messages.push_back("Internal error (can't get module).");
                    return status;
                }

                asIScriptFunction * const func_ptr = mod_ptr->GetFunctionByDecl(
                    "void main(const string &in)");

                if (not func_ptr)
                {
                    LOG(info, "angelscript", "run_script",
                        "Can't find main() func for " +
                            my_context.get_security_context().get_program().
                                to_string(true));
                    error_messages.push_back(
                        "Script does not have a main function with signature "
                        "void main(const string &in)");
                    return status;
                }

                // Got the function, now convert the argument and prepare
                // the context.
                //
                argument_ptr = new AString(engine_ptr); // Owned by engine

                try
                {
                    argument_ptr->import_from_string(arguments);
                }
                catch (...)
                {
                    error_messages.push_back("Argument string too big.");
                    return status;
                }

                if (context_ptr->Prepare(func_ptr) < 0)
                {
                    LOG(error, "angelscript", "run_script",
                        "Unable to prepare context!");
                    error_messages.push_back(
                        "Internal error (can't prepare context).");
                    return status;
                }

                if (context_ptr->SetArgObject(0, argument_ptr) < 0)
                {
                    LOG(error, "angelscript", "run_script",
                        "Unable to set argument on main function.");
                    error_messages.push_back(
                        "Internal error (can't set main function arg).");
                    return status;
                }

                arguments.clear();
                arguments.shrink_to_fit();
            }

            // Execute the time slice
            const int execute_rc = context_ptr->Execute();
            engine_ptr->GarbageCollect();

            switch (execute_rc)
            {
                case asEXECUTION_ABORTED:
                {
                    // External abort; this shouldn't happen yet.
                    error_messages.push_back(
                        "Execution was aborted.");
                    status = PROCESS_STATUS_ERROR;
                    break;
                }

                case asEXECUTION_SUSPENDED:
                {
                    // We are suspended because our timeslice is up and more
                    // timeslices are needed, or because we ran out of memory.
                    //
                    if (overallocated)
                    {
                        // Out of memory.  Abort execution.
                        std::string message = "Script " +
                            my_context.get_security_context().get_program().
                              to_string(true) +
                            " had an exception of type: out of memory";

                        LOG(error, "angelscript", "run_script", message);
                        error_messages.push_back(message);
                        status = PROCESS_STATUS_ERROR;
                    }
                    else
                    {
                        // Needs more timeslices
                        status = PROCESS_STATUS_EXECUTE_MORE;
                    }

                    break;
                }

                case asEXECUTION_FINISHED:
                {
                    // Completely done with the script.
                    status = PROCESS_STATUS_FINISHED;
                    break;
                }

                case asEXECUTION_EXCEPTION:
                {
                    // Exception thrown while running script.  Log the info
                    // and stop.
                    //
                    std::string message = "Script " +
                        my_context.get_security_context().get_program().
                            to_string(true) + " had an exception of type: ";

                    switch (my_context.get_exception_type())
                    {
                        case ScriptContext::EXCEPTION_MEMORY:
                        {
                            message += "out of memory";
                            break;
                        }

                        case ScriptContext::EXCEPTION_SECURITY:
                        {
                            message += "security violation";
                            break;
                        }

                        case ScriptContext::EXCEPTION_ANGEL:
                        case ScriptContext::EXCEPTION_STD:
                        {
                            message += "script error";
                            break;
                        }

                        default:
                        {
                            message += "unknown";
                            break;
                        }
                    }

                    message += ", message: ";
                    message += my_context.get_exception_reason();
                    message += ", from AngelScript: ";
                    message += context_ptr->GetExceptionString();

                    LOG(error, "angelscript", "run_script", message);
                    error_messages.push_back(message);
                    status = PROCESS_STATUS_ERROR;
                    break;
                }

                default:
                {
                    // Unknown return code, just abort.
                    LOG(error, "angelscript", "run_script",
                        "Unknown error from Execute(): "
                          + text::to_string(execute_rc));
                    error_messages.push_back(
                        "Unknown error while executing script.");
                    status = PROCESS_STATUS_ERROR;
                    break;
                }
            }

            if ((status == PROCESS_STATUS_ERROR) and output_channel_ptr)
            {
                // Also print to user
                //
                text::ExternalTextLine line;

                for (ErrorMessageText::const_iterator error_iter =
                        error_messages.begin();
                    error_iter != error_messages.end();
                    ++error_iter)
                {
                    line.push_back(new text::ExternalFormattedText(
                        *error_iter,
                        false,
                        false,
                        false,
                        false,
                        text::ExternalFormattedText::COLOR_RED));

                    output_channel_ptr->send_item(line);
                    text::ExternalText::clear_text_line(line);
                }
            }

            heap_state =
                memory::ThreadVirtualHeapManager::get_thread_heap_state();
        }

        return status;
    }
}
}
