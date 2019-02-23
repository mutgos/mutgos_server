/*
 * concurrency_WriterLockToken.h
 */

#ifndef MUTGOS_CONCURRENCY_WRITERLOCKTOKEN_H_
#define MUTGOS_CONCURRENCY_WRITERLOCKTOKEN_H_

#include "concurrency/concurrency_ReaderLockToken.h"

namespace mutgos
{
namespace concurrency
{
    /**
     * This class represents a lock 'token' that can be used to keep an
     * object locked between read/write calls to it.  This allows for
     * atomic operations that require multiple calls to said object.  It cannot
     * be copied and should not be given to anyone else after construction.
     * When destructed, it will automatically release the lock.
     * Do not hold onto it for any reason - simply make a new one the next
     * time it's needed.
     *
     * When the intention is to lock several objects for a transaction or
     * other grouping, use the MultiLock version of the constructor so they
     * are all locked at once.  Failure to do this will cause a deadlock
     * as this is a multithreaded application.
     */
    class WriterLockToken : public ReaderLockToken
    {
    public:
        /**
         * Constructs an exclusive (reader/writer) lock token.
         * Do not use this with the intention of separately locking objects
         * that will be used together.  Use the MultiLock constructor version
         * instead.
         * Will block until lock is acquired.
         * @param object[in] The object being locked.
         */
        explicit WriterLockToken(LockableObject &object)
          : ReaderLockToken(object, false)
        {
            locked = do_lock(*locked_object_ptr);
        }

        /**
         * Constructs an exclusive (reader/writer) lock token for several
         * objects at once.  This is used primarily for transactions.
         * Will block until all locks are acquired.
         * @param objects[in] The objects being locked.
         */
        explicit WriterLockToken(const MultiLock &objects)
          : ReaderLockToken(objects, false)
        {
            locked = do_multilock();
        }

        /**
         * The destructor unlocks the object.
         */
        virtual ~WriterLockToken();

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

    private:
        // No copying allowed
        //
        WriterLockToken(const WriterLockToken &rhs);
        WriterLockToken &operator=(const WriterLockToken &rhs);
    };

} /* namespace concurrency */
} /* namespace mutgos */

#endif /* CONCURRENCY_WRITERLOCKTOKEN_H_ */
