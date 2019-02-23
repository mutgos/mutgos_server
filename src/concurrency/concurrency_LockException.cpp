/*
 * LockException.cpp
 */

#include <string>

#include "concurrency/concurrency_LockException.h"
#include "osinterface/osinterface_StackTrace.h"

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/core/demangle.hpp>


namespace mutgos
{
namespace concurrency
{
    // ----------------------------------------------------------------------
    LockException::LockException()
    {
        StackTrace stack;

        osinterface::StackTrace::get_stack_trace(stack);

        *this << StackTraceInfo(stack);
    }

    // ----------------------------------------------------------------------
    const char* LockException::what() const throw()
    {
        return "A mismatch in locking tokens was detected.";
    }

    // ----------------------------------------------------------------------
    LockException::~LockException() throw()
    {
    }

} /* namespace concurrency */
} /* namespace mutgos */
