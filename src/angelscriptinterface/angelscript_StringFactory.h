/*
 * angelscript_StringFactory.h
 */

#ifndef MUTGOS_ANGELSCRIPT_STRINGFACTORY_H
#define MUTGOS_ANGELSCRIPT_STRINGFACTORY_H

#include <angelscript.h>

#include "angelscript_AString.h"

namespace mutgos
{
namespace angelscript
{
    /**
     * Used by AngelScript to create AString instances of constant string
     * values.
     * TODO Add caching support.
     */
    class StringFactory : public asIStringFactory
    {
    public:
        /**
         * Creates a factory for a specific Engine.
         * @param engine[in] The AngelScript Engine this factory is for.
         */
        StringFactory(asIScriptEngine *engine);

        /**
         * Required virtual destructor.
         */
        virtual ~StringFactory();

        /**
         * From the AngelScript documentation:
         * The string factory can cache and return a pointer to the same
         * instance multiple times if the same string content is requested
         * multiple times.
         *
         * CURRENTLY, AString does not support caching constants.
         * @param data[in] The content of the string.
         * @param length[in] The length of the data buffer.
         * @return The pointer to the instantiated string constant.
         */
        virtual const void *
        GetStringConstant(const char *data, asUINT length);

        /**
         * From the AngelScript documentation:
         * The engine will call this method for each pointer returned by
         * GetStringConstant. If the string factory returns a pointer to the
         * same instance multiple times, then the string instance can only be
         * destroyed when the last call to ReleaseStringConstant for that
         * pointer is made.
         *
         * CURRENTLY, AString simply deletes the provided string.
         * @param str[in] The same pointer returned by GetStringConstant.
         * @return A negative value on error.
         */
        virtual int ReleaseStringConstant(const void *str);

        /**
         * From the AngelScript documentation:
         * The engine will first call this with data set to null to retrieve
         * the size of the buffer that must be allocated. Then the engine will
         * call the method once more with the allocated data buffer to be
         * filled with the content. The length should always be informed in
         * number of bytes.
         * @param str[in] The same pointer returned by GetStringConstant.
         * @param data[out] A pointer to the data buffer that should be filled
         * with the content.
         * @param length[in,out] A pointer to the variable that should be set
         * with the length of the data.
         * @return A negative value on error.
         */
        virtual int GetRawStringData(
            const void *str,
            char *data,
            asUINT *length) const;

    private:
        asIScriptEngine * const engine_ptr; ///< Engine this factory is for.
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_STRINGFACTORY_H
