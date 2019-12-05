/*
 * text_Utf8Tools.cpp
 */

#include <string>
#include <cstring>

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

    // ----------------------------------------------------------------------
    bool utf8_valid(const std::string &str)
    {
        return utf8_valid(str.c_str(), str.size());
    }

    // ----------------------------------------------------------------------
    bool utf8_valid(const char *str, const size_t len)
    {
        bool valid = true;

        // Null and empty checks.
        //
        if (not str)
        {
            return false;
        }
        else if (not len)
        {
            return true;
        }

        size_t index = 0;
        unsigned short utf8_bytes_left = 0;

        while (index < len)
        {
            const unsigned char current_char = (unsigned char) str[index];

            if (utf8_bytes_left)
            {
                // In the middle of UTF8.  See if this is a 'continue' byte.
                //
                if ((current_char & UTF8_CONTINUE_SEARCH_MASK) ==
                    UTF8_CONTINUE)
                {
                    --utf8_bytes_left;
                }
                else
                {
                    // Not a valid continue.  Invalid UTF8
                    break;
                }
            }
            else if (current_char < PRINTABLE_ASCII_BEGIN)
            {
                // Not printable; invalid.
                valid = false;
                break;
            }
            else if (current_char > PRINTABLE_ASCII_END)
            {
                // Not normal ASCII, so check for UTF8.
                //
                if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                    TWO_BYTE_UTF8_START)
                {
                    // Potentially valid 2 byte character.  Check it.
                    utf8_bytes_left = 1;
                }
                else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                         THREE_BYTE_UTF8_START)
                {
                    // Potentially valid 3 byte character.  Check it.
                    utf8_bytes_left = 2;
                }
                else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                         FOUR_BYTE_UTF8_START)
                {
                    // Potentially valid 4 byte character.  Check it.
                    utf8_bytes_left = 3;
                }
                else
                {
                    // Unknown what this is.  Invalid.
                    valid = false;
                    break;
                }
            }

            ++index;
        }

        if (utf8_bytes_left)
        {
            // We ended in the middle of UTF8.  Always invalid.
            valid = false;
        }

        return valid;
    }

    // ----------------------------------------------------------------------
    size_t utf8_size(const std::string &str)
    {
        return utf8_size(str.c_str(), str.size());
    }

    // ----------------------------------------------------------------------
    size_t utf8_size(const char *str, const size_t len)
    {
        size_t size = 0;

        // Null and empty checks.
        //
        if ((not str) or (not len))
        {
            return size;
        }

        size_t index = 0;
        unsigned short utf8_bytes_left = 0;

        while (index < len)
        {
            const unsigned char current_char = (unsigned char) str[index];

            if (utf8_bytes_left)
            {
                // In the middle of UTF8.  See if this is a 'continue' byte.
                // The entry to the code point already counts as the
                // element for size.
                //
                if ((current_char & UTF8_CONTINUE_SEARCH_MASK) ==
                    UTF8_CONTINUE)
                {
                    --utf8_bytes_left;
                }
                else
                {
                    // Not a valid continue.  Invalid UTF8
                    size = 0;
                    break;
                }
            }
            else if (current_char < PRINTABLE_ASCII_BEGIN)
            {
                // Not printable; invalid.
                size = 0;
                break;
            }
            else if (current_char > PRINTABLE_ASCII_END)
            {
                // Not normal ASCII, so check for UTF8.
                //
                if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                    TWO_BYTE_UTF8_START)
                {
                    // Potentially valid 2 byte character.  Check it.
                    utf8_bytes_left = 1;
                    ++size;
                }
                else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                         THREE_BYTE_UTF8_START)
                {
                    // Potentially valid 3 byte character.  Check it.
                    utf8_bytes_left = 2;
                    ++size;
                }
                else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                         FOUR_BYTE_UTF8_START)
                {
                    // Potentially valid 4 byte character.  Check it.
                    utf8_bytes_left = 3;
                    ++size;
                }
                else
                {
                    // Unknown what this is.  Invalid.
                    size = 0;
                    break;
                }
            }
            else
            {
                // Valid, normal ASCII.  Counts as one element.
                ++size;
            }

            ++index;
        }

        if (utf8_bytes_left)
        {
            // We ended in the middle of UTF8.  Always invalid.
            size = 0;
        }

        return size;
    }

    // ----------------------------------------------------------------------
    size_t utf8_index_to_byte(const std::string &str, const size_t utf8_index)
    {
        return utf8_index_to_byte(str.c_str(), str.size(), utf8_index);
    }

    // ----------------------------------------------------------------------
    size_t utf8_index_to_byte(
        const char *str,
        const size_t len,
        const size_t utf8_index)
    {
        size_t byte = 0;

        if ((not str) or (not len))
        {
            return byte;
        }

        if (not utf8_index)
        {
            return byte;
        }

        size_t current_index = 0;

        while ((byte < len) and (current_index < utf8_index))
        {
            const unsigned char current_char = (unsigned char) str[byte];

            if (current_char > PRINTABLE_ASCII_END)
            {
                // Not normal ASCII, so check for UTF8.
                //
                if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                    TWO_BYTE_UTF8_START)
                {
                    // 2 byte character.  Skip over.
                    byte += 2;
                }
                else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                         THREE_BYTE_UTF8_START)
                {
                    // 3 byte character. Skip over.
                    byte += 3;
                }
                else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                         FOUR_BYTE_UTF8_START)
                {
                    // 4 byte character.  Skip over.
                    byte += 4;
                }
                else
                {
                    // Unknown what this is.  Invalid.
                    byte = 0;
                    break;
                }
            }
            else
            {
                // Valid, normal ASCII.  Counts as one element.
                ++byte;
            }

            ++current_index;
        }

        if (current_index != utf8_index)
        {
            // Internal, unexpected error occurred.
            byte = 0;
        }

        return byte;
    }

    // ----------------------------------------------------------------------
    size_t utf8_byte_to_index(const std::string &str, const size_t byte)
    {
        return utf8_byte_to_index(str.c_str(), str.size(), byte);
    }

    // ----------------------------------------------------------------------
    size_t utf8_byte_to_index(
        const char *str,
        const size_t len,
        const size_t byte)
    {
        size_t utf8_index = 0;

        if ((not str) or (not len))
        {
            return byte;
        }

        if (not byte)
        {
            return byte;
        }

        size_t current_byte = 0;

        while ((current_byte < len) && (current_byte < byte))
        {
            const unsigned char current_char =
                (unsigned char) str[current_byte];

            if (current_char > PRINTABLE_ASCII_END)
            {
                // Not normal ASCII, so check for UTF8.
                //
                if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                    TWO_BYTE_UTF8_START)
                {
                    // 2 byte character.  Skip over.
                    current_byte += 2;
                }
                else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                         THREE_BYTE_UTF8_START)
                {
                    // 3 byte character. Skip over.
                    current_byte += 3;
                }
                else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                         FOUR_BYTE_UTF8_START)
                {
                    // 4 byte character.  Skip over.
                    current_byte += 4;
                }
                else
                {
                    // Unknown what this is.  Invalid.
                    utf8_index = 0;
                    break;
                }
            }
            else
            {
                // Valid, normal ASCII.  Counts as one element.
                ++current_byte;
            }

            ++utf8_index;
        }

        if (current_byte < byte)
        {
            // Internal, unexpected error occurred.
            utf8_index = 0;
        }
        else if (current_byte > byte)
        {
            // We overshot and are still within the previous code point.
            --utf8_index;
        }

        return utf8_index;
    }

    // ----------------------------------------------------------------------
    size_t utf8_find(
        const std::string &str,
        const std::string &to_find,
        const size_t start)
    {
        return utf8_find(
            str.c_str(),
            str.size(),
            to_find.c_str(),
            to_find.size(),
            start);
    }

    // ----------------------------------------------------------------------
    size_t utf8_find(
        const char *str,
        const size_t len,
        const char *to_find,
        const size_t to_find_len,
        const size_t start)
    {
        size_t found_index = std::string::npos;

        if ((not str) or (not to_find) or (not len) or (not to_find_len) or
            (to_find_len > len))
        {
            // Strings are empty, null, or the string to find is bigger than
            // the string to search.
            return found_index;
        }

        // First find the starting byte
        const size_t start_byte_index = utf8_index_to_byte(str, len, start);

        if (start and (not start_byte_index))
        {
            // Error returned while looking for starting byte (beyond end?), so
            // return error.
            return found_index;
        }

        // Do the find.
        //
        size_t found_byte_index = std::string::npos;
        size_t current_byte_index = start_byte_index;

        while (current_byte_index <= (len - to_find_len))
        {
            const void * const found_byte_ptr = memchr(
                &str[current_byte_index],
                to_find[0],
                len - current_byte_index);

            if (found_byte_ptr == NULL)
            {
                // No chance of substring being here.  Exit now.
                break;
            }
            else
            {
                // Convert pointer to byte offset
                //
                current_byte_index = ((const char *) found_byte_ptr) - str;

                if (to_find_len == 1)
                {
                    // Single character search, so we've found the match.
                    found_byte_index = current_byte_index;
                    break;
                }
                else
                {
                    // Chance of finding substring.  Search for it.
                    //
                    if (memcmp(
                        &str[current_byte_index + 1],
                        &to_find[1],
                        to_find_len - 1) == 0)
                    {
                        // Found substring!
                        found_byte_index = current_byte_index;
                        break;
                    }
                    else
                    {
                        // Did not find it, so advance one and try again.
                        ++current_byte_index;
                    }
                }
            }
        }

        if (found_byte_index != std::string::npos)
        {
            // Convert back to UTF8 index and return
            found_index = utf8_byte_to_index(str, len, found_byte_index);
        }

        return found_index;
    }

    // ----------------------------------------------------------------------
    size_t utf8_find_last(
        const std::string &str,
        const std::string &to_find)
    {
        return utf8_find_last(str.c_str(), str.size(), to_find.c_str(),
            to_find.size());
    }

    // ----------------------------------------------------------------------
    size_t utf8_find_last(
        const char *str,
        const size_t len,
        const char *to_find,
        const size_t to_find_len)
    {
        size_t found_index = std::string::npos;
        const static size_t MAX_INDEX = static_cast<size_t>(-1);

        if ((not str) or (not to_find) or (not len) or (not to_find_len) or
            (to_find_len > len))
        {
            // Strings are empty, null, or the string to find is bigger than
            // the string to search.
            return found_index;
        }

        // Do the find.
        //
        size_t found_byte_index = std::string::npos;
        size_t current_byte_index = len - to_find_len;

        while (current_byte_index != MAX_INDEX)
        {
            const void * const found_byte_ptr = memrchr(
                str,
                to_find[0],
                current_byte_index);

            if (found_byte_ptr == NULL)
            {
                // No chance of substring being here.  Exit now.
                break;
            }
            else
            {
                // Convert pointer to byte offset
                //
                current_byte_index = ((const char *) found_byte_ptr) - str;

                if (to_find_len == 1)
                {
                    // Single character search, so we've found the match.
                    found_byte_index = current_byte_index;
                    break;
                }
                else
                {
                    // Chance of finding substring.  Search for it.
                    //
                    if (memcmp(
                        &str[current_byte_index + 1],
                        &to_find[1],
                        to_find_len - 1) == 0)
                    {
                        // Found substring!
                        found_byte_index = current_byte_index;
                        break;
                    }
                    else
                    {
                        // Did not find it, so go back one and try again.
                        --current_byte_index;
                    }
                }
            }
        }

        if (found_byte_index != std::string::npos)
        {
            // Convert back to UTF8 index and return
            found_index = utf8_byte_to_index(str, len, found_byte_index);
        }

        return found_index;
    }

    // ----------------------------------------------------------------------
    std::string utf8_char_at(const std::string &str, const size_t utf8_index)
    {
        return utf8_char_at(str.c_str(), str.size(), utf8_index);
    }

    // ----------------------------------------------------------------------
    std::string utf8_char_at(
        const char *str,
        const size_t len,
        const size_t utf8_index)
    {
        std::string found_char;

        if ((not str) or (not len))
        {
            return found_char;
        }

        const size_t byte_index = utf8_index_to_byte(str, len, utf8_index);

        if (utf8_index and (not byte_index))
        {
            // Error returned while looking for index (beyond end?), so
            // return error.
            return found_char;
        }

        // Determine if this is a UTF8 character, and if so extract all bytes.
        //
        const unsigned char current_char =
            (unsigned char) str[byte_index];

        if (current_char > PRINTABLE_ASCII_END)
        {
            // Not normal ASCII, so check for UTF8.
            //
            if ((current_char & TWO_BYTE_UTF8_START_SEARCH_MASK) ==
                TWO_BYTE_UTF8_START)
            {
                // 2 byte character.
                found_char.assign(&str[byte_index], 2);
            }
            else if ((current_char & THREE_BYTE_UTF8_START_SEARCH_MASK) ==
                     THREE_BYTE_UTF8_START)
            {
                // 3 byte character.
                found_char.assign(&str[byte_index], 3);
            }
            else if ((current_char & FOUR_BYTE_UTF8_START_SEARCH_MASK) ==
                     FOUR_BYTE_UTF8_START)
            {
                // 4 byte character.
                found_char.assign(&str[byte_index], 4);
            }
        }
        else
        {
            // Valid, normal ASCII. Just extract the one character.
            found_char.assign(1, current_char);
        }

        return found_char;
    }

    // ----------------------------------------------------------------------
    std::string utf8_chop_at_limit(
        const std::string &str,
        const size_t utf8_size)
    {
        return utf8_chop_at_limit(str.c_str(), str.size(), utf8_size);
    }

    // ----------------------------------------------------------------------
    std::string utf8_chop_at_limit(
        const char *str,
        const size_t len,
        const size_t utf8_size)
    {
        std::string result;

        if ((not str) or (not len) or (not utf8_size))
        {
            return result;
        }

        const size_t byte_index = utf8_index_to_byte(str, len, utf8_size);

        if (utf8_size and (not byte_index))
        {
            // Likely utf8_size is bigger than string, so return everything.
            result.assign(str, len);
        }
        else
        {
            // Not past the limit, so use up to the index.
            result.assign(str, byte_index);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void utf8_chop_modify_at_limit(
        std::string &str,
        const size_t utf8_size)
    {
        if (not utf8_size)
        {
            str.clear();
            str.shrink_to_fit();
        }

        const size_t byte_index = utf8_index_to_byte(str, utf8_size);

        if (not (utf8_size and (not byte_index)))
        {
            // Chop off end of string since it's too long.
            str.erase(byte_index);
        }
    }

    // ----------------------------------------------------------------------
    void utf8_split(
        const std::string &str,
        const size_t utf8_index,
        std::string &before_str,
        std::string &after_str)
    {
        utf8_split(str.c_str(), str.size(), utf8_index, before_str, after_str);
    }

    // ----------------------------------------------------------------------
    void utf8_split(
        const char *str,
        const size_t len,
        const size_t utf8_index,
        std::string &before_str,
        std::string &after_str)
    {
        if ((not str) or (not len))
        {
            // Invalid string.  Return nothing.
            //
            before_str.clear();
            before_str.shrink_to_fit();
            after_str.clear();
            after_str.shrink_to_fit();
            return;
        }

        if (not utf8_index)
        {
            // Easy case where everything goes to 'after'.
            before_str.clear();
            before_str.shrink_to_fit();
            after_str.assign(str, len);
        }
        else
        {
            const size_t byte_index = utf8_index_to_byte(str, len, utf8_index);

            if (not byte_index)
            {
                // Likely utf8_index is bigger than string, so return everything
                // in 'before'.
                before_str.assign(str, len);
                after_str.clear();
                after_str.shrink_to_fit();
            }
            else
            {
                before_str.assign(str, byte_index);
                after_str.assign(&str[byte_index], len - byte_index);
            }
        }
    }
}
}
