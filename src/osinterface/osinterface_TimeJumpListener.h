/*
 * osinterface_TimeJumpListener.h
 */

#ifndef MUTGOS_OSINTERFACE_TIMEJUMPLISTENER_H
#define MUTGOS_OSINTERFACE_TIMEJUMPLISTENER_H

namespace mutgos
{
namespace osinterface
{
    /**
     * An interface that notifies the listener when an OS-level 'time jump'
     * has occurred.  This can happen because the user manually set the
     * system clock backwards or forwards a significant amount, or perhaps
     * NTP misconfiguration.
     *
     * Since some Boost calls require absolute time, they may not return from
     * their timed wait when expected if time were to go backwards a
     * significant amount.
     */
    class TimeJumpListener
    {
    public:
        /**
         * Default instructor.
         */
        TimeJumpListener(void)
          { }

        /**
         * Destructor.
         */
        virtual ~TimeJumpListener()
          { }

        /**
         * Called when a massive (more than a few seconds) system time jump has
         * been detected.
         * @param backwards[in] True if the jump was backwards.
         */
        virtual void os_time_has_jumped(bool backwards) =0;
    };
}
}

#endif //MUTGOS_OSINTERFACE_TIMEJUMPLISTENER_H
