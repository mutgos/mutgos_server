/*
 * angelscript_CompiledBytecodeStream.h
 */

#ifndef MUTGOS_ANGELSCRIPT_COMPILEDBYTECODESTREAM_H
#define MUTGOS_ANGELSCRIPT_COMPILEDBYTECODESTREAM_H

#include <string>
#include <angelscript.h>

namespace mutgos
{
namespace angelscript
{
    /**
     * Implements required interface to read and write compiled AngelScript
     * bytecode.  This is not thread safe.
     */
    class CompiledBytecodeStream : public asIBinaryStream
    {
    public:
        /**
         * Constructs the class in write mode.
         */
        CompiledBytecodeStream(void);

        /**
         * Constructs the class in read mode.
         * @param compiled_code_ptr[in] Pointer to the compiled bytecode.
         * Ownership of the pointer transfers to this class.
         * @param len[in] Size of compiled_code.
         */
        CompiledBytecodeStream(char * const compiled_code_ptr, const size_t len);

        /**
         * Required virtual destructor.
         */
        virtual ~CompiledBytecodeStream();

        /**
         * Gets the bytecode written, in a format suitable for dbtype::Program.
         * @param data_ptr[out] A pointer to the data written.  This is not
         * a copy, and will become invalid when this class instance is
         * destructed.
         * @param data_size[out] The size of data_ptr.
         * @return True if data_ptr and data_size are populated, false if not
         * (not in write mode, empty buffer, etc).
         */
        bool get_written_bytecode(const char *&data_ptr, size_t &data_size);

        /**
         * If this instance is in Read mode, copy data around to put it in
         * Write mode, and vice versa.  Considered expensive.
         */
        void swap_mode(void);

        /**
         * @return True if instance is in write mode (Write() method can be
         * used), false if in read mode (Read() method can be used).
         */
        bool in_write_mode(void) const;

        /**
         * Resets the read position to the beginning.
         */
        void reset_read(void);

        /**
         * Reads a chunk of data from provided compiled code.
         * @param ptr[out] Where to copy the data to.
         * @param size[in] How much data to read and place in ptr.
         * @return A negative value on error, otherwise 0.
         */
        virtual int Read(void *ptr, asUINT size);

        /**
         * Writes a chunk of compiled data.
         * @param ptr[in] The data to copy.
         * @param size[in] How much data to copy from ptr.
         * @return A negative value on error, otherwise 0.
         */
        virtual int Write(const void *ptr, asUINT size);

    private:
        std::string *written_code_ptr; ///< When in write mode, the written out bytecode goes here.  Null in read mode.

        char *read_code_ptr; ///<  When in read mode, the data is read from here.  Null in write mode.
        char *read_code_ptr_end; ///<  When in read mode, This points to the next location AFTER the end of read_code_ptr's block.
        char *read_code_ptr_pos; ///< Current reading position inside read_code_ptr.
        size_t read_code_len; ///< Size of block allocated to read_code_ptr.

        // No copying.
        CompiledBytecodeStream(const CompiledBytecodeStream &rhs);
        CompiledBytecodeStream &operator=(const CompiledBytecodeStream &rhs);
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_COMPILEDBYTECODESTREAM_H
