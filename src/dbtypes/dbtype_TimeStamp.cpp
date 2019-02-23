/*
 * dbtype_DateTime.cpp
 */

#include "dbtype_TimeStamp.h"

#include <string>
#include <iostream>
#include <locale>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/time_duration.hpp>
#include <boost/date_time/local_time/local_time_types.hpp>
#include <boost/date_time/local_time/local_time_io.hpp>

#include "osinterface/osinterface_TimeUtils.h"

#define HOURS_DAY 24
#define DAYS_YEAR 365 // Doesn't need to be exact for relative time

namespace mutgos
{
namespace dbtype
{
    // -----------------------------------------------------------------------
    TimeStamp::TimeStamp(const bool set_now)
    {
        if (set_now)
        {
            set_to_now();
        }
        else
        {
            stored_time_epoch = 0;
        }
    }

    // -----------------------------------------------------------------------
    TimeStamp::TimeStamp(const osinterface::OsTypes::TimeEpochType time)
    {
        set_time(time);
    }

    // -----------------------------------------------------------------------
    TimeStamp::TimeStamp(const TimeStamp &rhs)
    {
        copy(rhs);
    }

    // -----------------------------------------------------------------------
    TimeStamp::~TimeStamp()
    {
        // Nothing to do
    }

    // -----------------------------------------------------------------------
    std::string TimeStamp::to_string(void) const
    {
        return format_time_stamp();
    }

    // -----------------------------------------------------------------------
    void TimeStamp::set_to_now(void)
    {
        stored_time =
            boost::posix_time::from_time_t(
                osinterface::TimeUtils::get_current_time_utc());

        stored_time_local = LocalTimeAdjust::utc_to_local(stored_time);
        stored_time_epoch =
            osinterface::TimeUtils::get_ptime_as_epoch(stored_time);
    }

    // -----------------------------------------------------------------------
    TimeStamp::YearType TimeStamp::get_year(void) const
    {
        return (YearType) stored_time_local.date().year();
    }

    // -----------------------------------------------------------------------
    TimeStamp::MonthType TimeStamp::get_month(void) const
    {
        return (MonthType) stored_time_local.date().month();
    }

    // -----------------------------------------------------------------------
    TimeStamp::DayType TimeStamp::get_day(void) const
    {
        return (DayType) stored_time_local.date().day();
    }

    // -----------------------------------------------------------------------
    TimeStamp::HourType TimeStamp::get_hour(void) const
    {
        return (HourType) stored_time_local.time_of_day().hours();
    }

    // -----------------------------------------------------------------------
    TimeStamp::MinuteType TimeStamp::get_minute(void) const
    {
        return (MinuteType) stored_time_local.time_of_day().minutes();
    }

    // -----------------------------------------------------------------------
    TimeStamp::SecondType TimeStamp::get_second(void) const
    {
        return (SecondType) stored_time_local.time_of_day().seconds();
    }

    // -----------------------------------------------------------------------
    std::string TimeStamp::format_time_stamp(void) const
    {
        boost::local_time::local_time_facet * const output_facet =
            new boost::local_time::local_time_facet("%Y/%m/%d %H:%M:%S ");
        std::stringstream stream;

        stream.imbue(std::locale(std::locale::classic(), output_facet));

        stream << stored_time_local;

        return stream.str() + osinterface::TimeUtils::get_timezone();
    }

    // -----------------------------------------------------------------------
    void TimeStamp::get_relative_time_stamp(
        TimeStamp::TimeType &time,
        TimeStamp::RelativeTimeUnit &unit) const
    {
        time = 0;
        unit = RELATIVETIMEUNIT_seconds;

        const TimeStamp current;
        const boost::posix_time::time_duration diff =
            (current.stored_time - stored_time);

        if (diff.is_special())
        {
            // Essentially happened now.
            return;
        }
        else
        {
            const unsigned long int days = diff.hours() / HOURS_DAY;
            const unsigned long int years = days / DAYS_YEAR;

            if (years)
            {
                unit = RELATIVETIMEUNIT_years;
                time = years;
            }
            else if (days)
            {
                unit = RELATIVETIMEUNIT_days;
                time = days;
            }
            else if (diff.hours())
            {
                unit = RELATIVETIMEUNIT_hours;
                time = diff.hours();
            }
            else if (diff.minutes())
            {
                unit = RELATIVETIMEUNIT_minutes;
                time = diff.minutes();
            }
            else
            {
                time = diff.seconds();
            }
        }
    }

    // -----------------------------------------------------------------------
    MG_LongUnsignedInt TimeStamp::get_relative_seconds(void) const
    {
        const TimeStamp current;
        const boost::posix_time::time_duration diff =
            (current.stored_time - stored_time);

        if (diff.is_special())
        {
            // Essentially happened now.
            return 0;
        }
        else
        {
            return (MG_LongUnsignedInt) diff.total_seconds();
        }
    }

    // -----------------------------------------------------------------------
    bool TimeStamp::operator==(const TimeStamp &rhs) const
    {
        return stored_time_epoch == rhs.stored_time_epoch;
    }

    // -----------------------------------------------------------------------
    TimeStamp &TimeStamp::operator=(const TimeStamp &rhs)
    {
        if (&rhs != this)
        {
            copy(rhs);
        }

        return *this;
    }

    // -----------------------------------------------------------------------
    bool TimeStamp::operator<(const TimeStamp &rhs) const
    {
        return stored_time_epoch < rhs.stored_time_epoch;
    }

    // -----------------------------------------------------------------------
    osinterface::OsTypes::TimeEpochType TimeStamp::get_time(void) const
    {
        return stored_time_epoch;
    }

    // -----------------------------------------------------------------------
    void TimeStamp::set_time(const osinterface::OsTypes::TimeEpochType time)
    {
        stored_time =
            boost::posix_time::from_time_t(time);

        stored_time_local = LocalTimeAdjust::utc_to_local(stored_time);
        stored_time_epoch = time;
    }

    // -----------------------------------------------------------------------
    void TimeStamp::copy(const TimeStamp &source)
    {
        stored_time = source.stored_time;
        stored_time_local = source.stored_time_local;
        stored_time_epoch = source.stored_time_epoch;
    }
} /* namespace dbtype */
} /* namespace mutgos */
