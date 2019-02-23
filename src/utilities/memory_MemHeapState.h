/*
 * memory_MemHeapState.h
 */

#ifndef MUTGOS_MEMORY_MEMHEAPSTATE_H
#define MUTGOS_MEMORY_MEMHEAPSTATE_H

#include <stddef.h>

namespace mutgos
{
namespace memory
{
    /**
     * A data/container class used to hold the current state of the heap for
     * the custom memory allocator.  The allocator's main purpose is to
     * assist softcode interpreters in maintaining a set heap size per process,
     * so one softcode program can't use up all the memory.
     *
     * The max memory allowed is very approximate.
     */
    class MemHeapState
    {
    public:
        //
        // Use default copiers
        //

        /**
         * Constructor. Specify how much memory can be allocated in this heap,
         * or default or 0 for unlimited.
         * @param max[in] The maximum memory that can be allocated in the heap,
         * in bytes.  0 or default for unlimited.
         */
        MemHeapState(const size_t max = 0)
          : mem_in_use(0),
            max_mem(max)
        { }

        /**
         * Destructor.
         */
        ~MemHeapState()
        { }

        /**
         * Called before the heap state is actually used, this can adjust the
         * maximum heap size.
         * @param max[in] The maximum heap size in bytes, or 0 for unlimited.
         */
        inline void set_max_mem(const size_t max)
        { max_mem = max; }

        /**
         * Indicates some memory is being freed.  This does not do the actual
         * freeing, but will decrement the size from the internal heap state.
         * @param size[in] The size of the memory block being freed in bytes.
         * Only free memory that was added via alloc_mem(); it must be the same
         * size (matching) as some alloc_mem() call in the past.
         */
        inline void free_mem(const size_t size)
        {
            // It is possible for this to go negative if a different HeapState
            // was used for allocation (imperfect COTS products integration).
            // Don't let it go below 0 otherwise it will wrap around.
            //
            if (mem_in_use)
            {
                if (size >= mem_in_use)
                {
                    mem_in_use = 0;
                }
                else
                {
                    mem_in_use -= size;
                }
            }
        }

        /**
         * Indicates some memory needs to be allocated; make a check to
         * determine if there is enough space in the heap available and record
         * the allocation if allowed.  This does not do the actual allocation,
         * but will increment the size in the internal heap state.
         * @param size[in] The size of the memory block being allocated in
         * bytes.
         * @param override_max[in] If false (default), an allocation beyond
         * the max will not be recorded, and false will be returned.  If true,
         * the overallocation will be recorded even though false is returned.
         * This is useful for stack or other allocations that are to count
         * towards total allocations but cannot be aborted if they are too big.
         * @return True if there is space left in the heap for the allocation
         * (alloc must proceed), or false if the heap is 'full' and cannot
         * accept any more allocations.  If false, the allocation is NOT
         * recorded here, unless override_max is true.
         */
        inline bool alloc_mem(
            const size_t size,
            const bool override_max = false)
        {
            mem_in_use += size;

            if (max_mem and (mem_in_use > max_mem))
            {
                if (not override_max)
                {
                    mem_in_use -= size;
                }

                return false;
            }

            return true;
        }

        /**
         * @return The amount of memory in this heap in use.
         */
        inline size_t get_mem_in_use(void) const
        { return mem_in_use; }

        /**
         * @return True if there are more allocations than the maximum
         * allowed.
         */
        inline bool is_overallocated(void) const
        {
            if (not max_mem)
            {
                return false;
            }

            return mem_in_use > max_mem;
        }

    private:
        size_t mem_in_use; ///< Memory in this heap currently in use.
        size_t max_mem;  ///< Maximum memory that can be allocated in this heap, or 0 for unlimited.
    };
}
}

#endif //MUTGOS_MEMORY_MEMHEAPSTATE_H
