/*
 * text_Utf8Tools.h
 */

#ifndef MUTGOS_TEXT_UTF8TOOLS_H
#define MUTGOS_TEXT_UTF8TOOLS_H

#include <string>

namespace mutgos
{
namespace text
{
    /**
     * Converts a UTF-8 line into extended ASCII.
     * @param line[in] The UTF-8 line to convert.
     * @return The line in extended ASCII form.
     */
    std::string convert_utf8_to_extended(const std::string &line);

    /**
     * Converts an extended ASCII line to UTF-8 format.
     * @param line[in] The extended ASCII line to convert.
     * @return The line in UTF-8 form.
     */
    std::string convert_extended_to_utf8(const std::string &line);

    /**
     * Converts the raw bits into UTF-8 format.
     * Not normally used directly.
     * @param data[in] the raw bits to convert to UTF8
     * @param bits[in] The number of bits (from LSB) in data to convert to UTF-8
     * @param output[out] The UTF-8 encoded character will be appended to this.
     */
    void convert_bits_to_utf8(
        const u_int32_t data,
        const unsigned char bits,
        std::string &output);

    /**
     * Converts the UTF8 two byte encoded form into a single byte
     * extended ascii form, if it's within range.
     * Not normally used directly.
     * @param first The first (leading) UTF8 encoded byte
     * @param second The second (data) UTF8 encoded byte
     * @return first + second as a single extended ascii character, or
     * '?' if invalid.
     */
    unsigned char convert_bits_to_extended(
        const unsigned char first,
        const unsigned char second);

    /**
     * @param str[in] The string to check.
     * @return True if string is valid utf8 and completely printable,
     * false if not.  Unprintable non-UTF8 codes are considered invalid.
     */
    bool utf8_valid(const std::string &str);

    /**
     * @param str[in] The string to check.
     * @param len[in] The length of the string.
     * @return True if string is valid utf8 and completely printable,
     * false if not.  Unprintable non-UTF8 codes are considered invalid.
     */
    bool utf8_valid(const char *str, const size_t len);

    /**
     * Determines the 'UTF8 size' of a string.  Normally each byte
     * in a string is considered an element.  The size is therefore the
     * number of bytes.  In order to standardize how UTF8 strings
     * are manipulated, the 'UTF8 size' considers a code point to be
     * one 'element'.  Example: A string with one ascii character and
     * a 4 byte UTF8 character has a size of 2.  Methods below aid in
     * converting byte position to an index relative to 'UTF8 size'.
     * @param str[in] The string to get the UTF8 size of.
     * @return The UTF8 size of the string, or 0 if
     * error or not valid UTF8.
     */
    size_t utf8_size(const std::string &str);

    /**
     * @param str[in] The string to get the UTF8 size of.
     * @param len[in] The length of str.
     * @return The UTF8 size of the string, or 0 if
     * error or not valid UTF8.
     * @see utf8_size(const std::string &str) for explanation of
     * 'UTF8 size' and indexing.
     */
    size_t utf8_size(const char *str, const size_t len);

    /**
     * Converts a 'UTF8 index' to the actual byte index in the string.
     * This is useful for making substrings, searching, etc.
     * @param str[in] The string to get the byte index for.
     * @param utf8_index[in] The UTF8 index to look up the byte index for.
     * @return The byte index in str that corresponds to utf8_index.  If
     * an error occurs (out of bounds, etc), 0 is returned.
     * @see utf8_size(const std::string &str) for explanation of
     * 'UTF8 size' and indexing.
     */
    size_t utf8_index_to_byte(const std::string &str, const size_t utf8_index);

    /**
     * Converts a 'UTF8 index' to the actual byte index in the string.
     * This is useful for making substrings, searching, etc.
     * @param str[in] The string to get the byte index for.
     * @param len[in] The length of the string.
     * @param utf8_index[in] The UTF8 index to look up the byte index for.
     * @return The byte index in str that corresponds to utf8_index.  If
     * an error occurs (out of bounds, etc), 0 is returned.
     * @see utf8_size(const std::string &str) for explanation of
     * 'UTF8 size' and indexing.
     */
    size_t utf8_index_to_byte(
        const char *str,
        const size_t len,
        const size_t utf8_index);

    /**
     * Converts a byte index in a string to the 'UTF8 index'.
     * @param str[in] The string to get the UTF8 index for.
     * @param byte[in] The byte index to look up the UTF8 index for.
     * @return The UTF8 index, or 0 if error.
     * @see utf8_size(const std::string &str) for explanation of
     * 'UTF8 size' and indexing.
     */
    size_t utf8_byte_to_index(const std::string &str, const size_t byte);

    /**
     * Converts a byte index in a string to the 'UTF8 index'.
     * @param str[in] The string to get the UTF8 index for.
     * @param len[in] The length of str.
     * @param byte[in] The byte index to look up the UTF8 index for.
     * @return The UTF8 index, or 0 if error.
     * @see utf8_size(const std::string &str) for explanation of
     * 'UTF8 size' and indexing.
     */
    size_t utf8_byte_to_index(
        const char *str,
        const size_t len,
        const size_t byte);

    /**
     * Finds a UTF8 string inside of a larger UTF8 string.
     * @param str[in] The string to search inside.  Must be valid UTF8.
     * @param to_find[in] The substring to find inside of str.
     * Must be valid UTF8.
     * @param start[in] The UTF8 index to start searching at.
     * @return The UTF8 index of the found substring, or std::string::npos
     * if not found.
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    size_t utf8_find(
        const std::string &str,
        const std::string &to_find,
        const size_t start = 0);

    /**
     * Finds a UTF8 string inside of a larger UTF8 string.
     * @param str[in] The string to search inside.  Must be valid UTF8.
     * @param len[in] The length of str.
     * @param to_find[in] The substring to find inside of str.
     * Must be valid UTF8.
     * @param to_find_len[in] The length of to_find.
     * @param start[in] The UTF8 index to start searching at.
     * @return The UTF8 index of the found substring, or std::string::npos
     * if not found.
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    size_t utf8_find(
        const char *str,
        const size_t len,
        const char *to_find,
        const size_t to_find_len,
        const size_t start = 0);

    /**
     * Finds the last occurance of a UTF8 string inside of a larger UTF8 string.
     * @param str[in] The string to search inside.  Must be valid UTF8.
     * @param to_find[in] The substring to find inside of str.
     * Must be valid UTF8.
     * @return The UTF8 index of the last found substring, or std::string::npos
     * if not found.
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    size_t utf8_find_last(
        const std::string &str,
        const std::string &to_find);

    /**
     * Finds the last occurance of a UTF8 string inside of a larger UTF8 string.
     * @param str[in] The string to search inside.  Must be valid UTF8.
     * @param len[in] The length of str.
     * @param to_find[in] The substring to find inside of str.
     * Must be valid UTF8.
     * @param to_find_len[in] The length of to_find.
     * @return The UTF8 index of the last found substring, or std::string::npos
     * if not found.
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    size_t utf8_find_last(
        const char *str,
        const size_t len,
        const char *to_find,
        const size_t to_find_len);

    /**
     * Gets the character at the given UTF8 index.  If this is a UTF8
     * character, multiple bytes will be returned.  Must be valid UTF8.
     * @param str[in] The string to get the character from.  Must be valid UTF8.
     * @param utf8_index[in] The UTF8 index of the character to retrieve.
     * @return The character at the UTF8 index, or empty if error
     * (out of bounds, etc).
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    std::string utf8_char_at(const std::string &str, const size_t utf8_index);

    /**
     * Gets the character at the given UTF8 index.  If this is a UTF8
     * character, multiple bytes will be returned.  Must be valid UTF8.
     * @param str[in] The string to get the character from.  Must be valid UTF8.
     * @param len[in] The length of str.
     * @param utf8_index[in] The UTF8 index of the character to retrieve.
     * @return The character at the UTF8 index, or empty if error
     * (out of bounds, etc).
     * @see utf8_size() for explanation on 'UTF8 index'.
     */
    std::string utf8_char_at(
        const char *str,
        const size_t len,
        const size_t utf8_index);

    /**
     * Used to ensure a string is no bigger than a given number of UTF8
     * characters.  Must be valid UTF8.
     * @param str[in] The string to limit.
     * @param utf8_size[in] How many UTF8 characters to allow.
     * @return str with no more than utf8_size UTF8 characters.
     * @see utf8_size() for explanation on 'UTF8 index' / size
     */
    std::string utf8_chop_at_limit(
        const std::string &str,
        const size_t utf8_size);

    /**
     * Used to ensure a string is no bigger than a given number of UTF8
     * characters.  Must be valid UTF8.
     * @param str[in] The string to limit.
     * @param len[in] The length of str.
     * @param utf8_size[in] How many UTF8 characters to allow.
     * @return str with no more than utf8_size UTF8 characters.
     * @see utf8_size() for explanation on 'UTF8 index' / size
     */
    std::string utf8_chop_at_limit(
        const char *str,
        const size_t len,
        const size_t utf8_size);

    /**
     * Used to ensure a string is no bigger than a given number of UTF8
     * characters.  Must be valid UTF8.
     * @param str[in, out] The string to limit.  It will be modified to
     * the desired limit.
     * @param utf8_size[in] How many UTF8 characters to allow.
     * @see utf8_size() for explanation on 'UTF8 index' / size.
     */
    void utf8_chop_modify_at_limit(
        std::string &str,
        const size_t utf8_size);

    /**
     * Splits a UTF8 string into two strings.  Must be valid UTF8.
     * @param str[in] The string to split.
     * @param utf8_index[in] The index to split the string around.
     * @param before_str[out] Everything before utf8_index goes here.
     * @param after_str[out] Everything at or after utf8_index goes here.
     * @see utf8_size() for explanation on 'UTF8 index' / size.
     */
    void utf8_split(
        const std::string &str,
        const size_t utf8_index,
        std::string &before_str,
        std::string &after_str);

    /**
     * Splits a UTF8 string into two strings.  Must be valid UTF8.
     * @param str[in] The string to split.
     * @param len[in] The length of str.
     * @param utf8_index[in] The index to split the string around.
     * @param before_str[out] Everything before utf8_index goes here.
     * @param after_str[out] Everything at or after utf8_index goes here.
     * @see utf8_size() for explanation on 'UTF8 index' / size.
     */
    void utf8_split(
        const char *str,
        const size_t len,
        const size_t utf8_index,
        std::string &before_str,
        std::string &after_str);
}
}

#endif //MUTGOS_TEXT_UTF8TOOLS_H
