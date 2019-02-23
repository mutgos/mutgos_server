/*
 * osinterface_ThreadUtils.h
 */

#ifndef MUTGOS_OSINTERFACE_THREADUTILS_H_
#define MUTGOS_OSINTERFACE_THREADUTILS_H_

#include <pthread.h>

namespace mutgos
{
namespace osinterface
{
    /**
     * Handles any OS specific stuff with regards to threads.  All threading
     * stuff needs to be handled through here.
     */
    class ThreadUtils
    {
    public:
        /** Type for the ID of a thread. */
        typedef pthread_t ThreadId;

        /**
         * @return The ID of the thread which called this method.  The ID
         * is opaque and may be printed, compared, or utilized, but not
         * analyzed.
         */
        static ThreadId get_thread_id(void);

        /**
         * @param lhs[in] Left side to compare.
         * @param rhs[in] Right side to compare.
         * @return True if thread identifiers are equal.
         */
        static bool thread_id_equal(ThreadId &lhs, ThreadId &rhs);

        /**
         * Yields the thread (puts in back of execution queue).
         */
        static void yield(void);
    };

} /* namespace osinterface */
} /* namespace mutgos */

#endif /* OSINTERFACE_THREADUTILS_H_ */
