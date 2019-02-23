/*
 * utility_StringParsing.cpp
 */


#include <boost/algorithm/string/trim.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "text_StringParsing.h"

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    StringParsing::SplitStrings StringParsing::split_string(
        const std::string &str,
        const std::string &separator,
        const bool ignore_empty_token_data)
    {
        SplitStrings result;

        // Short circuit empty separator or string
        //
        if (separator.empty())
        {
            result.push_back(boost::trim_copy(str));
            return result;
        }

        if (str.empty())
        {
            result.push_back(str);
            return result;
        }

        // Expected case.
        //
        const size_t size = str.size();
        const size_t sep_size = separator.size();
        size_t index = 0;
        size_t search_index = 0;

        while (index < size)
        {
            search_index = str.find(separator, index);

            if (search_index == std::string::npos)
            {
                // No more separators found.  Add last part of string and we're
                // done.
                result.push_back(boost::trim_copy(str.substr(index)));
                index = size;
            }
            else
            {
                if (not ignore_empty_token_data)
                {
                    // Add the portion of string between here and the separator
                    result.push_back(boost::trim_copy(
                        str.substr(index, search_index - index)));
                }
                else if (search_index != index)
                {
                    // Caller does not want empty entries; this one does have
                    // data in it.
                    result.push_back(boost::trim_copy(
                        str.substr(index, search_index - index)));
                }

                // Advance past the separator to search again
                index = search_index + sep_size;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    StringParsing::SplitStrings StringParsing::split_string(
        const std::string &str,
        const std::string::size_type start_pos,
        const std::string::size_type length,
        const std::string &separator,
        const MG_UnsignedInt max_strings,
        const bool clear_if_max)
    {
        SplitStrings result;

        // Short circuit empty separator
        //
        if (separator.empty())
        {
            result.push_back(boost::trim_copy(str.substr(start_pos, length)));
            return result;
        }

        // Short circuit string empty, no length, or start position
        // beyond string end.
        //
        if (str.empty() || (not length) || (start_pos >= str.size()))
        {
            result.push_back(std::string());
            return result;
        }

        const std::string::size_type sep_size = separator.size();
        std::string::size_type max_index  = start_pos + length;
        size_t index = start_pos;
        size_t search_index = 0;

        // Adjust end index if past end.
        //
        if (max_index > str.size())
        {
            max_index = str.size();
        }

        while (index < max_index)
        {
            search_index = str.find(separator, index);

            if (search_index == std::string::npos)
            {
                // No more separators found.  Add last part of string and we're
                // done.
                result.push_back(boost::trim_copy(
                    str.substr(index, max_index - index)));
                index = max_index;
            }
            else if (search_index >= max_index)
            {
                // Next separator is beyond what we're allowed to process.
                // Stop here.
                index = max_index;
            }
            else
            {
                // Add the portion of string between here and the separator
                result.push_back(boost::trim_copy(
                    str.substr(index, search_index - index)));

                // Advance past the separator to search again
                index = search_index + sep_size;
            }

            if (max_strings and (result.size() >= max_strings))
            {
                // Reached our maximum.  End early.
                //
                if (clear_if_max)
                {
                    result.clear();
                }

                break;
            }
        }

        return result;
    }
}
}
