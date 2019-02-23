/*
 * MemoryBuffer.cpp
 */

#include "utility_MemoryBuffer.h"

#include <string.h>
#include <stddef.h>
#include <streambuf>

#define INITIAL_BUFFER_SIZE_BYTES 4096

namespace mutgos
{
namespace utility
{
    // ----------------------------------------------------------------------
    MemoryBuffer::MemoryBuffer(const void *data_ptr, const size_t data_size)
      : read_only(true)
    {
        // Yes, I am undoing the const declaration, but the data
        // will not be modified.  It is a requirement of streambuf.
        setg((char *)data_ptr, (char *)data_ptr, (char *)data_ptr + data_size);
    }

    // ----------------------------------------------------------------------
    MemoryBuffer::MemoryBuffer(void)
      : read_only(false)
    {
        char *buffer_ptr = new char[INITIAL_BUFFER_SIZE_BYTES];
        setp(buffer_ptr, buffer_ptr + INITIAL_BUFFER_SIZE_BYTES);
    }

    // ----------------------------------------------------------------------
    MemoryBuffer::~MemoryBuffer()
    {
        if (not read_only)
        {
            delete[] pbase();
            setp(0, 0);
        }
    }

    // ----------------------------------------------------------------------
    bool MemoryBuffer::get_data(char *&data_ptr, size_t &data_size)
    {
        if (not read_only)
        {
            data_ptr = pbase();
            data_size = pptr() - pbase();

            return data_ptr;
        }

        return false;
    }

    // ----------------------------------------------------------------------
    MemoryBuffer::int_type MemoryBuffer::overflow(MemoryBuffer::int_type c)
    {
        char *data_ptr = 0;
        size_t data_size = 0;

        if ((not read_only) and get_data(data_ptr, data_size))
        {
            const size_t bigger_size = data_size * 2;
            char *bigger_data_ptr = new char[bigger_size];

            memcpy(bigger_data_ptr, data_ptr, data_size);

            setp(bigger_data_ptr, bigger_data_ptr + bigger_size);
            pbump(data_size);

            delete[] data_ptr;
            data_ptr = 0;

            if (c != traits_type::eof())
            {
                // Append character if not EOF
                //
                *pptr() = (char) c;
                pbump(1);
            }

            return c;
        }
        else
        {
            // Not a write buffer or can't get the existing pointer.
            return traits_type::eof();
        }
    }
} // utilities
} // mutgos
