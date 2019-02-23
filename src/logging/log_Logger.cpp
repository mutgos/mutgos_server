/*
 * log_logger.cpp
 */

#include <string>

#include "log_Logger.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace mutgos
{
namespace log
{
    // Statics
    //
    LoggingSeverityLevel Logger::current_level = debug;

    const std::string Logger::level_to_string[] =
        {
            "debug",
            "info",
            "warning",
            "error",
            "fatal"
        };

    // -----------------------------------------------------------------------
    void Logger::init(const bool to_console)
    {
        if (not to_console)
        {
            boost::log::add_file_log(
                boost::log::keywords::file_name = "../logs/mutgos_log_%N.log",
                boost::log::keywords::rotation_size = 50 * 1024 * 1024,
                boost::log::keywords::max_size = 500 * 1024 * 1024,
                boost::log::keywords::format = "[%TimeStamp%]: %Message%");
        }

        // Our class will handle the logging levels
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= boost::log::trivial::trace);

        ALWAYS_LOG(info, "log", "init()", "Log started.");
    }

    // -----------------------------------------------------------------------
    void Logger::set_level(const LoggingSeverityLevel level)
    {
        if ((level < loggingseveritylevel_END) and (level >= debug))
        {
            current_level = level;
        }
    }
} /* namespace log */
} /* namespace mutgos */
