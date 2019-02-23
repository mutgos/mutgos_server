/*
 * osinterface_Signals.h
 */

#ifndef MUTGOS_OSINTERFACE_SIGNALS_H
#define MUTGOS_OSINTERFACE_SIGNALS_H

namespace mutgos
{
namespace osinterface
{
    /**
     * A simple class to register and detect when certain signals have been
     * received.  It will need to be improved later.
     */
    class Signals
    {
    public:
        /**
         * Called to register the signal handler for a QUIT or related
         * signal.  Only call this once.
         */
        static void register_quit(void);

        /**
         * @return True if the QUIT or related signal(s) have ever been
         * received.
         */
        static bool got_quit_signal(void);
    };
}
}

#endif //MUTGOS_OSINTERFACE_SIGNALS_H
