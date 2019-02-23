/*
 * vheap_td.cpp
 * Tests out basic virtual heap functionality.
 */

#include <string>
#include <iostream>
#include "osinterface/osinterface_ThreadUtils.h"
#include "utilities/memory_ThreadVirtualHeapManager.h"
#include "utilities/memory_VirtualHeapAllocator.h"
#include "utilities/memory_MemHeapState.h"

using namespace mutgos;

typedef std::basic_string<
    char, std::char_traits<char>, memory::VirtualHeapAllocator<char> >
      ManagedString;

int main(void)
{
    std::cout << "size_t bytes: " << sizeof(size_t) << std::endl;

    std::cout << std::endl;
    std::cout << "Running basic memory alloc and free tests..." << std::endl;

    memory::MemHeapState small_state(10240);

    memory::ThreadVirtualHeapManager::add_thread();
    memory::ThreadVirtualHeapManager::set_thread_heap_state(small_state);

    // Allocate some memory, confirming where it starts to deny the request.

    void *mem_A = memory::ThreadVirtualHeapManager::mem_alloc(4096);
    void *mem_B = memory::ThreadVirtualHeapManager::mem_alloc(8);
    void *mem_C = memory::ThreadVirtualHeapManager::mem_alloc(6000);
    void *mem_fail = memory::ThreadVirtualHeapManager::mem_alloc(512);

    if (not (mem_A and mem_B and mem_C and (not mem_fail)))
    {
        std::cerr << "FAILED to allocate expected memory." << std::endl;
        return -1;
    }

    // Get the state to confirm numbers.
    // We should show the allocation of A B C
    // The sizeof calculation at the end is to include overhead internal to
    // ThreadVirtualHeapManager.
    //
    small_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

    if (small_state.get_mem_in_use() != (4096 + 8 + 6000 +
        (sizeof(size_t) * 3)))
    {
        std::cerr << "FAILED to correctly determine memory allocated." << std::endl;
        return -1;
    }

    // Clean up the memory until it's zero.
    //
    memory::ThreadVirtualHeapManager::mem_free(mem_A);
    memory::ThreadVirtualHeapManager::mem_free(mem_B);
    memory::ThreadVirtualHeapManager::mem_free(mem_C);
    mem_A = 0;
    mem_B = 0;
    mem_C = 0;

    // Get the state to confirm zeroed out.
    //
    small_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

    if (small_state.get_mem_in_use())
    {
        std::cerr << "FAILED to correctly count memory freed." << std::endl;
        return -1;
    }

    // Try a quick, basic allocator test.
    //
    std::cout << "Running basic allocator test..." << std::endl;

    {
        ManagedString managed_string;

        managed_string.assign("Hello, test!  This has to be long enough to allocated beyond the stack.");

        small_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

        if (not small_state.get_mem_in_use())
        {
            std::cerr << "FAILED to count memory allocated by allocator." << std::endl;
            return -1;
        }

        std::cout << "Memory allocated for string: "
                  << small_state.get_mem_in_use() << std::endl;
    }

    // Get the state to confirm zeroed out.
    //
    small_state = memory::ThreadVirtualHeapManager::get_thread_heap_state();

    if (small_state.get_mem_in_use())
    {
        std::cerr << "FAILED to correctly count memory freed from allocator." << std::endl;
        return -1;
    }

    // Cleanup.
    memory::ThreadVirtualHeapManager::delete_thread();

    std::cout << "Tests passed." << std::endl;

    return 0;
}
