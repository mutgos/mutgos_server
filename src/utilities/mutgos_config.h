/*
 * mutgos_config.h
 */

#ifndef MUTGOS_CONFIG_H
#define MUTGOS_CONFIG_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

/**
 * This file is set up unlike any other in MUTGOS, to allow for quick, easy
 * access to config file options anywhere in the program.  It is essentially
 * a series of static methods organized by namespace, and is designed to
 * closely mirror the config file itself.
 */

namespace mutgos
{
namespace config
{
    /**
     * Parses the config file to make all the options available via static
     * methods.  This must be called before any of the accessors are used,
     * typically by a 'main' method.
     * If this is not called, defaults do exist; those are meant for utility
     * programs and no the main server, however.
     * @param config_file_name[in] The config file to parse.  If empty, will
     * try current directory, and then ../data/
     * @param data_dir_override[in] An override for the data directory
     * specified in the config file.  If blank, do not override.
     * @return True if successfully parsed.
     */
    bool parse_config(
        const std::string &config_file_name,
        const std::string &data_dir_override);

    /**
     * Config file options related to the Executor
     */
    namespace executor
    {
        /**
         * @return How many threads the executor should use.
         */
        MG_UnsignedInt thread_count(void);
    }


    /**
     * Config file options related to the communications / connection
     * subsystem.
     */
    namespace comm
    {
        /**
         * @return How many seconds to wait for authentication by a client
         * to complete.
         */
        MG_UnsignedInt auth_time(void);

        /**
         * @return How many seconds of inactivity before a client is
         * disconnected.
         */
        MG_UnsignedInt idle_time(void);

        /**
         * @return How many seconds after a Player improperly disconnected
         * that their spot will be 'held' and messages queued up.
         */
        MG_UnsignedInt reconnect_wait_time(void);

        /**
         * @return Maximum size of a single line input by a client, in bytes.
         */
        MG_UnsignedInt so_input_line_length(void);

        /**
         * @return True to enable SSL support.
         */
        bool so_enable_ssl(void);

        /**
         * @return True to enable plaintext (non-SSL) support.
         */
        bool so_enable_plain(void);

        /**
         * @return The main unencrypted port number.
         */
        MG_UnsignedInt so_port(void);

        /**
         * @return The main SSL port number.
         */
        MG_UnsignedInt so_port_ssl(void);

        /**
         * @return The socket certificate filename, including the path.
         */
        const std::string &so_certificate(void);

        /**
         * @return The socket certificate private key filename, including the
         * path.
         */
        const std::string &so_certificate_private(void);

        /**
         * @return True to enable the unencrypted websocket support.
         */
        bool ws_enable_plain(void);

        /**
         * @return The unencrypted websocket port number.
         */
        MG_UnsignedInt ws_port(void);

        /**
         * @return Maximum window size for a websocket client.
         */
        MG_UnsignedInt ws_max_window(void);

        /**
         * @return Maximum size of a websocket message coming from a client,
         * in bytes.
         */
        MG_UnsignedInt ws_max_incoming_message_size(void);
    }


    /**
     * Config file options related to the database.
     */
    namespace db
    {
        /**
         * @return The database filename, including the path.
         */
        const std::string &db_file(void);

        /**
         * @return The 'work factor' for database password encryption.
         */
        MG_UnsignedInt password_workfactor(void);
    }


    /**
     * Config file options related to AngelScript.
     */
    namespace angelscript
    {
        /**
         * @return Max heap of an AngelScript VM, in kilobytes.
         */
        MG_UnsignedInt max_heap(void);

        /**
         * @return Number of lines to be executed per timeslice.
         */
        MG_UnsignedInt timeslice(void);

        /**
         * @return Maximum number of idle/unused AngelScript engines to leave
         * in the pool.
         */
        MG_UnsignedInt max_pool_size(void);
    }
}
}

#endif //MUTGOS_CONFIG_H
