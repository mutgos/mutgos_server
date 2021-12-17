/*
 * mutgos_server_main.cpp
 */

#include <unistd.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "utilities/mutgos_config.h"

#include "osinterface/osinterface_Signals.h"

#include "utilities/memory_ThreadVirtualHeapManager.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "executor/executor_ExecutorAccess.h"
#include "events/events_EventAccess.h"
#include "comminterface/comm_CommAccess.h"
#include "security/security_SecurityAccess.h"
#include "primitives/primitives_PrimitivesAccess.h"
#include "primitives/primitives_NameRegistry.h"
#include "softcode/softcode_SoftcodeAccess.h"
#include "angelscriptinterface/angelscript_AngelScriptAccess.h"

#include "useragent/useragent_ConnectionLifecycleManager.h"
#include "useragent/useragent_EntityNameManager.h"

#include "dbtypes/dbtype_TimeStamp.h"

#define HELP_ARG "help"
#define CONFIGFILE_ARG "configfile"
#define DATADIR_ARG "datadir"

/**
 * Parses the commandline.
 * @param options[in] The arguments allowed.
 * @param args[out] The parsed arguments.
 * @param argc[in] Argument count from the raw commandline.
 * @param argv[in] The raw arguments from the raw commandline.
 * @return True if successfully parsed.
 */
bool parse_commandline(
    boost::program_options::options_description &options,
    boost::program_options::variables_map &args,
    int argc,
    char *argv[])
{
    bool success = true;

    try
    {
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, options),
            args);
        boost::program_options::notify(args);
    }
    catch (boost::program_options::unknown_option &uoex)
    {
        success = false;

        std::cout << "ERROR: "
            << "Unknown argument: " << uoex.get_option_name() << std::endl;
    }
    catch (boost::program_options::validation_error &vex)
    {
        success = false;

        std::cout << "ERROR: "
            << "Bad value for argument: " << vex.get_option_name()
            << std::endl;
    }
    catch (boost::program_options::multiple_occurrences &moex)
    {
        success = false;

        std::cout << "ERROR: "
            << "More than one instance of argument: "
            << moex.get_option_name() << std::endl;
    }
    catch (boost::program_options::multiple_values &mvex)
    {
        success = false;

        std::cout << "ERROR: "
            << "More than one value of argument: "
            << mvex.get_option_name() << std::endl;
    }
    catch (boost::program_options::error &eex)
    {
        success = false;

        std::cout << "ERROR: "
            << "Error parsing arguments: "  << eex.what() << std::endl;
    }

    return success;
}

int main(int argc, char* argv[])
{
    bool good_init = true;

    mutgos::memory::ThreadVirtualHeapManager::add_thread();
    mutgos::log::Logger::init(true);

    boost::program_options::options_description
        option_desc("MUTGOS Server Options");

    option_desc.add_options()
        (HELP_ARG, "Show this help screen")
        (CONFIGFILE_ARG,
            boost::program_options::value<std::string>(),
            "The config file to load and use.  Default is mutgos.conf "
            "in working directory.")
        (DATADIR_ARG,
            boost::program_options::value<std::string>(),
            "Override the data directory specified in the config file.")
    ;

    boost::program_options::variables_map args;
    std::string configfile;
    std::string datadir;
    good_init = parse_commandline(option_desc, args, argc, argv);

    if (not good_init)
    {
        // Error message already printed.
        return -1;
    }

    // Extract arguments, print help screen if needed.
    //
    if (args.count(CONFIGFILE_ARG))
    {
        configfile = args[CONFIGFILE_ARG].as<std::string>();
    }

    if (args.count(DATADIR_ARG))
    {
        datadir = args[DATADIR_ARG].as<std::string>();
    }

    if (args.count(HELP_ARG))
    {
        std::cout << option_desc << std::endl;
        return 0;
    }

    // Parse config file, now that we know where it is.
    //
    good_init = mutgos::config::parse_config(configfile, datadir);

    if (not good_init)
    {
        std::cout << "ERROR: Failed to parse config file." << std::endl;
        return -1;
    }

    // Bring up the system.
    //

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
     *
     * Shutdown order may vary.
     */

    if (good_init and
        not mutgos::dbinterface::DatabaseAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init dbinterface" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::executor::ExecutorAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init executor" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::events::EventAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init events" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::comm::CommAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init comm" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::security::SecurityAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init security" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::primitives::NameRegistry::make_singleton())
    {
        std::cout << "Failed to init name registry" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::primitives::PrimitivesAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init primitives" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::softcode::SoftcodeAccess::make_singleton())
    {
        std::cout << "Failed to init softcode" << std::endl;
        good_init = false;
    }

    if (good_init and
        not mutgos::angelscript::AngelScriptAccess::make_singleton()->startup())
    {
        std::cout << "Failed to init angelscript" << std::endl;
        good_init = false;
    }

    if (good_init)
    {
        mutgos::useragent::ConnectionLifecycleManager * const manager_ptr =
            new mutgos::useragent::ConnectionLifecycleManager();
        mutgos::useragent::EntityNameManager * const name_manager_ptr =
            new mutgos::useragent::EntityNameManager();

        const mutgos::executor::PID manager_pid =
            mutgos::executor::ExecutorAccess::instance()->add_process(
                manager_ptr);
        const mutgos::executor::PID name_manager_pid =
            mutgos::executor::ExecutorAccess::instance()->add_process(
                name_manager_ptr);

        if (not manager_pid)
        {
            std::cout << "Failed to add lifecycle manager" << std::endl;
            good_init = false;
        }
        else
        {
            if (not mutgos::executor::ExecutorAccess::instance()
                ->start_process(manager_pid))
            {
                std::cout << "Failed to start lifecycle manager" << std::endl;
                good_init = false;
            }
        }

        if (not name_manager_pid)
        {
            std::cout << "Failed to add name manager" << std::endl;
            good_init = false;
        }
        else
        {
            if (not mutgos::executor::ExecutorAccess::instance()
                ->start_process(name_manager_pid))
            {
                std::cout << "Failed to start name manager" << std::endl;
                good_init = false;
            }
        }

        if (good_init)
        {
            mutgos::osinterface::Signals::register_quit();

            // Stay here so the program doesn't exit.  The other threads
            // are what actually do all the MUTGOS work.
            //
            // During the loop, detect any large jumps in time.  This is
            // mostly to make sure database commits continue to happen
            // in the background.  It's a very basic detection for now
            // and is not designed to detect frequently occuring jumps.
            //
            mutgos::dbtype::TimeStamp prev_time;
            mutgos::dbtype::TimeStamp current_time = prev_time;

            while (not mutgos::osinterface::Signals::got_quit_signal())
            {
                sleep(10);

                prev_time = current_time;
                current_time = mutgos::dbtype::TimeStamp();

                bool negative = false;
                const MG_LongUnsignedInt relative_diff =
                    prev_time.get_relative_seconds(
                        current_time,
                        negative);

                if (relative_diff > 30)
                {
                    std::cout << "** Time jump detected! **" << std::endl;

                    // For now, don't have a whole listener infrastructure;
                    // just call the two we know that care about time jumps.

                    mutgos::dbinterface::DatabaseAccess::instance()->
                        os_time_has_jumped(negative);
                    mutgos::executor::ExecutorAccess::instance()->
                        os_time_has_jumped(negative);
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
    mutgos::primitives::NameRegistry::destroy_singleton();

    mutgos::memory::ThreadVirtualHeapManager::delete_thread();

    return (good_init ? 0 : -1);
}
