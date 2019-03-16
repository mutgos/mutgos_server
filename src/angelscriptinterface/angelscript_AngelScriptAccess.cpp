/*
 * angelscript_AngelScriptAccess.cpp
 */

#include <string>
#include <angelscript.h>

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_StringConversion.h"
#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "channels/events_TextChannel.h"

#include "concurrency/concurrency_WriterLockToken.h"

#include "dbinterface/dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_DocumentProperty.h"

#include "softcode/softcode_SoftcodeAccess.h"

#include "angelscriptinterface/add_on/scriptarray.h"
#include "angelscript_AString.h"
#include "angelscript_StringFactory.h"
#include "angelscript_AEntity.h"
#include "angelscript_OnlineStatEntry.h"
#include "angelscript_DatabaseOps.h"
#include "angelscript_InputOutputOps.h"
#include "angelscript_MovementOps.h"
#include "angelscript_SystemOps.h"
#include "angelscript_CompiledBytecodeStream.h"
#include "angelscript_AngelScriptAccess.h"
#include "angelscript_AngelProcess.h"

#include "executor/executor_ExecutorAccess.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"
#include "utilities/memory_MemHeapState.h"

namespace
{
    const std::string SCRIPT_TYPE_NAME = "AngelScript";
    const std::string SCRIPT_MODULE_NAME = "mutgos_script";
}

namespace mutgos
{
namespace angelscript
{
    // Statics
    //
    AngelScriptAccess *AngelScriptAccess::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    AngelScriptAccess* AngelScriptAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new AngelScriptAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void AngelScriptAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool AngelScriptAccess::startup(void)
    {
        bool success = true;

        boost::lock_guard<boost::mutex> guard(mutex);

        const memory::MemHeapState create_engine_heap;
        memory::ThreadVirtualHeapManager::set_thread_heap_state(
            create_engine_heap);

        asSetGlobalMemoryFunctions(
            memory::ThreadVirtualHeapManager::mem_alloc_nofail,
            memory::ThreadVirtualHeapManager::mem_free);
        asPrepareMultithread();

        softcode::SoftcodeAccess::instance()->register_language(this);

        return success;
    }

    // ----------------------------------------------------------------------
    bool AngelScriptAccess::shutdown(void)
    {
        bool success = true;

        boost::lock_guard<boost::mutex> guard(mutex);

        softcode::SoftcodeAccess::instance()->unregister_language(this);

        // Clean up unused engines. Log error if some are still in use.
        //
        const memory::MemHeapState delete_engine_heap;
        memory::ThreadVirtualHeapManager::set_thread_heap_state(
            delete_engine_heap);

        for (size_t index = 0; index < engines_avail.size(); ++index)
        {
            EngineContextState &engine_state =
                engines_avail[index];

            engine_state.engine_ptr->ReturnContext(engine_state.context_ptr);
            engine_state.engine_ptr->ShutDownAndRelease();
            delete engine_state.string_factory_ptr;
        }

        engines_avail.clear();

        if (not engines_used.empty())
        {
            LOG(fatal, "angelscript", "shutdown",
                "There are " + text::to_string(engines_used.size())
                + " engines still in use!");

            success = false;
        }

        // Deinitialize AngelScript
        //
        asThreadCleanup();
        // Causes crash!!
        // asUnprepareMultithread();

        return success;
    }

    // ----------------------------------------------------------------------
    std::string AngelScriptAccess::get_language_name(void)
    {
        return SCRIPT_TYPE_NAME;
    }

    // ----------------------------------------------------------------------
    executor::PID AngelScriptAccess::make_process(
        security::Context *security_context_ptr,
        const std::string &command_name,
        const std::string &arguments,
        events::TextChannel *outgoing_channel_ptr,
        events::TextChannel *incoming_channel_ptr)
    {
        executor::PID pid = 0;
        asIScriptEngine *engine_ptr = 0;
        asIScriptContext *context_ptr = 0;

        if (not security_context_ptr)
        {
            LOG(error, "angelscript", "make_process",
                "Security pointer is null; cannot run.");

            return pid;
        }

        // Get engine, make the process, and add it to the Executor.
        //
        if (not get_available_engine(engine_ptr, context_ptr))
        {
            LOG(error, "angelscript", "make_process",
                "Unable to get an Engine; cannot run.");

            delete security_context_ptr;
        }
        else
        {
            AngelProcess * const process_ptr = new AngelProcess(
                security_context_ptr,
                outgoing_channel_ptr,
                incoming_channel_ptr,
                engine_ptr,
                context_ptr,
                arguments);

            pid = executor::ExecutorAccess::instance()->add_process(
                security_context_ptr->get_program(),
                security_context_ptr->get_requester(),
                process_ptr);
            security_context_ptr->set_pid(pid);
        }

        return pid;
    }

    // ----------------------------------------------------------------------
    bool AngelScriptAccess::compile(
        const dbtype::Id &program_id,
        events::TextChannel *outgoing_channel_ptr)
    {
        // Get the program, grab a free Engine, and do the compile.
        //
        bool success = false;
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);
        asIScriptEngine *engine_ptr = 0;
        asIScriptContext *context_ptr = 0;
        char *bytecode_ptr = 0;
        size_t bytecode_size = 0;

        if (not get_available_engine(engine_ptr, context_ptr))
        {
            LOG(error, "angelscript", "compile",
                "Unable to get an Engine; cannot compile.");
        }
        else
        {
            success = compile_script(
                program_ref,
                *engine_ptr,
                outgoing_channel_ptr,
                false,
                bytecode_ptr,
                bytecode_size);

            // Done compiling, return the engine.
            return_used_engine(engine_ptr, context_ptr);
            engine_ptr = 0;
            context_ptr = 0;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool AngelScriptAccess::uncompile(const dbtype::Id &program_id)
    {
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);

        return uncompile_script(program_ref);
    }

    // ----------------------------------------------------------------------
    void AngelScriptAccess::release_engine_context(
        asIScriptEngine * const engine,
        asIScriptContext * const context)
    {
        return_used_engine(engine, context);
    }

    // ----------------------------------------------------------------------
    // We can assume that if we are called, the program must be for
    // our language.
    //
    bool AngelScriptAccess::uncompile_script(
        dbinterface::EntityRef &program_ref)
    {
        bool success = false;
        dbtype::Program * const program_ptr = program_ref.valid() ?
            dynamic_cast<dbtype::Program *>(program_ref.get())
            : 0;

        if (program_ptr)
        {
            success = program_ptr->set_compiled_code(0, 0);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    // We can assume that if we are called, the program must be for
    // our language.
    //
    bool AngelScriptAccess::compile_script(
        dbinterface::EntityRef &program_ref,
        asIScriptEngine &engine,
        events::TextChannel *output_channel_ptr,
        const bool want_bytecode,
        char *&bytecode_ptr,
        size_t &bytecode_size)
    {
        bool success = false;

        bytecode_ptr = 0;
        bytecode_size = 0;

        dbtype::Program * const program_ptr = program_ref.valid() ?
            dynamic_cast<dbtype::Program *>(program_ref.get())
            : 0;

        if (not program_ptr)
        {
            LOG(error, "angelscript", "compile_script",
                "Not a program: " + (program_ref.valid() ?
                    program_ref.id().to_string(true) : "<INVALID>"));
        }
        else
        {
            concurrency::WriterLockToken token(*program_ptr);

            if (not program_ptr->has_compiled_code(token))
            {
                // Currently not compiled, so compilation required.
                // Set up virtual heap to be unlimited, since we are just
                // compiling.
                //
                LOG(info, "angelscript", "compile_script", "Compiling script "
                  + program_ref.id().to_string(true));

                engine.GarbageCollect();
                const memory::MemHeapState current_heap =
                    memory::ThreadVirtualHeapManager::get_thread_heap_state();
                const memory::MemHeapState compile_heap;
                memory::ThreadVirtualHeapManager::set_thread_heap_state(
                    compile_heap);

                int rc = 0;
                MessageCallbackWrapper messageCallback(
                    program_ref.id(),
                    output_channel_ptr);

                if (engine.SetMessageCallback(
                    asMETHOD(MessageCallbackWrapper, message),
                    &messageCallback,
                    asCALL_THISCALL) < 0)
                {
                    LOG(error, "angelscript", "compile_script",
                        "Could not register message callback.  "
                        "No compiler messages will be seen.");
                }

                asIScriptModule * const mod_ptr = engine.GetModule(
                    SCRIPT_MODULE_NAME.c_str(),
                    asGM_ALWAYS_CREATE);

                if (not mod_ptr)
                {
                    LOG(error, "angelscript", "compile_script",
                        "Could not get module.");
                }
                else
                {
                    success = true;

                    // Add the source code to the module.
                    //
                    const std::string source_code =
                        program_ptr->get_source_code(token).get_as_string();
                    rc = mod_ptr->AddScriptSection(
                        "script",
                        source_code.c_str(),
                        source_code.size());
                    check_compile_rc(rc, __LINE__, success);

                    if (success)
                    {
                        // Compile the source code
                        //
                        rc = mod_ptr->Build();
                        check_compile_rc(rc, __LINE__, success);
                    }

                    if (success)
                    {
                        // Save the binary data off in the Program
                        //
                        CompiledBytecodeStream bytecode;
                        rc = mod_ptr->SaveByteCode(&bytecode, false);
                        check_compile_rc(rc, __LINE__, success);

                        if (success)
                        {
                            const char *raw_bytecode_ptr = 0;
                            size_t raw_bytecode_size = 0;

                            success = bytecode.get_written_bytecode(
                                raw_bytecode_ptr,
                                raw_bytecode_size);

                            if (success)
                            {
                                success = program_ptr->set_compiled_code(
                                    raw_bytecode_ptr,
                                    raw_bytecode_size,
                                    token);
                            }
                        }
                    }
                }

                engine.DiscardModule(SCRIPT_MODULE_NAME.c_str());
                engine.ClearMessageCallback();

                // Go back to the original heap
                //
                engine.GarbageCollect();
                memory::ThreadVirtualHeapManager::set_thread_heap_state(
                    current_heap);
            }

            if (want_bytecode and program_ptr->has_compiled_code(token))
            {
                success = program_ptr->get_compiled_code(
                    bytecode_ptr,
                    bytecode_size,
                    token);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    AngelScriptAccess::MessageCallbackWrapper::MessageCallbackWrapper(
        const dbtype::Id &id,
        events::TextChannel * const channel_ptr)
          : prog_id(id),
            output_channel_ptr(channel_ptr)
    {
        if (not channel_ptr)
        {
            LOG(error, "angelscript", "MessageCallbackWrapper",
                "Null channel pointer passed in!");
        }
    }

    // ----------------------------------------------------------------------
    AngelScriptAccess::MessageCallbackWrapper::~MessageCallbackWrapper()
    {
    }

    // ----------------------------------------------------------------------
    void AngelScriptAccess::MessageCallbackWrapper::message(
        const asSMessageInfo *msg,
        void *param)
    {
        if (msg)
        {
            const char *severity_string = 0;

            switch (msg->type)
            {
                case asMSGTYPE_ERROR:
                {
                    severity_string = "ERR     ";
                    break;
                }

                case asMSGTYPE_WARNING:
                {
                    severity_string = "WARN    ";
                    break;
                }

                case asMSGTYPE_INFORMATION:
                {
                    severity_string = "INFO    ";
                    break;
                }

                default:
                {
                    severity_string = "UNKNOWN ";
                    break;
                }
            }

            std::string formatted_message = severity_string;

            formatted_message += " ** AngelScript ** ";
            formatted_message += prog_id.to_string(true);
            formatted_message += "  ";
            formatted_message += msg->section;
            formatted_message += " (row ";
            formatted_message += text::to_string(msg->row);
            formatted_message += ", col ";
            formatted_message += text::to_string(msg->col);
            formatted_message += "): ";
            formatted_message += msg->message;

            switch (msg->type)
            {
                case asMSGTYPE_ERROR:
                {
                    LOG(error, "angelscript", "message (compiler)",
                        formatted_message);
                    break;
                }

                case asMSGTYPE_WARNING:
                {
                    LOG(warning, "angelscript", "message (compiler)",
                        formatted_message);
                    break;
                }

                case asMSGTYPE_INFORMATION:
                {
                    LOG(info, "angelscript", "message (compiler)",
                        formatted_message);
                    break;
                }

                default:
                {
                    LOG(error, "angelscript", "message (compiler)",
                        formatted_message);
                    break;
                }
            }

            if (output_channel_ptr)
            {
                text::ExternalTextLine line;
                line.push_back(new text::ExternalPlainText(formatted_message));

                if (not output_channel_ptr->send_item(line))
                {
                    LOG(warning, "angelscript", "message (compiler)",
                        "Output channel is blocked/closed");
                }

                text::ExternalText::clear_text_line(line);
            }
        }
    }

    // ----------------------------------------------------------------------
    AngelScriptAccess::AngelScriptAccess(void)
    {
    }

    // ----------------------------------------------------------------------
    AngelScriptAccess::~AngelScriptAccess()
    {
        shutdown();
    }

    // ----------------------------------------------------------------------
    bool AngelScriptAccess::get_available_engine(
        asIScriptEngine *&engine_ptr,
        asIScriptContext *&context_ptr)
    {
        bool status = false;

        boost::lock_guard<boost::mutex> guard(mutex);

        if (not engines_avail.empty())
        {
            // An engine is available.  Mark it in use and return.
            //
            engine_ptr = engines_avail.back().engine_ptr;
            context_ptr = engines_avail.back().context_ptr;

            engines_used.push_back(engines_avail.back());
            engines_avail.pop_back();
            status = true;
        }
        else
        {
            // No engines currently available.  Make a new one.
            // Start by registering all MUTGOS-specific classes with it.
            // Give it an unlimited, separate heap so base registrations won't
            // count against a running script.
            //
            const memory::MemHeapState create_engine_heap;
            memory::ThreadVirtualHeapManager::set_thread_heap_state(
                create_engine_heap);

            int rc = 0;
            bool register_success = true;
            asIScriptEngine * const new_engine = asCreateScriptEngine();
            StringFactory  * const string_factory =
                new StringFactory(new_engine);

            register_success = AString::register_methods(*new_engine)
                and register_success;
            rc = new_engine->RegisterStringFactory("string", string_factory);

            CScriptArray::SetMemoryFunctions(
                memory::ThreadVirtualHeapManager::mem_alloc,
                memory::ThreadVirtualHeapManager::mem_free);
            RegisterScriptArray(new_engine, true);

            if (rc < 0)
            {
                register_success = false;
                LOG(error, "angelscript", "get_available_engine",
                    "Failed to register string factory with AngelScript.  "
                    "rc = " + text::to_string(rc));
            }

            // Order is important here as there are dependencies!!
            register_success = register_success and
                AEntity::register_methods(*new_engine) and
                OnlineStatEntry::register_methods(*new_engine) and
                DatabaseOps::register_methods(*new_engine) and
                InputOutputOps::register_methods(*new_engine) and
                MovementOps::register_methods(*new_engine) and
                SystemOps::register_methods(*new_engine);

            asIScriptContext * const new_context = new_engine->RequestContext();

            if (register_success and new_context)
            {
                // Successfully created engine and context.  Save off and
                // return.
                //
                engines_used.push_back(EngineContextState(
                    new_engine,
                    new_context,
                    string_factory));

                engine_ptr = new_engine;
                context_ptr = new_context;
                status = true;
            }
            else
            {
                LOG(error, "angelscript", "get_available_engine",
                    "Failed to create engine and context.");
            }
        }

        return status;
    }

    // ----------------------------------------------------------------------
    void AngelScriptAccess::return_used_engine(
        asIScriptEngine * const engine_ptr,
        asIScriptContext * const context_ptr)
    {
        if (engine_ptr and context_ptr)
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            const memory::MemHeapState delete_engine_heap;
            memory::ThreadVirtualHeapManager::set_thread_heap_state(
                delete_engine_heap);

            context_ptr->Abort();
            context_ptr->Unprepare();
            engine_ptr->DiscardModule(SCRIPT_MODULE_NAME.c_str());
            engine_ptr->GarbageCollect();

            // Remove from 'in use' and put in 'avail' if pool size is less
            // than max.
            //
            if (engines_avail.size() > config::angelscript::max_pool_size())
            {
                // Enough engines are in the pool; free this one and
                // remove from list.
                //
                engine_ptr->ReturnContext(context_ptr);
                engine_ptr->ShutDownAndRelease();

                if (engines_used.empty())
                {
                    LOG(error, "angelscript", "return_used_engine",
                        "No used engines stored!");
                }
                else
                {
                    for (size_t used_index = 0;
                         used_index < engines_used.size();
                         ++used_index)
                    {
                        if (engines_used[used_index].engine_ptr == engine_ptr)
                        {
                            // Found it!  Delete string factory and remove.
                            //
                            delete engines_used[used_index].string_factory_ptr;

                            if (used_index == (engines_used.size() - 1))
                            {
                                // Last one, just pop it.
                                engines_used.pop_back();
                            }
                            else
                            {
                                // Shift the last one on top of this one for
                                // a more efficient delete.
                                //
                                engines_used[used_index] = engines_used.back();
                                engines_used.pop_back();
                            }

                            break;
                        }
                    }
                }
            }
            else
            {
                // Keep this engine for future reuse.  Find it from the
                // array of used engines and put it back in available engines.
                //
                if (engines_used.empty())
                {
                    LOG(error, "angelscript", "return_used_engine",
                        "No used engines stored!  Deleting this engine.");

                    engine_ptr->ReturnContext(context_ptr);
                    engine_ptr->ShutDownAndRelease();
                }
                else
                {
                    for (size_t used_index = 0;
                         used_index < engines_used.size();
                         ++used_index)
                    {
                        if (engines_used[used_index].engine_ptr == engine_ptr)
                        {
                            // Found it!  Move into available engines and
                            // remove from used engines.
                            //
                            engines_avail.push_back(engines_used[used_index]);

                            if (used_index == (engines_used.size() - 1))
                            {
                                // Last one, just pop it.
                                engines_used.pop_back();
                            }
                            else
                            {
                                // Shift the last one on top of this one for
                                // a more efficient delete.
                                //
                                engines_used[used_index] = engines_used.back();
                                engines_used.pop_back();
                            }

                            break;
                        }
                    }
                }
            }
        }
        else
        {
            LOG(error, "angelscript", "return_used_engine",
                "engine or context are null!  Cannot return engine.");
        }
    }

    // ----------------------------------------------------------------------
    void AngelScriptAccess::check_compile_rc(
        const int rc,
        const size_t line,
        bool &current_result)
    {
        if (rc < 0)
        {
            current_result = false;

            LOG(info, "angelscript", "check_compile_rc",
                "Failed to compile with AngelScript.  rc = "
                + text::to_string(rc)
                + ",  line = "
                + text::to_string(line));
        }
    }
}
}