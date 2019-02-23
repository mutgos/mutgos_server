/*
 * osinterface_ThreadUtils.cpp
 */

#include "osinterface_ThreadUtils.h"

#include <pthread.h>

namespace mutgos
{
namespace osinterface
{
    // -----------------------------------------------------------------------
    ThreadUtils::ThreadId ThreadUtils::get_thread_id(void)
    {
        return pthread_self();
    }

    // -----------------------------------------------------------------------
    bool ThreadUtils::thread_id_equal(
        ThreadUtils::ThreadId &lhs,
        ThreadUtils::ThreadId &rhs)
    {
        return pthread_equal(lhs, rhs) != 0;
    }

    // -----------------------------------------------------------------------
    void ThreadUtils::yield(void)
    {
        pthread_yield();
    }
} /* namespace osinterface */
} /* namespace mutgos */
