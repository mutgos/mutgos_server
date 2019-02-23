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
}
}

#endif //MUTGOS_TEXT_UTF8TOOLS_H
