/*
 * concurrency_NoLockToken.h
 */

#ifndef MUTGOS_CONCURRENCY_NOLOCKTOKEN_H_
#define MUTGOS_CONCURRENCY_NOLOCKTOKEN_H_

#include "concurrency/concurrency_WriterLockToken.h"

namespace mutgos
{
namespace concurrency
{
    /**
     * Used only when restoring an Entity, as it doesn't actually perform
     * any locking, but it used to speed up batch processing slightly.
     */
    class NoLockToken : public WriterLockToken
    {
    public:
        /**
         * Constructs a restore lock token.
         * Do not use this with the intention of separately locking objects
         * that will be used together.  Use the MultiLock constructor version
         * instead.
         * Will block until lock is acquired.
         * @param object[in] The object being locked.
         */
        explicit NoLockToken(LockableObject &object)
          : WriterLockToken(object)
        {
        }

        /**
         * Constructs a restore lock token for several
         * objects at once.  This is used primarily for transactions.
         * Will block until all locks are acquired.
         * @param objects[in] The objects being locked.
         */
        explicit NoLockToken(const MultiLock &objects)
          : WriterLockToken(objects)
        {
        }

        /**
         * The destructor unlocks the object.
         */
        virtual ~NoLockToken();

    protected:
        /**
         * Used by subclasses to perform the actual locking operation.
         * @param to_lock[in] Object to lock against.
         * @return True if success.
         */
        virtual bool do_lock(LockableObject &to_lock);

        /**
         * Used by subclasses to perform the actual unlocking operation.
         * @param to_unlock[in] Object to unlock against.
         * @return True if success.
         */
        virtual bool do_unlock(LockableObject &to_unlock);
    };

} /* namespace concurrency */
} /* namespace mutgos */

#endif /* CONCURRENCY_NOLOCKTOKEN_H_ */
