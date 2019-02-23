/*
 * concurrency_NoLockToken.cpp
 */

#include "concurrency_NoLockToken.h"

namespace mutgos
{
namespace concurrency
{
    // ----------------------------------------------------------------------
    NoLockToken::~NoLockToken()
    {
    }

    // ----------------------------------------------------------------------
    bool NoLockToken::do_lock(LockableObject& to_lock)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    bool NoLockToken::do_unlock(LockableObject& to_unlock)
    {
        return true;
    }

} /* namespace concurrency */
} /* namespace mutgos */
