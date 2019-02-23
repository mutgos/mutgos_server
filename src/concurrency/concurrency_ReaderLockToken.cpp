/*
 * concurrency_ReaderLockToken.cpp
 */


#include "concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#include "logging/log_Logger.h"


namespace mutgos
{
namespace concurrency
{
    bool ReaderLockToken::throw_on_bad_lock = false;

    // ----------------------------------------------------------------------
    ReaderLockToken::~ReaderLockToken()
    {
        unlock();
    }

    // ----------------------------------------------------------------------
    void ReaderLockToken::turn_on_exceptions(void)
    {
        throw_on_bad_lock = true;
    }

    // ----------------------------------------------------------------------
    void ReaderLockToken::unlock(void)
    {
        if (locked)
        {
            if (locked_object_ptr)
            {
                locked = not do_unlock(*locked_object_ptr);
            }
            else
            {
                locked = not do_multiunlock();
            }
        }
    }

    // ----------------------------------------------------------------------
    bool ReaderLockToken::do_lock(
        LockableObject &to_lock)
    {
        const bool is_locked = to_lock.lock_shared();

        if (not is_locked)
        {
            LOG(fatal, "concurrency", "do_lock",
                "Unable to lock!");
        }

        return is_locked;
    }

    // ----------------------------------------------------------------------
    bool ReaderLockToken::do_unlock(
        LockableObject &to_unlock)
    {
        const bool is_unlocked = to_unlock.unlock_shared();

        if (not is_unlocked)
        {
            LOG(fatal, "concurrency", "do_unlock",
                "Unable to unlock!");
        }

        return is_unlocked;
    }

    // ----------------------------------------------------------------------
    bool ReaderLockToken::do_multilock(void)
    {
        bool locked_multi = true;

        for (MultiLock::iterator iter = multi_lock.begin();
             iter != multi_lock.end();
             ++iter)
        {
            if (*iter)
            {
                locked_multi = do_lock(**iter) && locked_multi;
            }

            if (not locked_multi)
            {
                LOG(fatal, "concurrency", "do_multilock",
                    "Unable to fully lock! Unlocking...");

                // Couldn't lock for some reason, so unlock what we've
                // locked so far.
                //
                MultiLock::iterator unlock_iter = iter;

                // Work our way backwards to find what's locked.
                if (unlock_iter != multi_lock.begin())
                {
                    while (unlock_iter != multi_lock.begin())
                    {
                        --unlock_iter;

                        if (not do_unlock(**unlock_iter))
                        {
                            LOG(fatal, "concurrency", "do_multilock",
                                "Unable to unlock after error!");
                        }
                    }
                }

                break;
            }
        }

        return locked_multi;
    }


    // ----------------------------------------------------------------------
    bool ReaderLockToken::do_multiunlock(void)
    {
        for (MultiLock::iterator iter = multi_lock.begin();
                iter != multi_lock.end();
                ++iter)
        {
            if (*iter)
            {
                if (not do_unlock(**iter))
                {
                    LOG(fatal, "concurrency", "do_multiunlock",
                            "Unable to fully unlock!");
                }
            }
        }

        multi_lock.clear();

        return true;
    }
} /* namespace concurrency */
} /* namespace mutgos */
