/*
 * LockException.h
 */

#ifndef MUTGOS_CONCURRENCY_LOCKEXCEPTION_H_
#define MUTGOS_CONCURRENCY_LOCKEXCEPTION_H_

#include <exception>
#include <list>
#include <string>

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>

namespace mutgos
{
namespace concurrency
{
    /**
     * Exception used when the lock token check fails.
     */
    class LockException :
        virtual public boost::exception, virtual public std::exception
    {
    public:
        typedef std::list<std::string> StackTrace;
        typedef boost::error_info<struct stack_trace, StackTrace>
            StackTraceInfo;

        LockException();
        virtual ~LockException() throw();

        virtual const char* what() const throw();
    };

} /* namespace concurrency */
} /* namespace mutgos */

#endif /* CONCURRENCY_LOCKEXCEPTION_H_ */
