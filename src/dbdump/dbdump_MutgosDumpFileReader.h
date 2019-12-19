#ifndef MUTGOS_DBDUMP_MUTGOSDUMPFILEREADER_H
#define MUTGOS_DBDUMP_MUTGOSDUMPFILEREADER_H

#include <stddef.h>

#include <string>
#include <fstream>
#include <map>
#include <vector>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_SetProperty.h"
#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_Lock.h"
#include "dbdump_DumpReaderInterface.h"

namespace mutgos
{
namespace dbdump
{
    /**
     * A quick and dirty parser to initially get data into a MUTGOS database.
     * This will need substantial improvements at a later time.
     */
    class MutgosDumpFileReader
    {
    public:
        /**
         * Constructs a reader with the given file.
         * @param file_name[in] The file to parse.
         * @param base_path[in] The base path for includes (generally the
         * same directory as the dump file).
         */
        MutgosDumpFileReader(
            const std::string &file_name,
            const std::string &base_path);

        /**
         * Destructor.
         */
        ~MutgosDumpFileReader();

        /**
         * Parses the file specified in the constructor.  This can only be
         * called once.
         * @param message[out] The error or status message, if any.
         * @return True if no errors encountered during the parse.
         */
        bool parse(std::string &message);

        /**
         * @return The index (from 1) of the last line parsed.  Useful for
         * determining where an error occurred.  0 if error/no file.
         */
        size_t get_current_line_index(void) const;

        /**
         * @return The file name of the current file being parsed, or empty
         * if none/error.
         */
        std::string get_current_file(void) const;

        /**
         * @return The index (from 1) of the last line parsed of the next file
         * back in the stack.  Useful for determining where an error occurred
         * if a file is included in multiple places.  0 if error/no file.
         */
        size_t get_current_line_index_prev_file(void) const;

        /**
         * @return The file name of the next file in the stack being parsed,
         * or empty if none/error.
         */
        std::string get_prev_file(void) const;

    private:

        /**
         * Adds the given file to the file parsing stack if successfully
         * opened.  If there is an error, the file will not be added to the
         * stack.
         * @param file_name[in] The name of the file (including path if needed)
         * to add.
         * @return True if successfully added.  If false returned,
         * the error flag and message will have been set.
         */
        bool add_file_to_stack(const std::string &file_name);

        /**
         * Given a line from the file reader, determines what parser method
         * to call and calls it.
         * @param input[in] The input line to parse.
         */
        void parse_line(const std::string &input);

        /**
         * Parses anything related to sites and such
         * @param input[in] The input line to parse.
         */
        void parse_none(const std::string &input);

        /**
         * Parses anything related to top level parts of entities.
         * @param input[in] The input line to parse.
         */
        void parse_entity(const std::string &input);

        /**
         * Parses anything related to setting a security entry, for both Entities
         * and Applications.
         * @param input[in] The input line to parse.
         */
        void parse_security(const std::string &input);

        /**
         * Parses anything related to setting a field on an entity.
         * @param input[in] The input line to parse.
         */
        void parse_fields(const std::string &input);

        /**
         * Parses anything related to setting a property on an entity.
         * @param input[in] The input line to parse.
         */
        void parse_properties(const std::string &input);

        /**
         * Parses anything related to determining the type of lock.
         * @param input[in] The input line to parse.
         */
        void subparse_lock(const std::string &input);

        /**
         * Parses anything related to a lock by ID.
         * @param input[in] The input line to parse.
         */
        void subparse_lock_id(const std::string &input);

        /**
         * Parses anything related to a lock by property.
         * @param input[in] The input line to parse.
         */
        void subparse_lock_property(const std::string &input);

        /**
         * Parses anything related to setting a document value.
         * @param input[in] The input line to parse.
         */
        void subparse_document(const std::string &input);

        /**
         * Common code used to parse a document as a filed, property, or lock.
         * When document parsing has completed, current_document_ptr will
         * be the responsibility of the caller to use and then delete.
         * @param input[in] The input line to parse.
         * @return True if the document parsing has completed, false otherwise
         * (more lines needed).  set_error() will be called if any errors
         * occured, but this will not affect the return value (typically false).
         */
        bool shared_document_parser(const std::string &input);

        /**
         * Common code used to parse a set.
         * When set parsing has completed, current_set_ptr will be the
         * responsibility of the caller to use and then delete.
         * @param set_type[in] The type of the set items
         * @param input[in] The input line to parse.
         * @return True if the document parsing has completed, false otherwise
         * (more lines needed).  set_error() will be called if any errors
         * occured, but this will not affect the return value (typically false).
         */
        bool shared_set_parser(
            const dbtype::PropertyDataType &set_type,
            const std::string &input);

        /**
         * @return True if parser is handling multiline property data.
         */
        bool is_parsing_property_data() const
          { return current_document_ptr || current_set_ptr; }

        /**
         * Common code used to parse all property data and convert it to
         * the native format.
         * current_property will be updated if in the middle of parsing a
         * multiline value, and cleared when done.
         * current_document_ptr will be modified if parsing a document, and
         * cleared when done.
         * @param input[in] The input line to parse.
         * @param property_key[out] When returns true, this will be populated
         * with the key (property path).
         * @param value_ptr[out] When returns true, this will be populated
         * with the property data.  The caller is responsible for managing
         * the pointer.
         * @return True when done extracting the property name and value.
         * When true, the value_ptr becomes caller's to manage.  When returns
         * false, keep feeding it new input until it returns true.  If it
         * calls set_error() to indicate a problem, the return value is
         * not affected (typically false).
         */
        bool shared_property_parser(
            const std::string &input,
            std::string &property_key,
            dbtype::PropertyData *&value_ptr);

        /**
         * Sets the error flag and the cause.
         * @param cause[in] The reason the error flag was set.
         */
        void set_error(const std::string &cause);

        /**
         * Given a line, such as 'my_key=this value' Put my_key in key, and
         * 'this value' in value.
         * @param input[in] The input line to parse.  The line must already
         * be 'trimmed'.
         * @param key[out] The key extracted from input.
         * @param value[out] The value extracted from input.
         * @return True if successfully retrieved key and value.
         */
        bool get_key_value(
            const std::string &input,
            std::string &key,
            std::string &value) const;

        /**
         * Given a line, returns the first word, which is usually the command.
         * @param input[in,out] The input line to parse.  The line must already
         * be 'trimmed'.  It will be modified to no longer have the first word
         * in it, and trimmed.
         * @return The command (first word), or empty if none or error.
         */
        std::string get_word(std::string &input) const;

        /**
         * Does not check to see if variable exists.
         * @param input[in] The variable name to test.
         * @return True if input is a variable (in correct format).
         */
        bool is_variable(const std::string &input) const;

        /**
         * @param variable[in] The variable to get.
         * @return The ID represented by the variable, or default ID if not
         * found.
         */
        const dbtype::Id &get_variable(const std::string &variable) const;

        /**
         * Existing definitions are overwritten.
         * @param variable[in] The variable to set.
         * @param id[in] The ID to associate with the variable.
         * @return True if successfully set variable.
         */
        bool set_variable(const std::string &variable, const dbtype::Id &id);

        enum ParserMode
        {
            PARSER_NONE,       ///< No entity set
            PARSER_ENTITY,     ///< Entity set, not yet working on anything in it
            PARSER_SECURITY,   ///< Working on a security setting (entity or app)
            PARSER_FIELDS,     ///< Working on fields of Entity
            PARSER_PROPERTIES  ///< Working on properties of Entity
        };

        enum SubParserMode
        {
            SUBPARSER_NONE,          ///< Not working on a multi-part item.
            SUBPARSER_LOCK,          ///< Lock selected, not sure what type yet
            SUBPARSER_LOCK_ID,       ///< Working on lock by ID
            SUBPARSER_LOCK_PROPERTY, ///< Working on lock by property
            SUBPARSER_DOCUMENT       ///< Working on a document
        };

        /**
         * Container class that keeps track of a file being parsed and
         * information about it.
         */
        class FileStream
        {
        public:
            /**
             * Constructs and opens the file.
             * @param file[in] The full path of the file to open.
             */
            FileStream(const std::string &file)
              : current_line(0),
                file_name(file),
                stream(file.c_str())
            {
            }

            ~FileStream()
            {
            }

            /**
             * Increments the current line number.
             */
            void increment_line(void)
            {
                ++current_line;
            }

            size_t current_line;  // Now many lines have been parsed so far
            const std::string file_name; // Name of the file being parsed
            std::ifstream stream; // Stream for file being parsed
        };

        /** Maps dynamic variable name to ID */
        typedef std::map<std::string, dbtype::Id> VariableMap;

        bool error_condition;  // True if error and parser needs to stop
        bool file_parsed;       // True if file completed parsing.
        std::string status_message; // Message to be passed back to class caller
        ParserMode parser_mode;  // Mode the parser is in
        SubParserMode subparser_mode; // Mode the subparser is in, if any

        dbtype::Id::SiteIdType current_site; // Site being worked on
        dbtype::Id current_id;         // ID of the Entity being worked on

        std::string current_property;  // Current property, or empty if none
        dbtype::EntityField current_entity_field; // Current field.

        dbtype::DocumentProperty *current_document_ptr; // Doc being worked on, or null

        unsigned int items_left; // When inputting a doc or set, how many lines/items to go

        dbtype::PropertyDataType current_set_type; // Data type of set items
        dbtype::SetProperty *current_set_ptr; // Set being worked on, if non-null

        bool operation_not; // True if current operation is to be 'not'ed.

        VariableMap variables; // Maps variable name (without $) to ID.

        const dbtype::Id default_id; // Used to indicate not found

        DumpReaderInterface db; // Interface to write to the db
        std::vector<FileStream *> file_stack; // The files being processed, as a stack.  Last is latest.
        const std::string base_file_path; // Base path for stream includes
    };
}
}

#endif
