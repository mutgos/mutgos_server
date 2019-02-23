/*
 * memory_ThreadVirtualHeapManager.cpp
 */

#include <stdlib.h>
#include <exception>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/thread.hpp>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"
#include "osinterface/osinterface_ThreadUtils.h"

#include "memory_MemHeapState.h"
#include "memory_ThreadVirtualHeapManager.h"

namespace
{
    mutgos::memory::MemHeapState INVALID_HEAP_STATE(0);
}

namespace mutgos
{
namespace memory
{
    // Statics
    //
    ThreadVirtualHeapManager::ThreadHeapStates
        ThreadVirtualHeapManager::thread_heap_states;
    boost::mutex ThreadVirtualHeapManager::mutex;

    /*
     * Note that for the malloc and free calls, NO locking is performed for
     * performance reasons.  This is because the data to be modified is assumed
     * to be in a fixed location and will not be changed by any other thread
     * until completion.
     */

    // ----------------------------------------------------------------------
    // Refer to https://stackoverflow.com/a/35326444 for info on using
    // 'tags' to store alloc info with each block.
    // This is based on that answer.
    void *ThreadVirtualHeapManager::mem_alloc(const size_t size)
    {
        size_t *alloc_block_ptr = 0;

        if (not size)
        {
            return alloc_block_ptr;
        }
        else
        {
            const size_t actual_malloc_size = size + sizeof(size_t);

            if (not external_malloc(actual_malloc_size))
            {
                return alloc_block_ptr;
            }
            else
            {
                alloc_block_ptr = reinterpret_cast<size_t *>(
                    malloc(actual_malloc_size));

                if (not alloc_block_ptr)
                {
                    // This should never happen unless we run out of system
                    // memory.
                    external_free(actual_malloc_size);

                    LOG(fatal, "memory", "malloc",
                        "Failed to malloc "
                        + text::to_string(actual_malloc_size) + " bytes!");
                    return alloc_block_ptr;
                }
                else
                {
                    // Insert heap size data prior to the actual pointer
                    // returned to the user.
                    *alloc_block_ptr = size;
                }
            }
        }

        // Return area after our tag.
        return &alloc_block_ptr[1];
    }

    // ----------------------------------------------------------------------
    // Refer to https://stackoverflow.com/a/35326444 for info on using
    // 'tags' to store alloc info with each block.
    // This is based on that answer.
    void *ThreadVirtualHeapManager::mem_alloc_nofail(const size_t size)
    {
        size_t *alloc_block_ptr = 0;

        if (not size)
        {
            return alloc_block_ptr;
        }
        else
        {
            const size_t actual_malloc_size = size + sizeof(size_t);

            external_malloc(actual_malloc_size, true);

            alloc_block_ptr = reinterpret_cast<size_t *>(
                malloc(actual_malloc_size));

            if (not alloc_block_ptr)
            {
                // This should never happen unless we run out of system
                // memory.
                LOG(fatal, "memory", "malloc",
                    "Failed to malloc "
                    + text::to_string(actual_malloc_size) + " bytes!");

                external_free(actual_malloc_size);

                return alloc_block_ptr;
            }
            else
            {
                // Insert heap size data prior to the actual pointer
                // returned to the user.
                *alloc_block_ptr = size;
            }
        }

        // Return area after our tag.
        return &alloc_block_ptr[1];
    }

    // ----------------------------------------------------------------------
    // Refer to https://stackoverflow.com/a/35326444 for info on using
    // 'tags' to store alloc info with each block.
    // This is based on that answer.
    void ThreadVirtualHeapManager::mem_free(void *ptr)
    {
        if (ptr)
        {
            size_t * const alloc_block_ptr =
                reinterpret_cast<size_t *>(ptr) - 1;

            external_free(*alloc_block_ptr + sizeof(size_t));
            free(reinterpret_cast<void *>(alloc_block_ptr));
        }
    }

    // ----------------------------------------------------------------------
    bool ThreadVirtualHeapManager::external_malloc(
        const size_t size,
        const bool override_max)
    {
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
             thread_iter != thread_heap_states.end();
             ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread, try and do the external malloc.
                return thread_iter->second.alloc_mem(size, override_max);
            }
        }

        LOG(fatal, "memory", "external_malloc",
            "Unable to find thread heap state!");

        return false;
    }

    // ----------------------------------------------------------------------
    void ThreadVirtualHeapManager::external_free(const size_t size)
    {
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
             thread_iter != thread_heap_states.end();
             ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread, try and do the external free.
                thread_iter->second.free_mem(size);
                return;
            }
        }

        LOG(fatal, "memory", "external_free",
            "Unable to find thread heap state!");
    }

    // ----------------------------------------------------------------------
    void ThreadVirtualHeapManager::add_thread(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
             thread_iter != thread_heap_states.end();
             ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread, so we don't need to do add it again.
                return;
            }
        }

        thread_heap_states.push_back(
            std::make_pair(current_thread, MemHeapState(0)));
    }

    // ----------------------------------------------------------------------
    void ThreadVirtualHeapManager::delete_thread(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
             thread_iter != thread_heap_states.end();
             ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread.  Delete it.
                //
                thread_heap_states.erase(thread_iter);
                break;
            }
        }
    }

    // ----------------------------------------------------------------------
    bool ThreadVirtualHeapManager::check_overallocation(
        const bool throw_if_over)
    {
        if (get_thread_heap_state().is_overallocated())
        {
            if (throw_if_over)
            {
                throw std::bad_alloc();
            }

            return true;
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ThreadVirtualHeapManager::set_thread_heap_state(
        const MemHeapState &heap_state)
    {
        bool found_thread = false;
        boost::lock_guard<boost::mutex> guard(mutex);
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
                thread_iter != thread_heap_states.end();
            ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread.  Copy the state in.
                //
                thread_iter->second = heap_state;

                found_thread = true;
                break;
            }
        }

        return found_thread;
    }

    // ----------------------------------------------------------------------
    const MemHeapState &ThreadVirtualHeapManager::get_thread_heap_state(void)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        osinterface::ThreadUtils::ThreadId current_thread =
            osinterface::ThreadUtils::get_thread_id();

        for (ThreadHeapStates::iterator thread_iter = thread_heap_states.begin();
             thread_iter != thread_heap_states.end();
             ++thread_iter)
        {
            if (osinterface::ThreadUtils::thread_id_equal(
                thread_iter->first,
                current_thread))
            {
                // Found our thread.  Return the state.
                return thread_iter->second;
            }
        }

        LOG(fatal, "memory", "get_thread_heap_state",
            "Unable to find thread heap state!");

        return INVALID_HEAP_STATE;
    }
}
}
