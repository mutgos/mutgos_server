/*
 * angelscript_StringFactory.cpp
 */

#include <string.h>
#include <angelscript.h>

#include "angelscript_AString.h"
#include "angelscript_StringFactory.h"

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    StringFactory::StringFactory(asIScriptEngine *engine)
      : engine_ptr(engine)
    {
    }

    // ----------------------------------------------------------------------
    StringFactory::~StringFactory()
    {
    }

    // ----------------------------------------------------------------------
    const void *StringFactory::GetStringConstant(
        const char *data,
        asUINT length)
    {
        AString * const string = new AString(engine_ptr, data, length);

        return string;
    }

    // ----------------------------------------------------------------------
    int StringFactory::ReleaseStringConstant(const void *str)
    {
        if (str)
        {
            const AString * const astr = reinterpret_cast<const AString *>(str);
            delete astr;
        }

        return 0;
    }

    // ----------------------------------------------------------------------
    int StringFactory::GetRawStringData(
        const void *str,
        char *data,
        asUINT *length) const
    {
        int rc = 0;

        if (not str)
        {
            rc = -1;
        }
        else
        {
            const AString * const astr = reinterpret_cast<const AString *>(str);

            if (not data)
            {
                // Asking for size to allocate for data
                //
                if (not length)
                {
                    rc = -2;
                }
                else
                {
                    *length = (asUINT) astr->size();
                }
            }
            else
            {
                // Asking to populate data
                memcpy(data, astr->get_raw_data(), *length);
            }
        }

        return rc;
    }
}
}
