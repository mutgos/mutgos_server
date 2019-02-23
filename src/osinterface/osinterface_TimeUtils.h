/*
 * osinterface_TimeUtils.h
 */

#ifndef MUTGOS_OSINTERFACE_TIMEUTILS_H_
#define MUTGOS_OSINTERFACE_TIMEUTILS_H_

#include <string>
#include <time.h>

#include "osinterface_OsTypes.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace mutgos
{
namespace osinterface
{
    /**
     * Static methods used to manipulate time, which may be specific to the
     * platform this is running on.
     */
    class TimeUtils
    {
    public:
        /**
         * Initializes the class for use.
         * @param tz[in] A user-defined string that has the timezone.  Only
         * used for display.
         */
        static void init(const std::string &tz);

        /**
         * @return A user defined string representing the timezone.  May
         * be empty, and should only be used for display.
         */
        static inline const std::string &get_timezone(void)
          { return timezone; }

        /**
         * Given a ptime, return it as seconds since the epoch (unix time).
         * @param ptime_source[in] The ptime to convert.
         * @return ptime_source in unix time.
         */
        static OsTypes::TimeEpochType get_ptime_as_epoch(
            const boost::posix_time::ptime ptime_source);

        /**
         * @return The current UTC time in seconds since the epoch (unix time).
         */
        static OsTypes::TimeEpochType get_current_time_utc(void);

        /**
         * Subtracts two timespecs.
         * @param lhs[in] The timespec to subtract from.
         * @param rhs[in] The amount to subtract.
         * @param result[out] The result of the subtraction.
         * @return True if result is negative.
         */
        static bool timespec_substract(
            const timespec &lhs,
            const timespec &rhs,
            timespec &result);

    private:
        static boost::posix_time::ptime epoch; ///< When time began
        static std::string timezone; ///< A user defined string for our TZ
    };
} /* namespace osinterface */
} /* namespace mutgos */

#endif /* OSINTERFACE_TIMEUTILS_H_ */
