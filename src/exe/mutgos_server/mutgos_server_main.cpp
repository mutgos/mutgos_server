/*
 * mutgos_server_main.cpp
 */

#include <unistd.h>
#include <string>
#include <iostream>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "osinterface/osinterface_Signals.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "executor/executor_ExecutorAccess.h"
#include "events/events_EventAccess.h"
#include "comminterface/comm_CommAccess.h"
#include "security/security_SecurityAccess.h"
#include "primitives/primitives_PrimitivesAccess.h"
#include "softcode/softcode_SoftcodeAccess.h"
#include "angelscriptinterface/angelscript_AngelScriptAccess.h"

#include "useragent/useragent_ConnectionLifecycleManager.h"

int main(int argc, char* argv[])
{
    bool good_init = true;

    mutgos::memory::ThreadVirtualHeapManager::add_thread();

    mutgos::log::Logger::init(true);

    /**
     * Order:
     * DatabaseAccess
     * ExecutorAccess
     * EventAccess
     * CommAccess (at end??)
     * SecurityAccess
     * PrimitivesAccess
     * SoftcodeAccess
     * AngelScriptAccess
     */

    if (not mutgos::dbinterface::DatabaseAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init dbinterface" << std::endl;
        good_init = false;
    }

    if (not mutgos::executor::ExecutorAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init executor" << std::endl;
        good_init = false;
    }

    if (not mutgos::events::EventAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init events" << std::endl;
        good_init = false;
    }

    if (not mutgos::comm::CommAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init comm" << std::endl;
        good_init = false;
    }

    if (not mutgos::security::SecurityAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init security" << std::endl;
        good_init = false;
    }

    if (not mutgos::primitives::PrimitivesAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init primitives" << std::endl;
        good_init = false;
    }

    if (not mutgos::softcode::SoftcodeAccess::make_singleton())
    {
        std::cout << "Failed to init softcode" << std::endl;
        good_init = false;
    }

    if (not mutgos::angelscript::AngelScriptAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init angelscript" << std::endl;
        good_init = false;
    }

    if (good_init)
    {
        mutgos::useragent::ConnectionLifecycleManager * const manager_ptr =
            new mutgos::useragent::ConnectionLifecycleManager();

        const mutgos::executor::PID manager_pid =
            mutgos::executor::ExecutorAccess::instance()->add_process(
                manager_ptr);

        if (not manager_pid)
        {
            std::cout << "Failed to add lifecycle manager" << std::endl;
            good_init = false;
        }
        else
        {
            if (not mutgos::executor::ExecutorAccess::instance()->start_process(
                manager_pid))
            {
                std::cout << "Failed to start lifecycle manager" << std::endl;
                good_init = false;
            }

            if (good_init)
            {
                mutgos::osinterface::Signals::register_quit();

                // Stay here so the program doesn't exit.  The other threads
                // are what actually do all the MUTGOS work.
                while (not mutgos::osinterface::Signals::got_quit_signal())
                {
                    sleep(10);
                }
            }
        }
    }

    // We're coming down.  Shut down everything and destruct.
    //
    mutgos::comm::CommAccess::make_singleton()->shutdown();
    mutgos::executor::ExecutorAccess::make_singleton()->shutdown();
    mutgos::angelscript::AngelScriptAccess::make_singleton()->shutdown();
    mutgos::primitives::PrimitivesAccess::make_singleton()->shutdown();
    mutgos::security::SecurityAccess::make_singleton()->shutdown();
    mutgos::dbinterface::DatabaseAccess::make_singleton()->shutdown();
    mutgos::events::EventAccess::make_singleton()->shutdown();

    mutgos::angelscript::AngelScriptAccess::destroy_singleton();
    mutgos::softcode::SoftcodeAccess::destroy_singleton();
    mutgos::primitives::PrimitivesAccess::destroy_singleton();
    mutgos::security::SecurityAccess::destroy_singleton();
    mutgos::comm::CommAccess::destroy_singleton();
    mutgos::events::EventAccess::destroy_singleton();
    mutgos::executor::ExecutorAccess::destroy_singleton();
    mutgos::dbinterface::DatabaseAccess::destroy_singleton();

    mutgos::memory::ThreadVirtualHeapManager::delete_thread();

    return 0;
}
