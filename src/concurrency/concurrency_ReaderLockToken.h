/*
 * ReaderLockToken.h
 */

#ifndef MUTGOS_CONCURRENCY_READERLOCKTOKEN_H_
#define MUTGOS_CONCURRENCY_READERLOCKTOKEN_H_

#include <set>

#include "concurrency/concurrency_LockableObject.h"

#include "concurrency/concurrency_LockException.h"

#include <boost/throw_exception.hpp>

namespace mutgos
{
namespace concurrency
{
    /**
     * This class represents a lock 'token' that can be used to keep an
     * object locked between read-only (const) calls to it.  This allows for
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
    class ReaderLockToken
    {
    public:
        /** Type used to organize locked objects.  The ordering inherit in a
            set is important here, so insure they always lock in the same
            order */
        typedef std::set<LockableObject *> MultiLock;

        /**
         * Constructs a shared (reader) lock token.
         * Do not use this with the intention of separately locking objects
         * that will be used together.  Use the MultiLock constructor version
         * instead.
         * Will block until lock is acquired.
         * @param object[in] The object being locked.
         */
        explicit ReaderLockToken(LockableObject &object)
          : locked_object_ptr(&object),
            locked(false)
        {
            locked = do_lock(*locked_object_ptr);
        }

        /**
         * Constructs a shared (reader) lock token for several objects at once.
         * This is used primarily for transactions.
         * Will block until all locks are acquired.
         * @param objects[in] The objects being locked.
         */
        explicit ReaderLockToken(const MultiLock &objects)
          : locked_object_ptr(0),
            locked(false),
            multi_lock(objects)
        {
            locked = do_multilock();
        }

        /**
         * The destructor unlocks the object.
         */
        virtual ~ReaderLockToken();

        /**
         * Enables throwing exceptions when a lock problem occurs.
         */
        static void turn_on_exceptions(void);

        /**
         * @param object[in] The object to check.
         * @return True if this has locked the provided object.
         */
        bool has_lock(LockableObject &object) const
        {
            const bool result =
                    locked and
                      (locked_object_ptr ?
                      (&object == locked_object_ptr) :
                      (multi_lock.find(&object) != multi_lock.end()));

            if ((not result) and throw_on_bad_lock)
            {
                BOOST_THROW_EXCEPTION(LockException());
            }

            return result;
        }

        /**
         * @return True if this lock is indeed locked to something.
         */
        bool is_locked(void) const
        {
            return locked;
        }

    protected:

        /**
         * Constructs a shared (reader) lock token.  Used by subclasses
         * to not perform the lock here, but instead perform it using their
         * virtual methods.
         * Do not use this with the intention of separately locking objects
         * that will be used together.  Use the MultiLock constructor version
         * instead.
         * Will block until lock is acquired.
         * @param object[in] The object being locked.
         * @param perform_lock[in] If true, do a reader lock.  If false,
         * assume the subclass did the lock.
         */
        explicit ReaderLockToken(
            LockableObject &object,
            const bool perform_lock)
            : locked_object_ptr(&object),
              locked(false)
        {
            if (perform_lock)
            {
                locked = do_lock(*locked_object_ptr);
            }
        }

        /**
         * Constructs a shared (reader) lock token for several objects at once.
         * Used by subclasses to not perform the lock here, but instead perform
         * it using their virtual methods.
         * This is used primarily for transactions.
         * Will block until all locks are acquired.
         * @param objects[in] The objects being locked.
         * @param perform_lock[in] If true, do a reader multilock.  If false,
         * assume the subclass did the multilock.
         */
        explicit ReaderLockToken(
            const MultiLock &objects,
            const bool perform_lock)
            : locked_object_ptr(0),
              locked(false),
              multi_lock(objects)
        {
            if (perform_lock)
            {
                locked = do_multilock();
            }
        }

        /**
         * Unlocks everything.  Must be called by all destructors, even of
         * subclasses since a destructor in the base class can't call virtual
         * methods of a subclass.
         */
        void unlock(void);

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

        /**
         * Locks all the LockableObjects in multi_lock.
         * @return True if success.
         */
        bool do_multilock(void);

        /**
         * Unlocks all the LockableObjects in multi_lock.
         * @return True if success.
         */
        bool do_multiunlock(void);

        LockableObject *locked_object_ptr; ///< The object being locked by this.
        bool locked; ///< If true, successfully locked.

        MultiLock multi_lock;

    private:
        // No copying allowed
        //
        ReaderLockToken(const ReaderLockToken &rhs);
        ReaderLockToken &operator=(const ReaderLockToken &rhs);

        static bool throw_on_bad_lock; ///< If true throw exception on bad lock
    };

} /* namespace concurrency */
} /* namespace mutgos */

#endif /* READERLOCKTOKEN_H_ */
