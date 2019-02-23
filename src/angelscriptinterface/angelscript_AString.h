/*
 * angelscript_AString.h
 */

#ifndef MUTGOS_ANGELSCRIPT_ASTRING_H
#define MUTGOS_ANGELSCRIPT_ASTRING_H

#include <stddef.h>
#include <string>

#include <angelscript.h>

#include "angelscript_SimpleGCObject.h"
#include "osinterface/osinterface_OsTypes.h"
#include "utilities/memory_VirtualHeapAllocator.h"

namespace mutgos
{
namespace angelscript
{
    /**
     * The custom string class (Angel String) used in AngelScript.  This class
     * makes use of the 'virtual' heap to limit how big the string can get,
     * and can restrict the use of certain characters.  It is also registered
     * as a reference type.
     *
     * The string is zero indexed, meaning the first character is at
     * position 0.
     *
     * This class is designed for use specifically while in the AngelScript
     * engine; it throws exceptions (such as AngelException) to indicate bad
     * inputs, virtual heap exhausted, or other error conditions.  Most
     * methods will throw an exception for index out of bounds, etc unless
     * noted.  Pointers returned are assumed to be managed by AngelScript,
     * and are registered with the garbage collector.
     *
     * Because this is specific to AngelScript, normal C++ code should not
     * call functions on this class; export to an std::string, perform
     * needed operations, then import the result.
     *
     * TODO UTF-8 support.
     * TODO Filtering of newlines, etc.
     * TODO Max string length, regardless of memory.
     * TODO Add 'complexity' metric to do better timesharing
     * TODO Useful post on how someone else did this class:  https://www.gamedev.net/forums/topic/639252-asobj-ref-and-asobj-value-at-the-same-time/?tab=comments#comment-5035942
     */
    class AString : public SimpleGCObject
    {
    public:
        /** Used for sizes of strings and positions within strings */
        typedef size_t StringPos;

        // Used to indicate a searched-for string is not found.
        static const StringPos NOT_FOUND;

        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * A factory used to create a new instance of an AString, equivalent
         * to the default constructor.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void string_factory(asIScriptGeneric *gen_ptr);

        /**
         * A factory used to create a copy of an AString, equivalent to the
         * copy constructor.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void string_factory_copy(asIScriptGeneric *gen_ptr);

        /**
         * Constructor that creates an empty string.
         * @param engine[in] The pointer to the script engine.
         * @param register_with_gc[in] True to register with garbage collector,
         * false to not register.
         */
        AString(asIScriptEngine *engine, const bool register_with_gc);

        /**
         * Constructor that creates an empty string and registers with
         * garbage collection.
         * @param engine[in] The pointer to the script engine.
         */
        AString(asIScriptEngine *engine);

        // NOTE:  There is purposely not a copy constructor, to avoid out of
        //        memory exceptions being thrown during construction and
        //        putting things in a strange state.

        /**
         * Constructor that creates a constant string, called only
         * by the string factory.  This does not register the instance
         * with the garbage collector.
         * @param engine[in] The script engine to register with.
         * @param data[in] The pointer to C style string data.
         * @param length[in] How many bytes 'data' is.
         */
        AString(
            asIScriptEngine *engine,
            const char *data,
            size_t length);

        /**
         * Destructor.
         */
        virtual ~AString();

        /**
         * Used by the string factory.
         * @return The string data as raw bytes.  Use size() to find out how
         * long the string is.
         */
        const char *get_raw_data(void) const;

        /**
         * Overwrites whatever is in the string with what's supplied.
         * @param str[in] The AString whose data is to be copied in.
         */
        void assign(const AString &str);

        /**
         * Overwrites whatever is in the string with what's supplied.
         * This can also be used as a sort of 'import' from std::string.
         * NOTE: This can throw exceptions if the virtual heap memory has
         * been exhausted.
         * @param str[in] The string whose data is to be copied in.
         * @throws exception An exception if the virtual heap memory is
         * exhausted.
         */
        void assign(const std::string &str);

        /**
         * Standard string assignment.
         * @param rhs[in] The source of the assignment.
         * @return This instance.
         */
        AString &operator=(const AString &rhs);

        /**
         * Puts the current string into an std::string, used for exporting
         * a string to other subsystems.
         * @return This string as an std::string.
         */
        std::string export_to_string(void) const;

        /**
         * Imports the given std::string into an AString.
         * NOTE: This can throw exceptions if the virtual heap memory has
         * been exhausted.
         * @param str[in] The string to copy in to this instance.
         * @throws exception An exception if the virtual heap memory is
         * exhausted.
         */
        void import_from_string(const std::string &str);

        /**
         * @return The number of characters in this string.
         */
        StringPos size(void) const;

        /**
         * @return True if size() == 0
         */
        bool empty(void) const;

        /**
         * Erases all of the string's contents.
         */
        void clear(void);

        /**
         * @param rhs[in] The other string to compare.
         * @return True if both strings are exactly equal.
         */
        bool operator==(const AString &rhs) const;

        /**
         * @param rhs[in] The other string to compare.
         * @return True if both strings are exactly equal.
         */
        bool operator<(const AString &rhs) const;

        /**
         * @param rhs[in] The other string to compare.
         * @return 0 for equal, < 0 if rhs has a lower mismatching character
         * or is shorter, or > 0 if rhs has a higher mismatching character
         * or is longer.
         */
        int compare(const AString &rhs) const;

        /**
         * Appends (concatonates) a string to the end of this one and returns
         * the result.  The original strings are not modified.
         * This is equivalent to operator+ within AngelScript.
         * @param rhs[in] The string to append.
         * @return A new string with this and rhs concatonated.
         */
        AString *concatonate(const AString &rhs) const;

        /**
         * Appends (concatonates) a string to the end of this one.  This string
         * will be modified.
         * @param rhs[in] The string to append.
         * @return This string with rhs appended.
         */
        AString &operator+=(const AString &rhs);

        /**
         * Appends (concatonates) a string to the end of this one.  This string
         * will be modified.
         * @param rhs[in] The string to append.
         */
        void append(const AString &rhs);

        /**
         * Appends (concatonates) the value to the end of this string and
         * returns the result, after converting it to a string.  The original
         * values are not modified.
         * This is equivalent to operator+ within AngelScript.
         * @param rhs[in] The value to append.
         * @return A new string with this string and rhs concatonated.
         */
        AString *concatonate(const MG_UnsignedInt rhs) const;

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         * @return This string with rhs appended.
         */
        AString &operator+=(const MG_UnsignedInt rhs);

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         */
        void append(const MG_UnsignedInt rhs);

        /**
         * Appends (concatonates) the value to the end of this string and
         * returns the result, after converting it to a string.  The original
         * values are not modified.
         * This is equivalent to operator+ within AngelScript.
         * @param rhs[in] The value to append.
         * @return A new string with this string and rhs concatonated.
         */
        AString *concatonate(const MG_LongUnsignedInt rhs) const;

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         * @return This string with rhs appended.
         */
        AString &operator+=(const MG_LongUnsignedInt rhs);

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         */
        void append(const MG_LongUnsignedInt rhs);

        /**
         * Appends (concatonates) the value to the end of this string and
         * returns the result, after converting it to a string.  The original
         * values are not modified.
         * This is equivalent to operator+ within AngelScript.
         * @param rhs[in] The value to append.
         * @return A new string with this string and rhs concatonated.
         */
        AString *concatonate(const bool rhs) const;

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         * @return This string with rhs appended.
         */
        AString &operator+=(const bool rhs);

        /**
         * Appends (concatonates) the value to the end of this string, after
         * converting it to a string.  This string will be modified.
         * @param rhs[in] The value to append.
         */
        void append(const bool rhs);

        /**
         * Appends multiple copies of the provided string onto the end of
         * this one.
         * @param str[in] The string to append.
         * @param repeats[in] How many types to append the string.
         */
        void fill(const AString &str, const StringPos repeats);

        /**
         * Finds the first instance of the given string inside of this one,
         * starting from the beginning.
         * @param str[in] The string to search for.
         * @return The position where str starts, or the NOT_FOUND constant
         * if the string is not found.
         */
        StringPos find(const AString &str) const;

        /**
         * Finds the first instance of the given string inside of this one,
         * starting where specified.
         * @param str[in] The string to search for.
         * @param pos[in] The position within this string to start searching.
         * @return The position where str starts, or the NOT_FOUND constant
         * if the string is not found.
         */
        StringPos find(const AString &str, const StringPos pos) const;

        /**
         * Finds the first instance of the given string inside of this one,
         * starting from the end.
         * @param str[in] The string to search for.
         * @return The position where str starts, or the NOT_FOUND constant
         * if the string is not found.
         */
        StringPos find_last(const AString &str) const;

        /**
         * Finds the first instance of the given string inside of this one,
         * starting where specified and working backwards.
         * @param str[in] The string to search for.
         * @param pos[in] The position within this string to start searching.
         * It will work backwards from this position.
         * @return The position where str starts, or the NOT_FOUND constant
         * if the string is not found.
         */
        StringPos find_last(const AString &str, const StringPos pos) const;

        /**
         * Creates a new string that starts at the given position and ends
         * at the end of this string.
         * @param start_pos[in] The starting position of the substring.
         * @return The substring.
         */
        AString *substring(const StringPos start_pos) const;

        /**
         * Creates a new string that starts at the given position and goes
         * on for the given length.  If the length is longer than remaining
         * characters of this string, it will stop at the end without
         * throwing an exception.
         * @param start_pos[in] The starting position of the substring.
         * @param len[in] How many characters are to be in the substring.
         * @return The substring.
         */
        AString *substring(
            const StringPos start_pos,
            const StringPos len) const;

        /**
         * @param pos[in] The position of the character to return.
         * @return The character at the given position.
         */
        AString *char_at(const StringPos pos) const;

        /**
         * Removes the end of the string, starting at the given position.
         * @param start_pos[in] The position to start erasing until the end
         * of the string.
         */
        void erase(const StringPos start_pos);

        /**
         * Removes a section of the string.  If the length is longer than
         * remains on the string, it will erase as much as it can without
         * throwing an exception.
         * @param start_pos[in] The starting position to erase.
         * @param len[in] How many characters to erase from the starting
         * position.
         */
        void erase(const StringPos start_pos, const StringPos len);

        /**
         * @param rhs[in] The other string to compare.
         * @return True if both strings are exactly equal.
         */
        bool equals(const AString &rhs) const;

        /**
         * @param rhs[in] The other string to compare.
         * @return True if both strings are equal even if they have
         * different cases.
         */
        bool equals_ignore_case(const AString &rhs) const;

        /**
         * @return A new string with all characters from this string in upper
         * case.
         */
        AString *to_upper(void) const;

        /**
         * @return A new string with all characters from this string in lower
         * case.
         */
        AString *to_lower(void) const;

        /**
         * Removes all whitespace at the beginning and end of the string,
         * modifying it.
         */
        void strip(void);

    private:
        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);

        // A string with the virtual heap allocator so its allocations
        // are tracked and restricted.
        typedef std::basic_string<
            char, std::char_traits<char>, memory::VirtualHeapAllocator<char>>
              ManagedString;

        ManagedString string_value; ///< The string value
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_ASTRING_H
