/*
 * concurrency_WriterLockToken.cpp
 */

#include "concurrency/concurrency_WriterLockToken.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace concurrency
{
    // ----------------------------------------------------------------------
    WriterLockToken::~WriterLockToken()
    {
        unlock();
    }

    // ----------------------------------------------------------------------
    bool WriterLockToken::do_lock(LockableObject &to_lock)
    {
        const bool is_locked = to_lock.lock();

        if (not is_locked)
        {
            LOG(fatal, "concurrency", "do_lock",
                "Unable to lock!");
        }

        return is_locked;
    }

    // ----------------------------------------------------------------------
    bool WriterLockToken::do_unlock(LockableObject &to_unlock)
    {
        const bool is_unlocked = to_unlock.unlock();

        if (not is_unlocked)
        {
            LOG(fatal, "concurrency", "do_unlock",
                "Unable to unlock!");
        }

        return is_unlocked;
    }
} /* namespace concurrency */
} /* namespace mutgos */
