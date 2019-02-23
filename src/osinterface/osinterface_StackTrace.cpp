/*
 * osinterface_StackTrace.cpp
 */

#include <string>
#include <list>

#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>

#include <boost/core/demangle.hpp>

#include "osinterface_OsTypes.h"
#include "osinterface_StackTrace.h"


namespace
{
    const MG_UnsignedInt STACK_SIZE = 100;
}

namespace mutgos
{
namespace osinterface
{
    // ----------------------------------------------------------------------
    void StackTrace::get_stack_trace(std::list<std::string> trace)
    {
        //
        // Taken from backtrace man page example
        //

        void *buffer[STACK_SIZE];
        char **stack_strings;
        const int ptr_count = backtrace(buffer, STACK_SIZE);

        stack_strings = backtrace_symbols(buffer, ptr_count);

        if (stack_strings)
        {
            for(int index = 0; index < ptr_count; ++index)
            {
                trace.push_back(boost::core::demangle(stack_strings[index]));
            }
        }

        free(stack_strings);
    }
} /* namespace osinterface */
} /* namespace mutgos */
