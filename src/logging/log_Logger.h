/*
 * log_logger.h
 */

#ifndef MUTGOS_LOG_LOGGER_H_
#define MUTGOS_LOG_LOGGER_H_

#include <string>

#include "osinterface/osinterface_ThreadUtils.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

/** The severity level of the log.  Outside a namespace for ease of use */
enum LoggingSeverityLevel
{
    debug,
    info,
    warning,
    error,
    fatal,
    loggingseveritylevel_END ///< Internal use only
};

// Logs only if the severity level passes the filter.
#ifndef LOG
#define LOG(severity, namesp, method, entry) \
    if (log::Logger::get_level() <= severity) \
    log::Logger::log_text(severity, namesp, __FILE__, method, __LINE__, entry)
#endif

// Always logs the text regardless of filter setting.
#ifndef ALWAYS_LOG
#define ALWAYS_LOG(severity, namesp, method, entry) \
    log::Logger::log_text(severity, namesp, __FILE__, method, __LINE__, entry)
#endif

namespace mutgos
{
namespace log
{
    class Logger
    {
    public:
        /**
         * Initializes the logger.  Only call once on program startup.
         * @param to_console[in] True if logging to console, false if to a file.
         */
        static void init(const bool to_console = false);

        /**
         * Sets the global logging level.
         * @param level[in] The new logging level.
         */
        static void set_level(const LoggingSeverityLevel level);

        /**
         * @return The current global logging level.
         */
        static inline LoggingSeverityLevel get_level(void)
        {
            return current_level;
        }

        /**
         * @param level[in] The logging level to check.
         * @return True if logger will output text at the provided level.
         * @see set_level()
         */
        static inline bool can_log(const LoggingSeverityLevel level)
        {
            return current_level <= level;
        }

        /**
         * Unconditionally logs text.  This is normally not called directly -
         * please use the macros!
         * @param level[in] The level to log at.
         * @param namesp[in] The namespace/class we're logging for.
         * @param filename[in] The filename creating the log entry.
         * @param method[in] The method creating the log entry.
         * @param line[in] The line number in the file creating the log entry.
         * @param entry[in] The actual text to log.
         */
        static inline void log_text(
            const LoggingSeverityLevel level,
            const std::string &namesp,
            const std::string &filename,
            const std::string &method,
            const int line,
            const std::string &entry);

    private:
        static LoggingSeverityLevel current_level; ///< Current logging level
        static const std::string level_to_string[]; ///< Maps severity to str
    };

    // -----------------------------------------------------------------------
    inline void Logger::log_text(
        const LoggingSeverityLevel level,
        const std::string &namesp,
        const std::string &filename,
        const std::string &method,
        const int line,
        const std::string &entry)
    {
        if (level < loggingseveritylevel_END)
        {
            BOOST_LOG_TRIVIAL(trace) << "*" << level_to_string[level] << "*  "
                                     << "T"
                                     << osinterface::ThreadUtils::get_thread_id()
                                     << "  "
                                     << namesp << "  " << filename << ":"
                                     << line
                                     << " / " << method << ":  " << entry;
        }
    }
} /* namespace log */
} /* namespace mutgos */

#endif /* LOG_LOGGER_H_ */
