/*
 * osinterface_StackTrace.h
 */

#ifndef MUTGOS_OSINTERFACE_STACKTRACE_H_
#define MUTGOS_OSINTERFACE_STACKTRACE_H_

#include <string>
#include <list>

namespace mutgos
{
namespace osinterface
{
    /** Static methods to get stack trace for the current thread */
    class StackTrace
    {
    public:
        /**
         * Puts a stack trace of the current thread into trace.
         * @param trace[out] The stack trace for the current thread.
         */
        static void get_stack_trace(std::list<std::string> trace);
    };

} /* namespace osinterface */
} /* namespace mutgos */

#endif /* OSINTERFACE_STACKTRACE_H_ */
