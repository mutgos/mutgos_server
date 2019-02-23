/*
 * osinterface_Signals.cpp
 */

#include <string>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "osinterface_Signals.h"

namespace
{
    static bool quit_signal_flag = false;
    const std::string SHUTDOWN_FILE = "mutgos_shutdown";
}

// ----------------------------------------------------------------------
static void quit_signal_callback(int sig)
{
    quit_signal_flag = true;

    std::cout << "MUTGOS: Got shutdown signal " << sig << std::endl;
}

namespace mutgos
{
namespace osinterface
{
    // ----------------------------------------------------------------------
    void Signals::register_quit(void)
    {
        struct sigaction action;
        struct sigaction old_action;

        action.sa_handler = quit_signal_callback;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGINT, &action, &old_action);
        sigaction(SIGQUIT, &action, &old_action);

        // Support file based quitting too; delete the file if it exists
        // so we don't trigger ourselves immediately.
        unlink(SHUTDOWN_FILE.c_str());
    }

    // ----------------------------------------------------------------------
    bool Signals::got_quit_signal(void)
    {
        struct stat stat_buffer;

        if (stat(SHUTDOWN_FILE.c_str(), &stat_buffer) == 0)
        {
            return true;
        }

        return quit_signal_flag;
    }
}
}
