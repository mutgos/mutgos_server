/*
 * memory_VirtualHeapAllocator.h
 */

#ifndef MUTGOS_MEMORY_VIRTUALHEAPALLOCATOR_H
#define MUTGOS_MEMORY_VIRTUALHEAPALLOCATOR_H

#include <type_traits>
#include <limits>
#include <utility>
#include <exception>
#include <stdexcept>
#include <new>

#include <boost/config.hpp>

#include "memory_ThreadVirtualHeapManager.h"

namespace mutgos
{
namespace memory
{
    /**
     * Allocator that uses the ThreadVirtualHeapManager to get and release
     * memory.
     * Copied and modified from:
     * https://www.codeproject.com/articles/4795/%2FArticles%2F4795%2FC-Standard-Allocator-An-Introduction-and-Implement
     * and the GCC standard allocator.
     * @tparam T The type being allocated.
     */
    template<typename T>
    class VirtualHeapAllocator {
    public:
        //    typedefs
        typedef T value_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        //    convert an allocator<T> to allocator<U>
        template<typename U>
        struct rebind
        {
            typedef VirtualHeapAllocator<U> other;
        };

#if __cplusplus >= 201103L
        // _GLIBCXX_RESOLVE_LIB_DEFECTS
        // 2103. propagate_on_container_move_assignment
        typedef std::true_type propagate_on_container_move_assignment;
#endif

        inline VirtualHeapAllocator() BOOST_NOEXCEPT
        { }

        inline ~VirtualHeapAllocator() BOOST_NOEXCEPT
        { }

        inline VirtualHeapAllocator(VirtualHeapAllocator const&)
            BOOST_NOEXCEPT
        { }

        template<typename U>
        inline VirtualHeapAllocator(VirtualHeapAllocator<U> const&)
            BOOST_NOEXCEPT
        { }


        //
        //    address
        //
        inline pointer address(reference r) const BOOST_NOEXCEPT
        { return std::addressof(r); }

        inline const_pointer address(const_reference r) const BOOST_NOEXCEPT
        { return std::addressof(r); }


        //
        //    memory allocation
        //
        inline pointer allocate(
            size_type cnt,
            typename std::allocator<void>::const_pointer = 0)
        {
            if (cnt > this->max_size())
            {
                throw std::bad_alloc();
            }

            // Use 'virtual' heap version of allocation so it gets recorded.
            //
            const size_type total_size = cnt * sizeof(T);
            void * const new_mem_ptr =
                ThreadVirtualHeapManager::mem_alloc(total_size);

            if (not new_mem_ptr)
            {
                throw std::bad_alloc();
            }

            return reinterpret_cast<pointer>(new_mem_ptr);
        }

        inline void deallocate(pointer p, size_type)
        {
            ThreadVirtualHeapManager::mem_free(p);
        }


        //
        //    size
        //
        inline size_type max_size() const BOOST_NOEXCEPT
        {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }


        //
        //    construction/destruction
        //
#if __cplusplus >= 201103L
        template<typename _Up, typename... _Args>
        void
        construct(_Up* __p, _Args&&... __args)
        { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }

        template<typename _Up>
        void
        destroy(_Up* __p) { __p->~_Up(); }
#else
        inline void construct(pointer p, const T& t)
        { new(p) T(t); }

        inline void destroy(pointer p)
        { p->~T(); }
#endif


        inline bool operator==(VirtualHeapAllocator<T> const&)
        { return true; }

        inline bool operator!=(VirtualHeapAllocator<T> const&)
        { return false; }
    };
}
}

#endif //MUTGOS_MEMORY_VIRTUALHEAPALLOCATOR_H
