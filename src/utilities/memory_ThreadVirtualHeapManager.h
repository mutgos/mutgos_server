/*
 * memory_ThreadVirtualHeapManager.h
 */

#ifndef MUTGOS_MEMORY_VIRTUALHEAPMANAGER_H
#define MUTGOS_MEMORY_VIRTUALHEAPMANAGER_H

#include <map>
#include <vector>
#include <stddef.h>

#include <boost/thread/mutex.hpp>

#include "osinterface/osinterface_ThreadUtils.h"
#include "memory_MemHeapState.h"

namespace mutgos
{
namespace memory
{
    /**
     * A 'virtual' heap manager, designed to manage a size-constrained heap on
     * a thread.  Its primary use is to limit the heap sizes of softcode
     * interpreter VMs.  The malloc and free functions can be passed to those
     * interpreters and used in place of the usual malloc and free.  A
     * C++ STL style allocator is also provided in another class.
     *
     * Before a VM that uses this manager activates on a thread, it must set
     * the current heap state using the methods below.  Due to performance
     * reasons, very little error checking is done.
     */
    class ThreadVirtualHeapManager
    {
    public:
        /**
         * Works like malloc() except it will check for overruns of the
         * 'virtual' heap.  set_thread_heap_state() must have been called
         * for the thread before this is called.
         * @param size[in] The size of the memory block to alloc.
         * @return The alloc'ed block, or null if error or it exceeded
         * the size of the 'virtual' heap.  The pointer returned, when freed,
         * MUST USE mem_free().
         */
        static void *mem_alloc(const size_t size);

        /**
         * Works like mem_alloc(), except it will track allocations but not
         * return a null when the maximum 'virtual' heap size has been
         * exceeded.  This is useful for COTS products that accept a
         * user-defined malloc-type function but can't actually handle running
         * out of memory at the point of allocation.
         * set_thread_heap_state() must have been called for the thread before
         * this is called.
         * @param size[in] The size of the memory block to alloc.
         * @return The alloc'ed block, or null if error or it exceeded
         * the size of the 'virtual' heap.  The pointer returned, when freed,
         * MUST USE mem_free().
         */
        static void *mem_alloc_nofail(const size_t size);

        /**
         * Works like free() except it also decrements the space used on the
         * 'virtual' heap.
         * @param ptr[in] The block of memory to free.  MUST have been
         * alloc'ed with mem_alloc() / mem_alloc_nofail() or a crash may result.
         */
        static void mem_free(void *ptr);

        /**
         * Used by programs that do their own malloc and free calls, but
         * need them to be tracked in the heap state.  This call assumes
         * the malloc is about to be performed but hasn't occurred yet.
         * @param size[in] The size of the memory block to be malloced and
         * tracked.
         * @param override_max[in] If false (default), an allocation beyond
         * the max will not be recorded, and false will be returned.  If true,
         * the overallocation will be recorded even though false is returned.
         * This is useful for stack or other allocations that are to count
         * towards total allocations but cannot be aborted if they are too big.
         * @return True if the malloc will be within the desired heap size
         * (malloc must proceed), or false if it would exceed the size (malloc
         * must not proceed).  In the event of an error, false will be
         * returned.  If false, the allocation is NOT recorded here, unless
         * override_max is true.
         */
        static bool external_malloc(
            const size_t size,
            const bool override_max = false);

        /**
         * Used by programs that do their own malloc and free calls, but
         * need them to be tracked in the heap state.  This call assumes
         * the free call has or is about to occur.
         * @param size[in] The size of the memory block to be freed and tracked.
         */
        static void external_free(const size_t size);

        /**
         * Adds the currently executing thread to the heap manager, generally
         * called because the thread has started up.
         * NOTE: Do not call this if other threads are currently executing
         * (using malloc() and free()).
         */
        static void add_thread(void);

        /**
         * Removes the currently executing thread from the heap manager,
         * generally called because the thread is shutting down.
         * NOTE: Do not call this if other threads are currently executing
         * (using malloc() and free()).
         */
        static void delete_thread(void);

        /**
         * Checks to see if we are currently overallocated for this thread's
         * 'virtual' heap.
         * set_thread_heap_state() must have been called for the thread before
         * this is called.
         * @param throw_if_over[in] If true, an std::bad_alloc will be thrown
         * if overallocated.
         * @return True if overallocated, false if not.
         */
        static bool check_overallocation(const bool throw_if_over);

        /**
         * Sets the heap state for the currently executing thread, so
         * malloc() and free() will make use of it.  Call this before
         * the thread starts executing with this heap manager.
         * @param heap_state[in] The heap state to set.
         * @return True if thread found and the heap state set, false
         * otherwise.  If false is returned, DO NOT USE malloc() and free(),
         */
        static bool set_thread_heap_state(const MemHeapState &heap_state);

        /**
         * Call this after the thread is finished executing with this heap
         * manager.
         * @return The heap state for the currently executing thread, or
         * a default (all zeros) if not found.
         */
        static const MemHeapState &get_thread_heap_state(void);

    private:

        typedef size_t AllocBlockSize;
        typedef std::pair<osinterface::ThreadUtils::ThreadId, MemHeapState>
            ThreadToHeapState;
        typedef std::vector<ThreadToHeapState> ThreadHeapStates;

        static ThreadHeapStates thread_heap_states; ///< Current virtual heap states, organized by running thread
        static boost::mutex mutex; ///< Enforces single access at a time when making major changes

        // Static class only; disable constructor/destructor.
        //
        ThreadVirtualHeapManager(void);
        ~ThreadVirtualHeapManager();
    };
}
}

#endif //MUTGOS_MEMORY_VIRTUALHEAPMANAGER_H
