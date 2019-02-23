/*
 * dbtype_DateTime.h
 */

#ifndef MUTGOS_DBTYPE_TIMESTAMP_H_
#define MUTGOS_DBTYPE_TIMESTAMP_H_

#include <string>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a timestamp.  The timestamp is always in UTC, but is
     * converted to local time depending on the method call.
     * TOD == Time Of Day
     */
    class TimeStamp
    {
    public:
        typedef unsigned short TimeType;  ///< Type for year in TOD
        typedef unsigned short YearType;  ///< Type for year in TOD
        typedef unsigned char MonthType;  ///< Type for month in TOD
        typedef unsigned char DayType;    ///< Type for day in TOD
        typedef unsigned char HourType;   ///< Type for hour in TOD
        typedef unsigned char MinuteType; ///< Type for minute in TOD
        typedef unsigned char SecondType; ///< Type for second in TOD

        /** Used when formatting relative times.  Indicates the unit of
            time. */
        enum RelativeTimeUnit
        {
            RELATIVETIMEUNIT_seconds,  ///< TimeStamp is seconds ago
            RELATIVETIMEUNIT_minutes,  ///< TimeStamp is minutes ago
            RELATIVETIMEUNIT_hours,    ///< TimeStamp is hours ago
            RELATIVETIMEUNIT_days,     ///< TimeStamp is days ago
            RELATIVETIMEUNIT_years     ///< TimeStamp is years ago
        };

        /**
         * Constructs a TimeStamp using the current UTC date and time.
         *
         * @param set_now[in] If true, set TimeStamp to the current UTC
         * date and time.  If false, leave at zero (used for deserialization).
         */
        TimeStamp(const bool set_now = true);

        /**
         * Constructs a TimeStamp using the provided time since epoch.
         * Typically used when deserializing from backend database.
         * @param time[in] The time since the epoch, in seconds.  UTC.
         */
        TimeStamp(const osinterface::OsTypes::TimeEpochType time);

        /**
         * Copy constructor.
         * @param rhs[in] The TimeStamp to copy.
         */
        TimeStamp(const TimeStamp &rhs);

        /**
         * Destructor.
         */
        virtual ~TimeStamp();

        /**
         * @return This TimeStamp is string form.
         */
        std::string to_string(void) const;

        /**
         * @return Approximate memory used by this class instance, in bytes.
         */
        inline size_t mem_used(void) const
        {
            return sizeof(TimeStamp)
                    + sizeof(stored_time)
                    + sizeof(stored_time_local);
        }

        /**
         * Sets this TimeStamp to 'now'.
         */
        void set_to_now(void);

        /**
         * @return The year of the timestamp.
         */
        YearType get_year(void) const;

        /**
         * @return The month of the timestamp.
         */
        MonthType get_month(void) const;

        /**
         * @return The day of the timestamp.
         */
        DayType get_day(void) const;

        /**
         * @return The hour of the timestamp.
         */
        HourType get_hour(void) const;

        /**
         * @return The minute of the timestamp.
         */
        MinuteType get_minute(void) const;

        /**
         * @return The second of the timestamp.
         */
        SecondType get_second(void) const;

        /**
         * @return The TimeStamp in a format suitable for display in a
         * standarized way.
         */
        std::string format_time_stamp(void) const;

        /**
         * Returns the data needed to create a relative time stamp (such
         * as 'Happened 5 minutes ago').  Because of internationalization,
         * the actual string is not returned.
         * @param time[out] The relative time.
         * @param unit[out] The unit of the relative time.
         */
        void get_relative_time_stamp(
            TimeType &time,
            RelativeTimeUnit &unit) const;

        /**
         * @return Returns how many seconds ago from 'now' this timestamp is.
         */
        MG_LongUnsignedInt get_relative_seconds(void) const;

        /**
         * Compares against another TimeStamp.
         * @param[in] rhs The TimeStamp to compare against.
         * @return True if the two instances are the same.
         */
        bool operator==(const TimeStamp &rhs) const;

        /**
         * Assignment operator.
         * @param[in] rhs The source to copy from.
         * @return The copy.
         */
        TimeStamp &operator=(const TimeStamp &rhs);

        /**
         * Compares against another TimeStamp.
         * @param[in] rhs The TimeStamp to compare against.
         * @return True if this TimeStamp is less than rhs.
         */
        bool operator<(const TimeStamp &rhs) const;

        /**
         * @return The UTC time as seconds from the epoch.
         */
        osinterface::OsTypes::TimeEpochType get_time(void) const;

        /**
         * Sets this TimeStamp to the UTC time as provided.
         * @param time[in] The UTC timestamp as seconds from the epoch.
         */
        void set_time(const osinterface::OsTypes::TimeEpochType time);

    private:
        typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime>
            LocalTimeAdjust;

        // Serialization using Boost Serialization
        //
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & stored_time_epoch;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & stored_time_epoch;

            set_time(stored_time_epoch);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

        /**
         * Copies a TimeStamp into this one.  Helper method.
         * @param source The TimeStamp to copy.
         */
        inline void copy(const TimeStamp &source);

        boost::posix_time::ptime stored_time; ///< time, UTC
        boost::posix_time::ptime stored_time_local; ///< time, local
        osinterface::OsTypes::TimeEpochType stored_time_epoch; ///< From epoch
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_TIMESTAMP_H_ */
