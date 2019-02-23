/*
 * utility_StringParsing.h
 */

#ifndef MUTGOS_TEXT_STRINGPARSING_H
#define MUTGOS_TEXT_STRINGPARSING_H

#include <string>
#include <vector>

namespace mutgos
{
namespace text
{
    /**
     * Static methods that aid in parsing strings.
     */
    class StringParsing
    {
    public:
        /** Result of splitting strings */
        typedef std::vector<std::string> SplitStrings;

        /**
         * Splits the string along the given separator (entire separator, not
         * per character), trimming for each item and never
         * including the separator in the output.  If the separator is not
         * found, the entire string is returned as-is, trimmed.
         *
         * For instance, if the string is "Hello  ||Hi  " and the separator
         * is "||", the output is [Hello],[Hi].
         * @param str[in] The string to split.
         * @param separator[in] The separator characters.  If this is empty or
         * the separator is not found, the entire string is returned as-is.
         * @param ignore_empty_token_data[in] If true, tokens that are back
         * to back (have no content) will be ignored and not included in the
         * output.
         * @return The split string.
         */
        static SplitStrings split_string(
            const std::string &str,
            const std::string &separator,
            const bool ignore_empty_token_data = false);

        /**
         * Splits a substring along the given separator (entire separator, not
         * per character), trimming for each item and never
         * including the separator in the output.  If the separator is not
         * found, the entire string is returned as-is, trimmed.
         *
         * For instance, if the string is "Hello  ||Hi  " and the separator
         * is "||", the output is [Hello],[Hi].
         * @param str[in] The string to split.
         * @param start_pos[in] The index within the string to start
         * processing at.  If this is beyond the string size, an empty
         * string will be returned.
         * @param length[in] The number of characters from start_pos to
         * consider for processing.  If this is beyond the string size, it
         * will be truncated.
         * @param separator[in] The separator character(s).  If this is empty or
         * the separator is not found, the entire string is returned as-is.
         * @param max_strings[in] Maximum number of split strings to output,
         * used to prevent attacks.  Optional (0 to disable).
         * @param clear_if_max[in] If max strings are reached, clear the
         * output when stopping.
         * @return The split string.  There will always be at least one
         * element, though it may be empty.
         */
        static SplitStrings split_string(
            const std::string &str,
            const std::string::size_type start_pos,
            const std::string::size_type length,
            const std::string &separator,
            const unsigned int max_strings = 0,
            const bool clear_if_max = false);
    };
}
}

#endif //MUTGOS_TEXT_STRINGPARSING_H
