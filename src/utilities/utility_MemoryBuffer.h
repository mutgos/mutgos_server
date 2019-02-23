/*
 * MemoryBuffer.h
 */

#ifndef MUTGOS_UTILITY_MEMORYBUFFER_H
#define MUTGOS_UTILITY_MEMORYBUFFER_H

#include <stddef.h>
#include <streambuf>
#include <iostream>
#include <cstdio>

namespace mutgos
{
namespace utility
{
    /**
     * Memory-only streambuf, used primarily for serialization/deserialization.
     * http://stackoverflow.com/a/13586356
     * http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
     *
     * This class has little error checking and is currently meant for
     * serialization only.  In the future it could be improved.
     */
    class MemoryBuffer : public std::streambuf
    {
    public:
        /**
         * Constructs a memory buffer for reading, using the
         * provided data as the source.  The data WILL have its const
         * removed, but will not be modified.  It will never be deleted.
         * @param data_ptr[in] Pointer to the data to read.
         * @param data_size[in] The size of the data in bytes.
         */
        MemoryBuffer(const void *data_ptr, const size_t data_size);

        /**
         * Constructs a memory buffer for writing.
         */
        MemoryBuffer(void);

        /**
         * Destructor to clean up write buffer.
         */
        virtual ~MemoryBuffer();

        /**
         * Gets the data present on a write-only buffer.
         * @param data_ptr
         * @param data_size
         * @return True if pointer and size populated, false if not or if
         * this is a read-only buffer.
         */
        bool get_data(char *&data_ptr, size_t &data_size);

    protected:
        /**
         * Doubles the size of the buffer to accept more characters, and
         * appends the character to the buffer.
         * @see std::streambuf
         */
        virtual int_type overflow(int_type c  = traits_type::eof());

    private:
        const bool read_only; ///< True if this is a buffer for reading.

        // No copying
        //
        MemoryBuffer(const MemoryBuffer &rhs);
        MemoryBuffer &operator=(const MemoryBuffer &rhs);
    };
} // utilities
} // mutgos

#endif //MUTGOS_UTILITY_MEMORYBUFFER_H
