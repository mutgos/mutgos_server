/*
 * text_StringConversion.h
 */

#ifndef MUTGOS_TEXT_STRINGCONVERSION_H
#define MUTGOS_TEXT_STRINGCONVERSION_H

#include <string>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

namespace mutgos
{
namespace text
{
    // TODO Make note that below 'raw' and such could become interfaces to allow internationalization??
    // TODO Retrofit as many classes as possible to use new methods below

    /**
     * Converts the given input to string form.
     * @tparam OBJ Any non-string class/primitive type that can be converted
     * to a string.
     * @param object[in] The class/primitive to convert to a string.
     * @return object in string form, or "<INVALID CONVERSION>" if failed
     * to convert to string.
     */
    template <typename OBJ>
    static std::string to_string(const OBJ &object)
    {
        try
        {
            return boost::lexical_cast<std::string>(object);
        }
        catch(const boost::bad_lexical_cast &bad)
        {
            return "<INVALID CONVERSION: " + std::string(bad.what()) + ">";
        }
        catch (...)
        {
            return "<INVALID CONVERSION>";
        }
    }

    /**
     * Converts the given string input to a primitive form
     * (int, double, etc).
     * @tparam OBJ Any non-string primitive type that can be converted
     * from a string.
     * @param str[in] The string to convert to OBJ.
     * @return String in primitive form, or 0 (false) if failed.
     */
    template <typename OBJ>
    static OBJ from_string(const std::string &str)
    {
        try
        {
            return boost::lexical_cast<OBJ>(str);
        }
        catch (...)
        {
            return 0;
        }
    }

    /**
     * Converts the given string input to a primitive form
     * (int, double, etc).
     * @tparam OBJ Any non-string primitive type that can be converted
     * from a string.
     * @param str[in] The string to convert to OBJ.
     * @param out[out] The string in converted form.
     * @return True if conversion succeeded, false if not.  If false, out will
     * not be modified.
     */
    template <typename OBJ>
    static bool from_string(const std::string &str, OBJ &out)
    {
        try
        {
            out = boost::lexical_cast<OBJ>(str);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /**
     * Converts the string to all lowercase in-place.
     * @param str[in,out] The string to convert to all lowercase.  It will
     * be modified in place.
     */
    static void to_lower(std::string &str)
    {
        boost::algorithm::to_lower(str);
    }

    /**
     * Converts the string to all lowercase, returning the result.
     * @param str[in] The string to convert to all lowercase.
     * @return The input string in lowercase.
     */
    static std::string to_lower_copy(const std::string &str)
    {
        return boost::algorithm::to_lower_copy(str);
    }

    /**
     * Converts the string to all uppercase in-place.
     * @param str[in,out] The string to convert to all uppercase.  It will
     * be modified in place.
     */
    static void to_upper(std::string &str)
    {
        boost::algorithm::to_upper(str);
    }

    /**
     * Converts the string to all uppercase, returning the result.
     * @param str[in] The string to convert to all uppercase.
     * @return The input string in uppercase.
     */
    static std::string to_upper_copy(const std::string &str)
    {
        return boost::algorithm::to_upper_copy(str);
    }

    /**
     * Removes excess spaces in the front and behind of string.
     * @param str[in] The string to trim spaces from.
     * @return A copy of str with excess spaces removed.
     */
    static std::string trim_copy(const std::string &str)
    {
        return boost::trim_copy(str);
    }

    /**
     * Removes excess spaces in the front and behind of string.
     * @param str[in,out] The string to trim spaces from.  The string will
     * be modified.
     */
    static void trim(std::string &str)
    {
        boost::trim(str);
    }
}
}

#endif //MUTGOS_TEXT_STRINGCONVERSION_H
