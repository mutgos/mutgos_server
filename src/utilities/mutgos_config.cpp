/*
 * mutgos_config.cpp
 */

#include <string>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "mutgos_config.h"

#define MINIMUM_WS_MAX_INCOMING_MESSAGE_SIZE 8192
#define MINIMUM_ANGEL_MAX_HEAP 64
#define MINIMUM_ANGEL_TIMESLICE 50

//
// To add a new key:
//  Add the key in mutgos.conf
//  In the header, add a getter (make new namespace if a new category).
//  In the anonymous namespace right below this, add a constant for the key
//  In the anonymous namespace right below this, add the value with default.
//  In do_parse(), add it to option_desc.
//  In do_parse(), retrieve the value from vars, validate, and set on the
//    anonymous namespace value.
//  Add a getter for the header method.

namespace
{
    const std::string MUTGOS_DEFAULT_CONFIG_FILE_NAME = "mutgos.conf";
    const std::string MUTGOS_DB_DEFAULT_FILE_NAME = "mutgos.db";
    const std::string KEY_DATA_DIR_PATH = "data_dir_path";

    // Defaults are provided, but normally aren't used except by utility
    // programs.
    // Constants for config keys go beyond 80 characters to maintain
    // readability.

    // executor
    //
    const std::string KEY_EXE_THREAD_COUNT = "executor.thread_count";
    MG_UnsignedInt config_exe_thread_count = 2;

    // comm
    //
    const std::string KEY_COMM_AUTH_TIME = "connection.auth_time";
    MG_UnsignedInt config_comm_auth_time = 300;
    const std::string KEY_COMM_IDLE_TIME = "connection.idle_time";
    MG_UnsignedInt config_comm_idle_time = 7200;
    const std::string KEY_COMM_RECONNECT_WAIT_TIME = "connection.reconnect_wait_time";
    MG_UnsignedInt config_comm_reconnect_wait_time = 300;

    const std::string KEY_SO_INPUT_LINE_LENGTH = "connection.socket.input_line_length";
    MG_UnsignedInt config_so_input_line_length = 8192;
    const std::string KEY_SO_ENABLE_SSL = "connection.socket.enable_ssl";
    bool config_so_enable_ssl = false;
    const std::string KEY_SO_ENABLE_PLAIN = "connection.socket.enable_plain";
    bool config_so_enable_plain = true;
    const std::string KEY_SO_PORT = "connection.socket.port";
    MG_UnsignedInt config_so_port = 7072;
    const std::string KEY_SO_PORT_SSL = "connection.socket.port_ssl";
    MG_UnsignedInt config_so_port_ssl = 7073;
    const std::string KEY_SO_CERTIFICATE = "connection.certificate_file";
    std::string config_so_certificate = "server.pem";
    const std::string KEY_SO_CERTIFICATE_PRIVATE = "connection.certificate_private_key_file";
    std::string config_so_certificate_private = "server.pem";

    const std::string KEY_WS_ENABLE_PLAIN = "connection.websocket.enable";
    bool config_ws_enable_plain = true;
    const std::string KEY_WS_PORT = "connection.websocket.port";
    MG_UnsignedInt config_ws_port = 7000;
    const std::string KEY_WS_MAX_WINDOW = "connection.websocket.max_window";
    MG_UnsignedInt config_ws_max_window = 8192;
    const std::string KEY_WS_MAX_INCOMING_MESSAGE_SIZE = "connection.websocket.max_incoming_message_size";
    MG_UnsignedInt config_ws_max_incoming_message_size = 16384;

    // db
    //
    const std::string KEY_DB_FILE = "database.db_file";
    std::string config_db_file = MUTGOS_DB_DEFAULT_FILE_NAME;
    const std::string KEY_DB_PASSWORD_WORKFACTOR = "database.password_workfactor";
    MG_UnsignedInt config_db_password_workfactor = 10;

    // Angelscript
    //
    const std::string KEY_ANGEL_MAX_HEAP = "angelscript.max_heap";
    MG_UnsignedInt config_angel_max_heap = 1024;
    const std::string KEY_ANGEL_TIMESLICE = "angelscript.timeslice";
    MG_UnsignedInt config_angel_timeslice = 300;
    const std::string KEY_ANGEL_MAX_POOL_SIZE = "angelscript.max_pool_size";
    MG_UnsignedInt config_angel_max_pool_size = 4;
}

namespace mutgos
{
namespace config
{
    //
    // Private static functions
    //

    // ----------------------------------------------------------------------
    /**
     * Simple validator for a uint key's value.  If validation fails,
     * it will log a suitable error.  If validation succeeds, it will
     * log the value as an info.
     * @param key[in] The key name, for logging purposes.
     * @param value[in] The value to check.
     * @param success[out] If the value does not validate, this will be set
     * to false.
     * @param min_value[in] The minimum that value can be.
     * @param max_value[in] The maximum that value can be.
     */
    void validate_uint(
        const std::string &key,
        const MG_UnsignedInt value,
        bool &success,
        const MG_UnsignedInt min_value = 1,
        const MG_UnsignedInt max_value = 0)
    {
        if (value < min_value)
        {
            LOG(fatal, "config", "validate_uint",
                key + " setting is not >= " + text::to_string(min_value));
            success = false;
        }
        else if (max_value and (value > max_value))
        {
            LOG(fatal, "config", "validate_uint",
                key + " setting is not <=" + text::to_string(max_value));
            success = false;
        }
        else
        {
            LOG(info, "config", "validate_uint", key + " set to "
                + text::to_string(value));
        }
    }

    /**
     * Validates and adjusts the file to have a prefixed path, if appropriate.
     * @param key[in] The key name, for logging purposes.
     * @param data_dir_prefix[in] The prefix to the data directory, with a
     * trailing slash.
     * @param must_exist[in] If true, file must exist at the location.
     * @param filename[in,out] The filename to validate.  It may be modified
     * to be normalized for the platform and have a prefixed path.
     * @param success[out] If the value does not validate, this will be set
     * to false.
     */
    void validate_file(
        const std::string &key,
        const std::string &data_dir_prefix,
        const bool must_exist,
        std::string &filename,
        bool &success)
    {
        if (filename.empty())
        {
            success = false;
            LOG(fatal, "config", "validate_file",
                "File name is empty for key: " + key);
        }
        else
        {
            boost::filesystem::path normalized_file;

            if ((filename[0] == '/') or (filename[0] == '\\'))
            {
                // Starts with a separator, so it's absolute
                normalized_file = filename;
            }
            else
            {
                // Relative
                normalized_file = data_dir_prefix + filename;
            }

            filename = normalized_file.native();

            LOG(info, "config", "validate_file",
                key + " set to " + filename);

            if (not boost::filesystem::is_regular_file(normalized_file))
            {
                if (must_exist)
                {
                    success = false;
                }

                LOG((must_exist ? fatal : warning), "config", "validate_file",
                    "For key " + key + ", file does not exist: "
                    + filename);
            }
        }
    }

    // ----------------------------------------------------------------------
    /**
     * Parses the config file and sets all the file-scope variables.
     * @param config_stream[in] The config file stream to parse.
     * @param data_dir_override[in] If not empty, an override for the data
     * directory prefix.
     * @return True if parse was successful.
     */
    bool do_parse(
        std::ifstream &config_stream,
        const std::string &data_dir_override)
    {
        bool success = true;

        // Define the options
        //
        boost::program_options::options_description
            option_desc("Config file options");

        // Not adding description to each key for now because this is the
        // config file; descriptions are found in the file itself.
        option_desc.add_options()
            // PATH
            //
            (KEY_DATA_DIR_PATH.c_str(),
                boost::program_options::value<std::string>(), "")

            // Executor
            //
            (KEY_EXE_THREAD_COUNT.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_exe_thread_count), "")

            // Comm
            //
            (KEY_COMM_AUTH_TIME.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_comm_auth_time), "")
            (KEY_COMM_IDLE_TIME.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_comm_idle_time), "")
            (KEY_COMM_RECONNECT_WAIT_TIME.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_comm_reconnect_wait_time), "")

            (KEY_SO_INPUT_LINE_LENGTH.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_so_input_line_length), "")
            (KEY_SO_ENABLE_SSL.c_str(),
                boost::program_options::value<bool>()->
                    default_value(config_so_enable_ssl), "")
            (KEY_SO_ENABLE_PLAIN.c_str(),
                boost::program_options::value<bool>()->
                    default_value(config_so_enable_plain), "")
            (KEY_SO_PORT.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_so_port), "")
            (KEY_SO_PORT_SSL.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_so_port_ssl), "")
            (KEY_SO_CERTIFICATE.c_str(),
                boost::program_options::value<std::string>()->
                    default_value(config_so_certificate), "")
            (KEY_SO_CERTIFICATE_PRIVATE.c_str(),
                boost::program_options::value<std::string>()->
                    default_value(config_so_certificate_private), "")

            (KEY_WS_ENABLE_PLAIN.c_str(),
                boost::program_options::value<bool>()->
                    default_value(config_ws_enable_plain), "")
            (KEY_WS_PORT.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_ws_port), "")
            (KEY_WS_MAX_WINDOW.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_ws_max_window), "")
            (KEY_WS_MAX_INCOMING_MESSAGE_SIZE.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_ws_max_incoming_message_size), "")

            // db
            //
            (KEY_DB_FILE.c_str(),
                boost::program_options::value<std::string>()->
                    default_value(config_db_file), "")
            (KEY_DB_PASSWORD_WORKFACTOR.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_db_password_workfactor), "")

            // Angelscript
            //
            (KEY_ANGEL_MAX_HEAP.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_angel_max_heap), "")
            (KEY_ANGEL_TIMESLICE.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_angel_timeslice), "")
            (KEY_ANGEL_MAX_POOL_SIZE.c_str(),
                boost::program_options::value<MG_UnsignedInt>()->
                    default_value(config_angel_max_pool_size), "")
        ;


        // Parse it.
        //
        try
        {
            boost::program_options::variables_map vars;

            boost::program_options::store(
                boost::program_options::parse_config_file(
                    config_stream,
                    option_desc,
                    false),
                vars);

            boost::program_options::notify(vars);

            // Extract and do basic validation of contents
            //

            std::string data_dir_prefix = "/";
            boost::filesystem::path normalized_file;

            if (data_dir_override.empty())
            {
                data_dir_prefix =
                    vars[KEY_DATA_DIR_PATH].as<std::string>()
                    + data_dir_prefix;
            }
            else
            {
                LOG(info, "config", "do_parse", "Data directory override.");

                data_dir_prefix = data_dir_override + data_dir_prefix;
            }

            LOG(info, "config", "do_parse",
                "Data directory: " + data_dir_prefix);

            normalized_file = data_dir_prefix;

            if (not boost::filesystem::is_directory(normalized_file))
            {
                LOG(fatal, "config", "do_parse", "Data directory "
                    + data_dir_prefix + " does not exist!");
                success = false;

                // End early to make it easier to see error.
                return success;
            }

            // Executor
            //
            config_exe_thread_count =
                vars[KEY_EXE_THREAD_COUNT].as<MG_UnsignedInt>();
            validate_uint(KEY_EXE_THREAD_COUNT, config_exe_thread_count, success);

            // Comm
            //
            config_comm_auth_time =
                vars[KEY_COMM_AUTH_TIME].as<MG_UnsignedInt>();
            validate_uint(KEY_COMM_AUTH_TIME, config_comm_auth_time, success);

            config_comm_idle_time =
                vars[KEY_COMM_IDLE_TIME].as<MG_UnsignedInt>();
            validate_uint(KEY_COMM_IDLE_TIME, config_comm_idle_time, success);

            config_comm_reconnect_wait_time =
                vars[KEY_COMM_RECONNECT_WAIT_TIME].as<MG_UnsignedInt>();
            validate_uint(
                KEY_COMM_RECONNECT_WAIT_TIME,
                config_comm_reconnect_wait_time,
                success);

            config_so_input_line_length =
                vars[KEY_SO_INPUT_LINE_LENGTH].as<MG_UnsignedInt>();
            validate_uint(
                KEY_SO_INPUT_LINE_LENGTH,
                config_so_input_line_length,
                success,
                80);

            config_so_enable_ssl = vars[KEY_SO_ENABLE_SSL].as<bool>();
            LOG(info, "config", "do_parse",
                KEY_SO_ENABLE_SSL + " set to "
                + text::to_string(config_so_enable_ssl));

            config_so_enable_plain = vars[KEY_SO_ENABLE_PLAIN].as<bool>();
            LOG(info, "config", "do_parse",
                KEY_SO_ENABLE_PLAIN + " set to "
                + text::to_string(config_so_enable_plain));

            config_so_port = vars[KEY_SO_PORT].as<MG_UnsignedInt>();
            validate_uint(KEY_SO_PORT, config_so_port, success);

            config_so_port_ssl = vars[KEY_SO_PORT_SSL].as<MG_UnsignedInt>();
            validate_uint(KEY_SO_PORT_SSL, config_so_port_ssl, success);

            config_so_certificate = vars[KEY_SO_CERTIFICATE].as<std::string>();
            validate_file(
                KEY_SO_CERTIFICATE,
                data_dir_prefix,
                config_so_enable_ssl,
                config_so_certificate,
                success);


            config_so_certificate_private =
                vars[KEY_SO_CERTIFICATE_PRIVATE].as<std::string>();
            validate_file(
                KEY_SO_CERTIFICATE_PRIVATE,
                data_dir_prefix,
                config_so_enable_ssl,
                config_so_certificate_private,
                success);

            config_ws_enable_plain = vars[KEY_WS_ENABLE_PLAIN].as<bool>();
            LOG(info, "config", "do_parse",
                KEY_WS_ENABLE_PLAIN + " set to "
                + text::to_string(config_ws_enable_plain));

            config_ws_port = vars[KEY_WS_PORT].as<MG_UnsignedInt>();
            validate_uint(KEY_WS_PORT, config_ws_port, success);

            config_ws_max_window = vars[KEY_WS_MAX_WINDOW].as<MG_UnsignedInt>();
            validate_uint(KEY_WS_MAX_WINDOW, config_ws_max_window, success);

            config_ws_max_incoming_message_size =
                vars[KEY_WS_MAX_INCOMING_MESSAGE_SIZE].as<MG_UnsignedInt>();
            validate_uint(
                KEY_WS_MAX_INCOMING_MESSAGE_SIZE,
                config_ws_max_incoming_message_size,
                success,
                MINIMUM_WS_MAX_INCOMING_MESSAGE_SIZE);


            // db
            //

            config_db_file = vars[KEY_DB_FILE].as<std::string>();
            // Database file may not exist if this is the import program;
            // it will be automatically created.
            validate_file(
                KEY_DB_FILE,
                data_dir_prefix,
                false,
                config_db_file,
                success);


            config_db_password_workfactor =
                vars[KEY_DB_PASSWORD_WORKFACTOR].as<MG_UnsignedInt>();
            validate_uint(
                KEY_DB_PASSWORD_WORKFACTOR,
                config_db_password_workfactor,
                success);

            // Angelscript
            //
            config_angel_max_heap = vars[KEY_ANGEL_MAX_HEAP].as<MG_UnsignedInt>();
            validate_uint(
                KEY_ANGEL_MAX_HEAP,
                config_angel_max_heap,
                success,
                MINIMUM_ANGEL_MAX_HEAP);

            config_angel_timeslice = vars[KEY_ANGEL_TIMESLICE].as<MG_UnsignedInt>();
            validate_uint(
                KEY_ANGEL_TIMESLICE,
                config_angel_timeslice,
                success,
                MINIMUM_ANGEL_TIMESLICE);

            config_angel_max_pool_size =
                vars[KEY_ANGEL_MAX_POOL_SIZE].as<MG_UnsignedInt>();
            validate_uint(
                KEY_ANGEL_MAX_POOL_SIZE,
                config_angel_max_pool_size,
                success);
        }
        catch (boost::program_options::unknown_option &uoex)
        {
            success = false;

            LOG(fatal, "config", "do_parse",
                "Unknown option: " + std::string(uoex.get_option_name()));
        }
        catch (boost::program_options::validation_error &vex)
        {
            success = false;

            LOG(fatal, "config", "do_parse",
                "Bad value for option: " + std::string(vex.get_option_name()));
        }
        catch (boost::program_options::multiple_occurrences &moex)
        {
            success = false;

            LOG(fatal, "config", "do_parse",
                "More than one instance of option: "
                + std::string(moex.get_option_name()));
        }
        catch (boost::program_options::multiple_values &mvex)
        {
            success = false;

            LOG(fatal, "config", "do_parse",
                "More than one value of option: "
                + std::string(mvex.get_option_name()));
        }
        catch (boost::program_options::error &eex)
        {
            success = false;

            LOG(fatal, "config", "do_parse",
                "Error parsing config file: " + std::string(eex.what()));
        }
        catch (boost::filesystem::filesystem_error &er)
        {
            LOG(fatal, "config", "do_parse",
                "Unable to parse file path.  Error: "
                + std::string(er.what()));

            success = false;
        }

        return success;
    }


    //
    // Public static functions
    //

    // ----------------------------------------------------------------------
    bool parse_config(
        const std::string &config_file_name,
        const std::string &data_dir_override)
    {
        bool success = true;

        try
        {
            boost::filesystem::path normalized_file;
            std::ifstream config_stream;

            // Find which file to use
            //
            if (not config_file_name.empty())
            {
                // User provided.
                //
                normalized_file = config_file_name;
                config_stream.open(normalized_file.native());

                if (! config_stream.fail())
                {
                    LOG(info, "config", "parse_config",
                        "Loading config file " + normalized_file.native());
                }
                else
                {
                    success = false;
                    config_stream.close();

                    LOG(fatal, "config", "parse_config",
                        "Could not open config file " + normalized_file.native());
                }
            }
            else
            {
                // User did not provide it.  Try a few common locations first.
                //
                normalized_file = MUTGOS_DEFAULT_CONFIG_FILE_NAME;

                config_stream.open(normalized_file.native());

                if (config_stream.fail())
                {
                    // Couldn't find file.  Try data directory.
                    //
                    config_stream.close();
                    config_stream.clear();

                    normalized_file =
                        "../data/" + MUTGOS_DEFAULT_CONFIG_FILE_NAME;

                    config_stream.open(normalized_file.native());
                }

                if (not config_stream.fail())
                {
                    LOG(info, "config", "parse_config",
                        "Loading config file " + normalized_file.native());
                }
                else
                {
                    success = false;

                    config_stream.close();
                    config_stream.clear();

                    LOG(fatal, "config", "parse_config",
                        "No config file available.");
                }
            }

            if (success)
            {
                success = do_parse(config_stream, data_dir_override);
            }
        }
        catch (boost::filesystem::filesystem_error &er)
        {
            LOG(fatal, "config", "parse_config",
                "Unable to parse config file path.  Error: "
                + std::string(er.what()));

            success = false;
        }

        return success;
    }


namespace executor
{
    // ----------------------------------------------------------------------
    MG_UnsignedInt thread_count(void)
    {
        return config_exe_thread_count;
    }
}

namespace comm
{
    // ----------------------------------------------------------------------
    MG_UnsignedInt auth_time(void)
    {
        return config_comm_auth_time;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt idle_time(void)
    {
        return config_comm_idle_time;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt reconnect_wait_time(void)
    {
        return config_comm_reconnect_wait_time;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt so_input_line_length(void)
    {
        return config_so_input_line_length;
    }

    // ----------------------------------------------------------------------
    bool so_enable_ssl(void)
    {
        return config_so_enable_ssl;
    }

    // ----------------------------------------------------------------------
    bool so_enable_plain(void)
    {
        return config_so_enable_plain;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt so_port(void)
    {
        return config_so_port;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt so_port_ssl(void)
    {
        return config_so_port_ssl;
    }

    // ----------------------------------------------------------------------
    const std::string &so_certificate(void)
    {
        return config_so_certificate;
    }

    // ----------------------------------------------------------------------
    const std::string &so_certificate_private(void)
    {
        return config_so_certificate_private;
    }

    // ----------------------------------------------------------------------
    bool ws_enable_plain(void)
    {
        return config_ws_enable_plain;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt ws_port(void)
    {
        return config_ws_port;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt ws_max_window(void)
    {
        return config_ws_max_window;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt ws_max_incoming_message_size(void)
    {
        return config_ws_max_incoming_message_size;
    }
}

namespace db
{
    // ----------------------------------------------------------------------
    const std::string &db_file(void)
    {
        return config_db_file;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt password_workfactor(void)
    {
        return config_db_password_workfactor;
    }
}

namespace angelscript
{
    // ----------------------------------------------------------------------
    MG_UnsignedInt max_heap(void)
    {
        return config_angel_max_heap;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt timeslice(void)
    {
        return config_angel_timeslice;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt max_pool_size(void)
    {
        return config_angel_max_pool_size;
    }
}

}
}
