/*
 * read_dump.cpp
 */

#include <string>
#include <iostream>
#include "text/text_StringConversion.h"
#include <boost/program_options.hpp>

#include "logging/log_Logger.h"
#include "dbdump/dbdump_MutgosDumpFileReader.h"
#include "utilities/mutgos_config.h"

#define HELP_ARG "help"
#define CONFIGFILE_ARG "configfile"
#define DUMPFILE_ARG "dumpfile"
#define DATAFILE_ARG "datafile"

// TODO Update documentation for how to run.

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
    int rc = 0;

    std::cout << "Read Dump Utility.  Use --help for usage information."
              << std::endl
              << std::endl;

    boost::program_options::options_description
        option_desc("Read Dump Utility Options");

    option_desc.add_options()
           (HELP_ARG, "Show this help screen")
           (CONFIGFILE_ARG,
               boost::program_options::value<std::string>(),
               "The config file to load and use.  Default is mutgos.conf")
           (DUMPFILE_ARG,
               boost::program_options::value<std::string>(),
               "The dump file to read in.  Default is mutgos.dump")
           (DATAFILE_ARG,
               boost::program_options::value<std::string>(),
               "Specifies the path and filename to save the generated database to.  Default is mutgos.db")
        ;

    boost::program_options::variables_map args;
    std::string config_file = "mutgos.conf";
    std::string dump_file = "mutgos.dump";
    std::string data_file = "mutgos.db";
    const bool good_parse = parse_commandline(option_desc, args, argc, argv);

    if (not good_parse)
    {
        // Error message already printed.
        return -1;
    }

    if (args.count(HELP_ARG))
    {
        std::cout << option_desc << std::endl;
        return 0;
    }

    if (args.count(CONFIGFILE_ARG))
    {
        config_file = args[CONFIGFILE_ARG].as<std::string>();
    }

    if (args.count(DUMPFILE_ARG))
    {
        dump_file = args[DUMPFILE_ARG].as<std::string>();
    }

    if (args.count(DATAFILE_ARG))
    {
        data_file = args[DATAFILE_ARG].as<std::string>();
    }

    mutgos::log::Logger::init(true);
    const bool good_config_read = mutgos::config::parse_config(config_file, "");

    if (not good_config_read)
    {
        std::cout << "ERROR: Failed to parse config file." << std::endl;
        return -1;
    }

    std::string message;
    mutgos::dbdump::MutgosDumpFileReader reader(dump_file);

    if (reader.parse(message))
    {
        std::cout << "Success: Parsing complete." << std::endl
                  << "  Message: " << message << std::endl;
    }
    else
    {
        std::cerr << "FAILURE: Parsing did NOT complete." << std::endl
                  << "  Line: " << mutgos::text::to_string(
                     reader.get_current_line_index()) << std::endl
                  << "  Message: " << message << std::endl;

        rc = -1;
    }

    return rc;
}