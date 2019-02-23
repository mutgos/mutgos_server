/*
 * angelscript_CompiledBytecodeStream.cpp
 */

#include <string>
#include <string.h>
#include <angelscript.h>

#include "logging/log_Logger.h"

#include "angelscript_CompiledBytecodeStream.h"

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    CompiledBytecodeStream::CompiledBytecodeStream(void)
      : written_code_ptr(new std::string()),
        read_code_ptr(0),
        read_code_ptr_end(0),
        read_code_ptr_pos(0),
        read_code_len(0)
    {
    }

    // ----------------------------------------------------------------------
    CompiledBytecodeStream::CompiledBytecodeStream(
        char * const compiled_code_ptr,
        const size_t len)
        : written_code_ptr(0),
          read_code_ptr(compiled_code_ptr),
          read_code_ptr_end(compiled_code_ptr + len),
          read_code_ptr_pos(compiled_code_ptr),
          read_code_len(len)
    {
        if ((not compiled_code_ptr) or (not len))
        {
            LOG(error, "angelscript", "CompiledBytecodeStream(char *, size_t)",
                "null data pointer or zero length!");
        }
    }

    // ----------------------------------------------------------------------
    CompiledBytecodeStream::~CompiledBytecodeStream()
    {
        delete written_code_ptr;
        written_code_ptr = 0;

        delete[] read_code_ptr;
        read_code_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool CompiledBytecodeStream::get_written_bytecode(
        const char *&data_ptr,
        size_t &data_size)
    {
        const bool success = written_code_ptr and
            (not written_code_ptr->empty());

        if (success)
        {
            data_ptr = written_code_ptr->data();
            data_size = written_code_ptr->size();
        }
        else
        {
            data_ptr = 0;
            data_size = 0;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void CompiledBytecodeStream::swap_mode(void)
    {
        if (written_code_ptr)
        {
            // Swapping to read mode
            //
            read_code_ptr = new char[written_code_ptr->size()];
            read_code_ptr_pos = read_code_ptr;
            read_code_len = written_code_ptr->size();
            read_code_ptr_end = read_code_ptr + read_code_len;

            memcpy(
                read_code_ptr,
                written_code_ptr->data(),
                written_code_ptr->size());

            delete written_code_ptr;
            written_code_ptr = 0;
        }
        else if (read_code_ptr)
        {
            // Swapping to write mode
            //
            written_code_ptr = new std::string(read_code_ptr, read_code_len);

            delete read_code_ptr;
            read_code_ptr = 0;
            read_code_ptr_end = 0;
            read_code_ptr_pos = 0;
            read_code_len = 0;
        }
    }

    // ----------------------------------------------------------------------
    bool CompiledBytecodeStream::in_write_mode(void) const
    {
        return written_code_ptr;
    }

    // ----------------------------------------------------------------------
    void CompiledBytecodeStream::reset_read(void)
    {
        if (read_code_ptr)
        {
            read_code_ptr_pos = read_code_ptr;
        }
    }

    // ----------------------------------------------------------------------
    int CompiledBytecodeStream::Read(void *ptr, asUINT size)
    {
        int status = -1;

        if (not ptr)
        {
            LOG(error, "angelscript", "Read", "ptr is null!");
            return status;
        }

        if (read_code_ptr)
        {
            if ((read_code_ptr_pos < read_code_ptr_end) and
                (read_code_ptr_pos + size) <= read_code_ptr_end)
            {
                memcpy(ptr, read_code_ptr_pos, size);
                read_code_ptr_pos += size;
                status = 0;
            }
            else
            {
                // Out of bounds
                //
                LOG(error, "angelscript", "Read",
                    "Out of bounds!  Corrupt stream?");
            }
        }

        return status;
    }

    // ----------------------------------------------------------------------
    int CompiledBytecodeStream::Write(const void *ptr, asUINT size)
    {
        int status = -1;

        if (written_code_ptr)
        {
            const char * const char_ptr = (const char *) ptr;

            if (char_ptr and size)
            {
                written_code_ptr->append(char_ptr, size);
            }

            status = 0;
        }

        return status;
    }
}
}
