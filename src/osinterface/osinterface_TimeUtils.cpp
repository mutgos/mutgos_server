/*
 * osinterface_TimeUtils.cpp
 */

#include <time.h>

#include "osinterface_TimeUtils.h"

#include "osinterface_OsTypes.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_duration.hpp>

// Defines
//
#define NS_IN_SECS 1000000000


namespace mutgos
{
namespace osinterface
{
    boost::posix_time::ptime TimeUtils::epoch(
        boost::gregorian::date(1970, 1, 1));
    std::string TimeUtils::timezone;

    // -----------------------------------------------------------------------
    void TimeUtils::init(const std::string &tz)
    {
        timezone = tz;
    }

    // -----------------------------------------------------------------------
    OsTypes::TimeEpochType TimeUtils::get_ptime_as_epoch(
        const boost::posix_time::ptime ptime_source)
    {
        const boost::posix_time::time_duration diff(ptime_source - epoch);

        return (diff.ticks() / diff.ticks_per_second());
    }

    // -----------------------------------------------------------------------
    OsTypes::TimeEpochType TimeUtils::get_current_time_utc(void)
    {
        const boost::posix_time::time_duration time_t_diff =
            boost::posix_time::second_clock::universal_time() - epoch;

        return time_t_diff.total_seconds();
    }

    // -----------------------------------------------------------------------
    // Taken from and modified:
    //   https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
    bool TimeUtils::timespec_substract(
        const timespec &lhs,
        const timespec &rhs,
        timespec &result)
    {
        timespec x = lhs;
        timespec y = rhs;

        // Perform the carry for the later subtraction by updating y.
        if (x.tv_nsec < y.tv_nsec)
        {
            const long nsec = (y.tv_nsec - x.tv_nsec) / NS_IN_SECS + 1;
            y.tv_nsec -= NS_IN_SECS * nsec;
            y.tv_sec += nsec;
        }

        if (x.tv_nsec - y.tv_nsec > NS_IN_SECS)
        {
            const long nsec = (x.tv_nsec - y.tv_nsec) / NS_IN_SECS;
            y.tv_nsec += NS_IN_SECS * nsec;
            y.tv_sec -= nsec;
        }

        // Compute the time remaining. tv_nsec is certainly positive.
        result.tv_sec = x.tv_sec - y.tv_sec;
        result.tv_nsec = x.tv_nsec - y.tv_nsec;

        // Return true if result is negative.
        return x.tv_sec < y.tv_sec;
    }

} /* namespace osinterface */
} /* namespace mutgos */
