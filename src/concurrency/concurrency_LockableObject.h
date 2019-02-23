/*
 * concurrency_LockableObject.h
 */

#ifndef MUTGOS_CONCURRENCY_LOCKABLEOBJECT_H_
#define MUTGOS_CONCURRENCY_LOCKABLEOBJECT_H_

namespace mutgos
{
namespace concurrency
{
    /**
     * Provides an interfaces a class can implement to support locking via
     * the concurrency namespace classes.
     */
    class LockableObject
    {
    public:

        /**
         * Default constructor.
         */
        LockableObject(void);

        /**
         * Locks this object for exclusive (read/write) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock(void) =0;

        /**
         * Attempts to lock this object for exclusive (read/write) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock(void) =0;

        /**
         * Attempts to lock this object for shared (read only) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock_shared(void) =0;

        /**
         * Locks this object for shared (read only) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock_shared(void) =0;

        /**
         * Unlocks this object from an exclusive lock.  Only call if lock()
         * was used!
         * @return True if success.
         */
        virtual bool unlock(void) =0;

        /**
         * Unlocks this object from a shared lock.  Only call if lock_shared()
         * was used!
         * @return True if success.
         */
        virtual bool unlock_shared(void) =0;

        /**
         * Required virtual destructor.
         */
        virtual ~LockableObject();
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* CONCURRENCY_LOCKABLEOBJECT_H_ */
