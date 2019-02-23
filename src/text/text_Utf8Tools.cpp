/*
 * text_Utf8Tools.cpp
 */

#include <string>

#include "text_Utf8Tools.h"

namespace
{
    const static u_int32_t REPLACEMENT_CHAR = 0xFFFD;
    const static char REPLACEMENT_CHAR_UNPRINTABLE = '?';
    const static char INVALID_TO_SOCKET = '?';

    const static unsigned char PRINTABLE_ASCII_BEGIN = 32;
    const static unsigned char PRINTABLE_ASCII_END = 126;
    const static unsigned char EXT_ASCII_BEGIN = 160;
    const static unsigned char EXT_ASCII_END = 255;

    // 11110000
    const static unsigned char FOUR_BYTE_UTF8_START = 0xF0;
    // 11111000
    const static unsigned char FOUR_BYTE_UTF8_START_SEARCH_MASK = 0xF8;
    // 11100000
    const static unsigned char THREE_BYTE_UTF8_START = 0xE0;
    // 11110000
    const static unsigned char THREE_BYTE_UTF8_START_SEARCH_MASK= 0xF0;
    // 11000000
    const static unsigned char TWO_BYTE_UTF8_START = 0xC0;
    // 11100000
    const static unsigned char TWO_BYTE_UTF8_START_SEARCH_MASK = 0xE0;

    // 10000000
    const static unsigned char UTF8_CONTINUE = 0x80;
    // 11000000
    const static unsigned char UTF8_CONTINUE_SEARCH_MASK = 0xC0;

    // Beginning of printable/standard extended ASCII
    const static unsigned char PRINTABLE_EXT_ASCII_BEGIN = 0xA0;
    // End of printable/standard extended ASCII
    const static unsigned char PRINTABLE_EXT_ASCII_END = 0xFE;
}

namespace mutgos
{
namespace text
{
    // ----------------------------------------------------------------------
    std::string convert_utf8_to_extended(const std::string &line)
    {
        // Basic algorithm:
        // Convert 2 byte to single char, if in valid range.
        // All others replace with ?
        //

        std::string result;

        if (not line.empty())
        {
            const char *line_ptr = line.data();
            unsigned char current_char = (unsigned char) *line_ptr;
            const size_t max = line.size();
            size_t mark = 0;
            size_t index = 0;
            size_t to_skip = 0;

            while (index < max)
            {
                current_char = (unsigned char) line_ptr[index];

                // Determine type of UTF8, if any
                // If correct one, convert, otherwise skip it

                if (current_char > PRINTABLE_ASCII_END)
                {
                    // First copy anything pending.
                    //
                    if (mark != index)
                    {
                        result.append(line_ptr + mark, index - mark);
                    }

                    if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                        TWO_BYTE_UTF8_START)
                    {
                        // Potentially valid 2 byte character.  Attempt to
                        // decode.
                        //
                        if (index >= (max - 1))
                        {
                            // At the end.  Invalid
                            result += INVALID_TO_SOCKET;
                        }
                        else
                        {
                            const unsigned char data_char =
                                (unsigned char) line_ptr[index + 1];

                            if ((data_char & UTF8_CONTINUE_SEARCH_MASK) ==
                                UTF8_CONTINUE)
                            {
                                // Valid UTF8 two byte.  Process and append
                                // result.
                                //
                                result.append(
                                    1,
                                    convert_bits_to_extended(
                                        current_char,
                                        data_char));

                                // We processed two bytes
                                ++index;
                            }
                            else
                            {
                                // Not quite UTF8.  Skip the header byte.
                                result += INVALID_TO_SOCKET;
                            }
                        }

                        mark = index + 1;
                    }
                    else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                             THREE_BYTE_UTF8_START)
                    {
                        // Not valid for conversion, skip.
                        to_skip = 2;
                    }
                    else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                             FOUR_BYTE_UTF8_START)
                    {
                        // Not valid for conversion, skip.
                        to_skip = 3;
                    }
                    else
                    {
                        mark = index + 1;

                        // Unknown. add ? and skip this byte
                        result += INVALID_TO_SOCKET;
                    }
                }

                if (not to_skip)
                {
                    ++index;
                }
                else
                {
                    // Skip the extra parts of the UTF8 bytes.
                    //

                    // Insert a '?' to show the UTF8 was not allowed/invalid
                    result.append(1, INVALID_TO_SOCKET);

                    ++index;

                    while (to_skip and (index < max))
                    {
                        current_char = (unsigned char) line_ptr[index];

                        if ((current_char & UTF8_CONTINUE_SEARCH_MASK) ==
                            UTF8_CONTINUE)
                        {
                            --to_skip;
                            ++index;
                        }
                        else
                        {
                            break;
                        }
                    }

                    mark = index;
                    to_skip = 0;
                }
            }

            // Copy anything left over.
            //
            if (mark < max)
            {
                result.append(line_ptr + mark, max - mark);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string convert_extended_to_utf8(const std::string &line)
    {
        // Basic algorithm:
        // Convert > 127 to UTF8 2 byte, if in valid range.
        // If not in valid range, do ?
        // Copy normal characters in batch for efficiency.

        std::string result;

        if (not line.empty())
        {
            const char *line_ptr = line.data();
            unsigned char current_char = (unsigned char) *line_ptr;
            const size_t max = line.size();
            size_t mark = 0;

            for (size_t index = 0; index < max; ++index)
            {
                current_char = (unsigned char) line_ptr[index];

                if (current_char < PRINTABLE_ASCII_BEGIN)
                {
                    // First copy anything pending.
                    //
                    if (mark != index)
                    {
                        result.append(line_ptr + mark, index - mark);
                    }

                    // Nonprintable ASCII.  Insert standard ?
                    result.append(1, REPLACEMENT_CHAR_UNPRINTABLE);

                    mark = index + 1;
                }
                else if (current_char > PRINTABLE_ASCII_END)
                {
                    // Possibly extended ascii that we can convert

                    // First copy anything pending.
                    //
                    if (mark != index)
                    {
                        result.append(line_ptr + mark, index - mark);
                    }

                    if ((current_char >= PRINTABLE_EXT_ASCII_BEGIN) and
                        (current_char <= PRINTABLE_EXT_ASCII_END))
                    {
                        // Valid extended ascii.  Convert
                        convert_bits_to_utf8(current_char, 8, result);
                    }
                    else
                    {
                        // Nonprintable extended ascii.  Turn into ?
                        convert_bits_to_utf8(REPLACEMENT_CHAR, 16, result);
                    }

                    mark = index + 1;
                }
            }

            // Copy anything left over.
            //
            if (mark < max)
            {
                result.append(line_ptr + mark, max - mark);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void convert_bits_to_utf8(
        const u_int32_t data,
        const unsigned char bits,
        std::string &output)
    {
        unsigned char output_char = 0;
        unsigned char bits_left = bits;
        u_int32_t data_working = data;

        if (bits > 16)
        {
            bits_left = 21;

            // 4 byte utf8.  Put in header
            output_char = FOUR_BYTE_UTF8_START;

            // Fill remainder with as many bits (3) as we can
            data_working >>= (bits_left - 3);
            output_char |= (unsigned char) data_working;

            bits_left -= 3;
        }
        else if (bits > 11)
        {
            bits_left = 16;

            // 3 byte utf8.  Put in header
            output_char = THREE_BYTE_UTF8_START;

            // Fill remainder with as many bits (4) as we can
            data_working >>= (bits_left - 4);
            output_char |= (unsigned char) data_working;

            bits_left -= 4;
        }
        else if (bits > 7)
        {
            bits_left = 11;

            // 2 byte utf8.  Put in header
            output_char = TWO_BYTE_UTF8_START;

            // Fill remainder with as many bits (5) as we can
            data_working >>= (bits_left - 5);
            output_char |= (unsigned char) data_working;

            bits_left -= 5;
        }
        else
        {
            // Doesn't require utf8 encoding
            output.append(1, (char) data);

            ///
            return;
            ///
        }

        output.append(1, output_char);

        // Now add the remaining bits.  It is safe to assume everything is in
        // multiples of 6.
        //
        unsigned char data_working_char = 0;

        while (bits_left)
        {
            data_working = data;
            data_working >>= (bits_left - 6);

            // Put UTF8 continue header in front
            output_char = UTF8_CONTINUE;

            // Grab the next 6 bits from the data source
            data_working_char = (unsigned char) data_working;
            data_working_char &= ~UTF8_CONTINUE_SEARCH_MASK;

            // Combine the data with the header
            output_char |= data_working_char;

            // Add it to the output and loop
            output.append(1, output_char);
            data_working >>= 6;
            bits_left -= 6;
        }
    }

    // ----------------------------------------------------------------------
    unsigned char convert_bits_to_extended(
        const unsigned char first,
        const unsigned char second)
    {
        unsigned char result = INVALID_TO_SOCKET;
        u_int32_t data_working = 0;

        if (((first & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
             TWO_BYTE_UTF8_START) and
            ((second & UTF8_CONTINUE_SEARCH_MASK) ==
             UTF8_CONTINUE))
        {
            // Grab the data bytes out of the first part...
            data_working = (first & ~TWO_BYTE_UTF8_START_SEARCH_MASK);
            data_working <<= 6;

            // ...and then out of the second
            data_working |= (second & ~UTF8_CONTINUE_SEARCH_MASK);

            // See if it's within the range for extended ascii
            if ((data_working <= EXT_ASCII_END) and
                (data_working >= EXT_ASCII_BEGIN))
            {
                // And truncate the extra, unneeded bits
                result = (unsigned char) data_working;
            }
        }

        return result;
    }
}
}
