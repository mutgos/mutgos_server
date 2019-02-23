/*
 * angelscript_td.cpp
 */

#include <string>
#include <iostream>
#include <angelscript.h>

#include "utilities/memory_ThreadVirtualHeapManager.h"
#include "utilities/memory_MemHeapState.h"

#include "angelscriptinterface/angelscript_AString.h"
#include "angelscriptinterface/angelscript_StringFactory.h"

using namespace mutgos;

unsigned int lines_executed = 0;

void MessageCallback(const asSMessageInfo *msg, void *param)
{
    const char *type = "ERR ";
    if( msg->type == asMSGTYPE_WARNING )
        type = "WARN";
    else if( msg->type == asMSGTYPE_INFORMATION )
        type = "INFO";

    std::cout << "** AngelScript: " << msg->section << " (" << msg->row
              << ", " << msg->col << ") : " << type << " : "
              << msg->message << std::endl;
}

void DebugLineCallback(asIScriptContext *ctx, void *dbg)
{
    ++lines_executed;

    if (lines_executed > 50)
    {
        // Executed too many ,so abort.
        ctx->Suspend();
    }
}

void angel_print(const angelscript::AString &data)
{
    const std::string data_exported = data.export_to_string();
    std::cout << ">> " << data_exported << std::endl;
}

// Adapted from sample angelscript code provided with library.
int main(void)
{
    int rc = 0;
    memory::MemHeapState heap_state(1024 * 2048);

    memory::ThreadVirtualHeapManager::add_thread();
    memory::ThreadVirtualHeapManager::set_thread_heap_state(heap_state);

    asSetGlobalMemoryFunctions(
        memory::ThreadVirtualHeapManager::mem_alloc_nofail,
        memory::ThreadVirtualHeapManager::mem_free);
    asPrepareMultithread();

    asIScriptEngine *engine = asCreateScriptEngine();

    engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

    angelscript::StringFactory string_factory(engine);

    angelscript::AString::register_methods(*engine);
    rc = engine->RegisterStringFactory("string", &string_factory);
    engine->SetDefaultNamespace("ns");
    engine->RegisterGlobalFunction(
        "void print(const string &in str)",
        asFUNCTION(angel_print), asCALL_CDECL);
    engine->SetDefaultNamespace("");
    engine->RegisterGlobalFunction(
        "void print(const string &in str)",
        asFUNCTION(angel_print), asCALL_CDECL);

    // Create our context, prepare it, and then execute
    asIScriptContext *ctx = engine->RequestContext();

    if (rc < 0)
    {
        std::cerr << "Unable to register string factory: " << rc << std::endl;
        return -1;
    }

    const char *script1 =
        "void main() \n"
        "{\n"
//        "  string test;\n"
//        "  test.size();\n"
//        "test.assign(\"My assign string.\");\n"
        "  string test = \"My Test String\";\n"
        "  test += \"  More Data\";\n"
        "  string lower = test.to_lower();\n"
        "  print(lower);\n"
        "  if (lower == test) print(\"equals\");\n"
        "  string other = test + \" extra\";\n"
        "  print(other);\n"
        "  ns::print(\"NS Test!\");\n"
//        "  while (true) { other += other; }\n"
        "}\n";

    asIScriptModule *mod = engine->GetModule("script1", asGM_ALWAYS_CREATE);
    rc = mod->AddScriptSection("script1", script1, strlen(script1));

    if (rc < 0)
    {
        std::cerr << "AddScriptSection() failed: " << rc << std::endl;
        return -1;
    }

    std::cout << "Compiling..." << std::endl;
    rc = mod->Build();

    if (rc < 0)
    {
        std::cerr << "Build() failed: " << rc << std::endl;
        return -1;
    }

    asIScriptFunction *func = mod->GetFunctionByDecl("void main()");

    if (not func)
    {
        // The function couldn't be found. Instruct the script writer
        // to include the expected function in the script.
        std::cerr << "Could not find main func." << std::endl;
        return -1;
    }

    // ctx->SetLineCallback(asFUNCTION(DebugLineCallback), 0, asCALL_CDECL);
    ctx->Prepare(func);

    std::cout << "Running..." << std::endl;
    rc = ctx->Execute();

    if (rc != asEXECUTION_FINISHED )
    {
        // The execution didn't complete as expected. Determine what happened.
        if ( rc == asEXECUTION_EXCEPTION )
        {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            std::cerr << "ERROR: Exception thrown while executing: "
                      << ctx->GetExceptionString()
                      << std::endl;
        }
        else
        {
            std::cerr << "ERROR: Script did not complete."  << std::endl;
        }
    }

    heap_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

    std::cout << "Cleaning up..." << std::endl;

    func = 0;
    engine->ReturnContext(ctx);
    ctx = 0;

    engine->DiscardModule("script1");
    engine->ShutDownAndRelease();
    asThreadCleanup();

    heap_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

    // Cleanup.
    memory::ThreadVirtualHeapManager::delete_thread();

    std::cout << "DONE." << std::endl;
    return 0;
}