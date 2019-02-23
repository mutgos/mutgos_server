/*
 * dbdump_MutgosDumpFileReader.cpp
 */

#include <stddef.h>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "text/text_StringConversion.h"

#include "dbdump_MutgosDumpFileReader.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_IdProperty.h"
#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_Lock.h"


#define VAR_PREFIX '$'
#define COMMENT_PREFIX '#'

namespace mutgos
{
namespace dbdump
{
    // ----------------------------------------------------------------------
    MutgosDumpFileReader::MutgosDumpFileReader(const std::string &file_name)
      : error_condition(false),
        file_parsed(false),
        current_line(0),
        parser_mode(MutgosDumpFileReader::PARSER_NONE),
        subparser_mode(MutgosDumpFileReader::SUBPARSER_NONE),
        current_site(0),
        current_entity_field(dbtype::ENTITYFIELD_invalid),
        current_document_ptr(0),
        items_left(0),
        current_set_type(dbtype::PROPERTYDATATYPE_invalid),
        current_set_ptr(0),
        operation_not(false),
        stream_ptr(0)
    {
        if (file_name.empty())
        {
            set_error("The file name provided was empty.");
        }
        else
        {
            stream_ptr = new std::ifstream(file_name.c_str());

            if ((not stream_ptr->good()) or (not stream_ptr->is_open()))
            {
                set_error("The file " + file_name + " cannot be read.");
            }
        }
    }

    // ----------------------------------------------------------------------
    MutgosDumpFileReader::~MutgosDumpFileReader()
    {
        if (stream_ptr)
        {
            stream_ptr->close();
            delete stream_ptr;
            stream_ptr = 0;
        }

        delete current_document_ptr;
        current_document_ptr = 0;

        delete current_set_ptr;
        current_set_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::parse(std::string &message)
    {
        bool result = true;
        message.clear();

        if (error_condition)
        {
            // File was already bad, just return error and stop.
            result = false;
            message = status_message;
        }
        else
        {
            std::string line;

            // Confirm version
            //
            ++current_line;
            std::getline(*stream_ptr, line);
            boost::trim(line);

            if (line != "MUTGOS DUMP VERSION 1")
            {
                // Not a dump file.
                set_error("Not a MUTGOS version 1 dump file!");
            }

            // Parse the file line by line
            //
            while ((not file_parsed) and (not error_condition))
            {
                ++current_line;
                std::getline(*stream_ptr, line);

                if (not stream_ptr->good())
                {
                    file_parsed = true;
                }
                else if (line == "MUTGOS DUMP END")
                {
                    file_parsed = true;
                }
                else
                {
                    if (not current_document_ptr)
                    {
                        boost::trim(line);
                    }

                    parse_line(line);
                }
            }

            // Parsing completed.  Determine if there was an error and if the
            // file was complete.
            //
            if (not error_condition)
            {
                if ((parser_mode == PARSER_NONE) and
                    (subparser_mode == SUBPARSER_NONE))
                {
                    message = "Parsing completed successfully.";
                }
                else
                {
                    result = false;
                    message = "Parsing error: File is incomplete, not "
                        "properly closed, or in the wrong mode to close.";
                }
            }
            else
            {
                result = false;
                message = status_message;
                db.set_error();
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_line(const std::string &input)
    {
        if ((! input.empty()) and (input[0] != COMMENT_PREFIX))
        {
            if (subparser_mode != SUBPARSER_NONE)
            {
                // In the middle of a multi-line item, so let those parsers
                // handle it.
                //
                switch (subparser_mode)
                {
                    case SUBPARSER_LOCK:
                    {
                        subparse_lock(input);
                        break;
                    }

                    case SUBPARSER_LOCK_ID:
                    {
                        subparse_lock_id(input);
                        break;
                    }

                    case SUBPARSER_LOCK_PROPERTY:
                    {
                        subparse_lock_property(input);
                        break;
                    }

                    case SUBPARSER_DOCUMENT:
                    {
                        subparse_document(input);
                        break;
                    }

                    default:
                    {
                        set_error("Unknown subparser mode!");
                    }
                }
            }
            else
            {
                // Standard line
                //
                switch (parser_mode)
                {
                    case PARSER_NONE:
                    {
                        parse_none(input);
                        break;
                    }

                    case PARSER_ENTITY:
                    {
                        parse_entity(input);
                        break;
                    }

                    case PARSER_SECURITY:
                    {
                        parse_security(input);
                        break;
                    }

                    case PARSER_FIELDS:
                    {
                        parse_fields(input);
                        break;
                    }

                    case PARSER_PROPERTIES:
                    {
                        parse_properties(input);
                        break;
                    }

                    default:
                    {
                        set_error("Unknown parser mode!");
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_none(const std::string &input)
    {
        // Supports these commands:
        // mksite
        // setsite
        // mkentity
        // modify
        // end site

        std::string parsed_input = input;
        const std::string command =
            boost::to_lower_copy(get_word(parsed_input));

        if (command == "mksite")
        {
            if (not db.make_site(parsed_input, current_site))
            {
                set_error("Unable to make site " + parsed_input);
            }
        }
        else if (command == "setsite")
        {
            dbtype::Id::SiteIdType parsed_site_id = 0;

            if (not text::from_string<dbtype::Id::SiteIdType>(
                parsed_input,
                parsed_site_id))
            {
                set_error("Cannot convert site ID: " + parsed_input);
            }
            else
            {
                if (not db.set_site(parsed_site_id))
                {
                    set_error("Unable to set site " + parsed_input);
                }
            }
        }
        else if (command == "mkentity")
        {
            const std::string entity_type_str =
                boost::to_lower_copy(get_word(parsed_input));
            const dbtype::EntityType entity_type =
                dbtype::string_to_entity_type(entity_type_str);

            current_id = db.make_entity(entity_type);

            if (current_id.is_default())
            {
                set_error("mkentity: Could not make Entity with type "
                          + entity_type_str);
            }
            else
            {
                parser_mode = PARSER_ENTITY;

                // Made entity, now see if we need to store it in the lookup
                if (not parsed_input.empty())
                {
                    if (not set_variable(parsed_input, current_id))
                    {
                        set_error("mkentity: unable to set variable "
                            + parsed_input
                            + ".  Wrong format?");
                    }
                }
            }
        }
        else if (command == "modentity")
        {
            if (parsed_input.empty())
            {
                set_error("modentity: missing variable");
            }
            else
            {
                if (not is_variable(parsed_input))
                {
                    set_error("modentity: invalid variable declaration "
                              + parsed_input);
                }
                else
                {
                    const dbtype::Id &var_id = get_variable(parsed_input);

                    if (var_id.is_default())
                    {
                        set_error("modentity: variable does not exist: "
                                  + parsed_input);
                    }
                    else
                    {
                        if (db.set_entity(var_id))
                        {
                            parser_mode = PARSER_ENTITY;
                        }
                        else
                        {
                            set_error("modentity: unable to set Entity "
                              + var_id.to_string(true));
                        }
                    }
                }
            }
        }
        else if (command == "end")
        {
            boost::to_lower(parsed_input);

            if (parsed_input == "site")
            {
                if (not db.end_site())
                {
                    set_error("end site: Wrong mode to end site!");
                }
                else
                {
                    current_site = 0;
                }
            }
            else
            {
                set_error("end: Invalid end command for this mode: "
                          + parsed_input);
            }
        }
        else
        {
            set_error("Unknown command: " + command);
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_entity(const std::string &input)
    {
        // Supports these commands:
        // print
        // owner
        // name
        // flag
        // security
        // fields
        // properties
        // end entity
        //

        std::string parsed_input = input;
        const std::string command =
            boost::to_lower_copy(get_word(parsed_input));

        if (command == "print")
        {
            db.log_entity();
        }
        else if (command == "owner")
        {
            if (not is_variable(parsed_input))
            {
                set_error("entity owner: invalid variable reference");
            }
            else
            {
                const dbtype::Id &owner = get_variable(parsed_input);

                if (owner.is_default())
                {
                    set_error("entity owner: cannot find variable "
                              + parsed_input);
                }
                else
                {
                    if (not db.set_entity_owner(owner))
                    {
                        set_error("entity owner: cannot set");
                    }
                }
            }
        }
        else if (command == "name")
        {
            if (parsed_input.empty())
            {
                set_error("entity name: Missing name");
            }
            else if (not db.set_entity_name(parsed_input))
            {
                set_error("entity name: Unable to set!");
            }
        }
        else if (command == "flag")
        {
            if (parsed_input.empty())
            {
                set_error("entity name: Missing flag");
            }
            else if (not db.add_entity_flag(parsed_input))
            {
                set_error("entity flag: Unable to add flag!");
            }
        }
        else if (command == "security")
        {
            if (db.set_entity_security())
            {
                parser_mode = PARSER_SECURITY;
            }
            else
            {
                set_error("entity security:  Unable to set mode!");
            }
        }
        else if (command == "fields")
        {
            parser_mode = PARSER_FIELDS;
        }
        else if (command == "properties")
        {
            // TODO Actually confirm entity can support properties
            parser_mode = PARSER_PROPERTIES;
        }
        else if (command == "end")
        {
            boost::to_lower(parsed_input);

            if (parsed_input == "entity")
            {
                if (not db.end_entity())
                {
                    set_error("end entity:  Wrong mode to end Entity.");
                }
                else
                {
                    current_id = default_id;
                    parser_mode = PARSER_NONE;
                }
            }
            else
            {
                set_error("entity: Unknown mode to end: " + parsed_input);
            }
        }
        else
        {
            set_error("entity: Unknown command: " + command);
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_security(const std::string &input)
    {
        // Supports these commands:
        // group
        // admin
        // flag  (group, other)
        // end security
        //

        std::string parsed_input = input;
        const std::string command =
            boost::to_lower_copy(get_word(parsed_input));

        if (command == "group")
        {
            if (not is_variable(parsed_input))
            {
                set_error("security group: invalid variable reference");
            }
            else
            {
                const dbtype::Id &group = get_variable(parsed_input);

                if (group.is_default())
                {
                    set_error("security group: cannot find variable "
                              + parsed_input);
                }
                else
                {
                    if (not db.add_to_security_group(group))
                    {
                        set_error("security group: cannot add");
                    }
                }
            }
        }
        else if (command == "admin")
        {
            if (not is_variable(parsed_input))
            {
                set_error("security admin: invalid variable reference");
            }
            else
            {
                const dbtype::Id &admin = get_variable(parsed_input);

                if (admin.is_default())
                {
                    set_error("security admin: cannot find variable "
                              + parsed_input);
                }
                else
                {
                    if (not db.add_to_security_admins(admin))
                    {
                        set_error("security admin: cannot add");
                    }
                }
            }
        }
        else if (command == "flag")
        {
            boost::to_lower(parsed_input);
            const std::string flag_list = get_word(parsed_input);
            dbtype::SecurityFlag parsed_flag =
                dbtype::Security::security_flag_from_string(parsed_input);

            if (parsed_flag == dbtype::SECURITYFLAG_invalid)
            {
                set_error("security flag: Unknown flag " + parsed_input);
            }
            else
            {
                if (flag_list == "group")
                {
                    if (not db.add_security_flag_list(parsed_flag))
                    {
                        set_error("security flag set group: Could not set flag "
                                  + parsed_input);
                    }
                }
                else if (flag_list == "other")
                {
                    if (not db.add_security_flag_other(parsed_flag))
                    {
                        set_error("security flag set other: Could not set flag "
                                  + parsed_input);
                    }
                }
                else
                {
                    set_error("security flag set: Unknown flag list "
                              + flag_list);
                }
            }
        }
        else if (command == "end")
        {
            boost::to_lower(parsed_input);

            if (parsed_input == "security")
            {
                if (not db.end_security())
                {
                    set_error("end security:  Wrong mode to end Security");
                }
                else
                {
                    if (current_property.empty())
                    {
                        parser_mode = PARSER_ENTITY;
                    }
                    else
                    {
                        parser_mode = PARSER_PROPERTIES;
                    }
                }
            }
            else
            {
                set_error("security: Unknown mode to end: " + parsed_input);
            }
        }
        else
        {
            set_error("security: Unknown command: " + command);
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_fields(const std::string &input)
    {
        std::string field_string;
        std::string value;

        if (not get_key_value(input, field_string, value))
        {
            // Not a key/value, see if it's an end
            //
            std::string parsed_input = boost::to_lower_copy(input);
            const std::string command = get_word(parsed_input);

            if ((command == "end") and (parsed_input == "fields"))
            {
                parser_mode = PARSER_ENTITY;
            }
            else
            {
                set_error("entity field: Malformed input: " + input);
            }
        }
        else
        {
            boost::to_lower(field_string);
            const dbtype::EntityField field =
                dbtype::string_to_entity_field(field_string);

            if (field == dbtype::ENTITYFIELD_invalid)
            {
                set_error("entity field:  Unknown field " + field_string);
            }
            else
            {
                current_entity_field = field;

                switch (db.which_set_field_method(field))
                {
                    // String fields
                    //
                    case DumpReaderInterface::METHOD_string:
                    case DumpReaderInterface::METHOD_string_multiple:
                    {
                        // Field is a string type
                        //
                        if (not db.set_entity_field(field, value))
                        {
                            set_error("entity field (string): Cannot set field "
                                      + field_string + " to " + value);
                        }

                        break;
                    }

                    // ID fields
                    //
                    case DumpReaderInterface::METHOD_id:
                    case DumpReaderInterface::METHOD_id_multiple:
                    {
                        if (not is_variable(value))
                        {
                            set_error(
                                "entity field (id): Value is not a variable");
                        }
                        else
                        {
                            const dbtype::Id &id = get_variable(value);

                            if (id.is_default())
                            {
                                set_error("entity field (id): Variable does "
                                  "not exist: " + value);
                            }
                            else
                            {
                                if (not db.set_entity_field(field, id))
                                {
                                    set_error("entity field (id):  Unable to "
                                      "set ID " + id.to_string(true));
                                }
                            }
                        }

                        break;
                    }

                    // Document fields
                    //
                    case DumpReaderInterface::METHOD_document:
                    {
                        // Going into a subparser.  Set the subparser mode,
                        // then call it with the value.
                        //
                        subparser_mode = SUBPARSER_DOCUMENT;
                        subparse_document(value);
                        break;
                    }

                    // Lock fields
                    //
                    case DumpReaderInterface::METHOD_lock:
                    {
                        subparser_mode = SUBPARSER_LOCK;
                        subparse_lock(value);
                        break;
                    }

                    // Invalid
                    //
                    case DumpReaderInterface::METHOD_invalid:
                    {
                        set_error("entity field: Invalid field for being set: "
                          + field_string);
                        break;
                    }

                    // Unknown
                    //
                    default:
                    {
                        set_error("entity field: Unknown field type "
                            "(internal error).");
                        break;
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::parse_properties(const std::string &input)
    {
        std::string parsed_input = input;
        const std::string command =
            boost::to_lower_copy(get_word(parsed_input));
        std::string property_path;
        dbtype::PropertyData *property_data_ptr = 0;

        // Handle a command if not currently in the middle of parsing a
        // multiline property, or parse a new property.
        //
        if ((not is_parsing_property_data()) and command == "security")
        {
            // Extract the application name and owner variable
            const std::string application = get_word(parsed_input);

            if (application.empty() or (not is_variable(parsed_input)))
            {
                set_error("properties: application name empty or no "
                              "variable for owner: " + input);
            }
            else
            {
                const dbtype::Id &id = get_variable(parsed_input);

                if (id.is_default())
                {
                    set_error("properties: Cannot find variable for owner: "
                              + parsed_input);
                }
                else
                {
                    if (db.add_application(application, id) and
                        db.set_application_props_security(application))
                    {
                        // Added application, parse security parameters
                        //
                        current_property = application;
                        parser_mode = PARSER_SECURITY;
                    }
                    else
                    {
                        set_error("properties: Unable to add application or "
                                      "set security for " + application);
                    }
                }
            }
        }
        else if ((not is_parsing_property_data()) and command == "end")
        {
            boost::to_lower(parsed_input);

            if (parsed_input == "properties")
            {
                parser_mode = PARSER_ENTITY;
                current_property.clear();
            }
            else
            {
                set_error("properties: Invalid end command for this "
                              "mode: " + parsed_input);
            }
        }
        else
        {
            // A property to parse, or one already in progress.
            //
            if (shared_property_parser(input, property_path, property_data_ptr))
            {
                // Completed parsing, set it.
                if (not db.set_prop(property_path, *property_data_ptr))
                {
                    set_error("properties: Unable to set property "
                              + property_path);
                }

                delete property_data_ptr;
                property_data_ptr = 0;
            }
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::subparse_lock(const std::string &input)
    {
        // supports only two commands, basically a brancher:
        // id (lock by)
        // property (lock by)
        //
        const std::string &parsed_input = boost::to_lower_copy(input);

        if (parsed_input == "id")
        {
            subparser_mode = SUBPARSER_LOCK_ID;
            operation_not = false;
        }
        else if (parsed_input == "!id")
        {
            subparser_mode = SUBPARSER_LOCK_ID;
            operation_not = true;
        }
        else if (parsed_input == "property")
        {
            subparser_mode = SUBPARSER_LOCK_PROPERTY;
            operation_not = false;
        }
        else if (parsed_input == "!property")
        {
            subparser_mode = SUBPARSER_LOCK_PROPERTY;
            operation_not = true;
        }
        else
        {
            set_error("lock: Unknown lock type " + input);
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::subparse_lock_id(const std::string &input)
    {
        // Supports locking by ID and end (lock)
        //
        if (is_variable(input))
        {
            // An ID, try and look it up and use as the lock
            //
            const dbtype::Id &id = get_variable(input);

            if (id.is_default())
            {
                set_error("lock by ID: variable " + input + " not found");
            }
            else
            {
                if (not db.set_entity_lock_field(
                    current_entity_field,
                    id,
                    operation_not))
                {
                    set_error("lock by ID:  Could not lock against ID "
                        + id.to_string(true));
                }
            }
        }
        else
        {
            std::string parsed_input = input;
            const std::string command =
                boost::to_lower_copy(get_word(parsed_input));

            if (command != "end")
            {
                set_error("lock by ID: Unknown command " + command);
            }
            else
            {
                if (parsed_input != "lock")
                {
                    set_error("lock by ID: Unknown type to end " + parsed_input);
                }
                else
                {
                    subparser_mode = SUBPARSER_NONE;
                    operation_not = false;
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::subparse_lock_property(const std::string &input)
    {
        std::string parsed_input = input;
        const std::string command =
            boost::to_lower_copy(get_word(parsed_input));

        if ((not is_parsing_property_data()) and (command == "end"))
        {
            if (parsed_input != "lock")
            {
                set_error("lock by property: Unknown type to end " + parsed_input);
            }
            else
            {
                subparser_mode = SUBPARSER_NONE;
                operation_not = false;
            }
        }
        else
        {
            // Probably the property.  Go ahead and try to parse it.
            //
            std::string property_key;
            dbtype::PropertyData *property_value_ptr = 0;

            if (shared_property_parser(input, property_key, property_value_ptr))
            {
                // Finished parsing, go ahead and use property info.
                if (not property_value_ptr)
                {
                    set_error("lock by property: Unexpected null property value!");
                }
                else
                {
                    if (not db.set_entity_lock_field(
                        current_entity_field,
                        property_key,
                        *property_value_ptr,
                        operation_not))
                    {
                        set_error("lock by property: Unable to set lock");
                    }

                    delete property_value_ptr;
                    property_value_ptr = 0;
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::subparse_document(const std::string &input)
    {
        if (shared_document_parser(input))
        {
            // All done, set the document and continue.
            //
            if (current_property.empty())
            {
                // Setting on a field
                //
                if (not db.set_entity_field(
                        current_entity_field,
                        *current_document_ptr))
                {
                    set_error("document:  Unable to set document on field "
                      + dbtype::entity_field_to_string(
                        current_entity_field));
                }
            }
            else
            {
                // Setting on a property
                //
                if (not db.set_prop(
                        current_property,
                        *current_document_ptr))
                {
                    set_error(
                        "document:  Unable to set document on property "
                        + current_property);
                }
            }

            subparser_mode = SUBPARSER_NONE;
            delete current_document_ptr;
            current_document_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::shared_document_parser(const std::string &input)
    {
        bool result = false;

        if (not current_document_ptr)
        {
            // First call.  Figure out how many lines we will have
            //
            std::string parsed_input = input;
            const std::string command =
                boost::to_lower_copy(get_word(parsed_input));

            if (command == "lines")
            {
                if (not text::from_string<MG_UnsignedInt>(
                    parsed_input,
                    items_left))
                {
                    set_error("document:  Cannot convert number of lines: "
                              + parsed_input);
                }
                else
                {
                    current_document_ptr = new dbtype::DocumentProperty();
                }
            }
            else
            {
                set_error("document: Missing number of lines: " + input);
            }
        }
        else if (items_left)
        {
            // Document in progress.  You can end it early with '.end'.
            // TODO Eliminate line count altogether?
            //
            if (input == ".end")
            {
                items_left = 1;
                result = true;
            }
            else if (not current_document_ptr->append_line(input))
            {
                set_error("document:  Unable to append line.");
            }

            --items_left;
        }
        else
        {
            // We're at the end; confirm by looking for an 'end', then returning
            // true so caller knows they can set the document.
            //
            std::string parsed_input = boost::to_lower_copy(input);
            const std::string command = get_word(parsed_input);

            if (command == "end")
            {
                if (parsed_input == "lines")
                {
                    result = true;
                }
                else
                {
                    set_error("document: Unknown mode to end: " + parsed_input);
                }
            }
            else
            {
                set_error("document: Unexpected command: " + input);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::shared_set_parser(
        const dbtype::PropertyDataType &set_type,
        const std::string &input)
    {
        bool result = false;

        if (not current_set_ptr)
        {
            // First call.  Figure out how many items we will have
            //
            std::string parsed_input = input;
            const std::string command =
                boost::to_lower_copy(get_word(parsed_input));

            if (command == "items")
            {
                if (not text::from_string<MG_UnsignedInt>(
                    parsed_input,
                    items_left))
                {
                    set_error("set:  Cannot convert number of items: "
                              + parsed_input);
                }
                else
                {
                    current_set_ptr = new dbtype::SetProperty();
                }
            }
            else
            {
                set_error("set: Missing number of items: " + input);
            }
        }
        else if (items_left)
        {
            // Set in progress
            //
            dbtype::PropertyData *item_data =
                db.create_property_data(current_set_type, input);

            if (not item_data)
            {
                set_error("set: Unable to parse item: " + input);
            }
            else
            {
                if (not current_set_ptr->add(*item_data))
                {
                    set_error("set: Unable to add item data: "
                        + item_data->get_as_string());
                }

                delete item_data;
                item_data = 0;
            }

            --items_left;
        }
        else
        {
            // We're at the end; confirm by looking for an 'end', then returning
            // true so caller knows they can set the 'set'.
            //
            std::string parsed_input = boost::to_lower_copy(input);
            const std::string command = get_word(parsed_input);

            if (command == "end")
            {
                if (parsed_input == "items")
                {
                    result = true;
                }
                else
                {
                    set_error("set: Unknown mode to end: " + parsed_input);
                }
            }
            else
            {
                set_error("set: Unexpected command: " + input);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::shared_property_parser(
        const std::string &input,
        std::string &property_key,
        dbtype::PropertyData *&value_ptr)
    {
        bool result = false;

        if (current_document_ptr)
        {
            // In the middle of parsing a document, so call that instead
            if (shared_document_parser(input))
            {
                // Finished the document, return the data.
                //
                property_key = current_property;
                value_ptr = current_document_ptr;

                current_property.clear();
                current_document_ptr = 0;
                result = true;
            }
        }
        else if (current_set_ptr)
        {
            // In the middle of parsing a set, so call that instead.
            if (shared_set_parser(current_set_type, input))
            {
                // Finished the set, return the data.
                //
                property_key = current_property;
                value_ptr = current_set_ptr;

                current_property.clear();
                current_set_ptr = 0;
                result = true;
            }
        }
        else
        {
            std::string parsed_input = input;
            const std::string property_type_string =
                boost::to_lower_copy(get_word(parsed_input));

            dbtype::PropertyDataType property_type =
                dbtype::string_to_property_data_type(property_type_string);

            if (property_type == dbtype::PROPERTYDATATYPE_invalid)
            {
                set_error("property: Unknown property type "
                          + property_type_string);
            }
            else
            {
                std::string property_value;

                // Parse a property, including any special multi-line or other
                // cases.
                //

                if (property_type == dbtype::PROPERTYDATATYPE_document)
                {
                    // Special case for documents since they are multi-line
                    //
                    if (not get_key_value(
                        parsed_input,
                        current_property,
                        property_value))
                    {
                        set_error("property (document): Unable to parse "
                            "key value pair: " + parsed_input);
                    }
                    else
                    {
                        // Prime the document parser with number of lines
                        shared_document_parser(property_value);
                    }
                }
                else if (property_type == dbtype::PROPERTYDATATYPE_id)
                {
                    // Special case for IDs because they only use variables.
                    //
                    if (not get_key_value(
                        parsed_input,
                        property_key,
                        property_value))
                    {
                        set_error("property (id): Unable to parse "
                                      "key value pair: " + parsed_input);
                    }
                    else
                    {
                        if (not is_variable(property_value))
                        {
                            set_error("property (id): Value is not a variable");
                        }
                        else
                        {
                            const dbtype::Id &id = get_variable(property_value);

                            if (id.is_default())
                            {
                                set_error("property (id): Variable "
                                    + property_value + " does not exist.");
                            }
                            else
                            {
                                value_ptr = new dbtype::IdProperty(id);
                                result = true;
                            }
                        }
                    }
                }
                else if (property_type == dbtype::PROPERTYDATATYPE_set)
                {
                    // Special case for sets.  Next word is the type of the
                    // set values
                    //
                    const std::string set_type_string =
                        boost::to_lower_copy(get_word(parsed_input));
                    const dbtype::PropertyDataType set_type =
                        dbtype::string_to_property_data_type(set_type_string);

                    if (set_type == dbtype::PROPERTYDATATYPE_invalid)
                    {
                        set_error("property (set): Invalid set type "
                                  + set_type_string);
                    }
                    else if (not get_key_value(
                        parsed_input,
                        current_property,
                        property_value))
                    {
                        set_error("property (set): Unable to parse key value "
                                      "pair: " + parsed_input);
                    }
                    else
                    {
                        shared_set_parser(set_type, property_value);
                    }
                }
                else
                {
                    // Normal case
                    //
                    if (not get_key_value(
                        parsed_input,
                        property_key,
                        property_value))
                    {
                        set_error("property (simple): Unable to parse key value "
                                  "pair: " + parsed_input);
                    }
                    else
                    {
                        value_ptr =
                            db.create_property_data(property_type, property_value);
                        result = value_ptr;
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void MutgosDumpFileReader::set_error(const std::string &cause)
    {
        error_condition = true;
        status_message = cause;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::get_key_value(
        const std::string &input,
        std::string &key,
        std::string &value) const
    {
        bool result = true;
        const size_t equals_index = input.find_first_of('=');

        key.clear();
        value.clear();

        if (equals_index == std::string::npos)
        {
            result = false;
        }
        else
        {
            if (equals_index)
            {
                key = boost::trim_right_copy(input.substr(0, equals_index));
            }

            if ((input.size() - 1) > equals_index)
            {
                value = boost::trim_left_copy(input.substr(equals_index + 1));
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string MutgosDumpFileReader::get_word(
        std::string &input) const
    {
        std::string result;

        const size_t index = input.find_first_of(' ');

        if (index == std::string::npos)
        {
            // Not found or whole string is just the command.
            if (not input.empty())
            {
                result = input;
                input.clear();
            }
        }
        else
        {
            result = input.substr(0, index);
            input.erase(0, index + 1);
            boost::trim_left(input);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::is_variable(const std::string &input) const
    {
        return (input.size() > 1) and (input[0] == VAR_PREFIX);
    }

    // ----------------------------------------------------------------------
    const dbtype::Id& MutgosDumpFileReader::get_variable(
        const std::string &variable) const
    {
        if (is_variable(variable))
        {
            VariableMap::const_iterator var_iter =
                variables.find(variable.substr(1));

            if (var_iter != variables.end())
            {
                return var_iter->second;
            }
        }

        return default_id;
    }

    // ----------------------------------------------------------------------
    bool MutgosDumpFileReader::set_variable(
        const std::string &variable,
        const dbtype::Id &id)
    {
        bool result = false;

        if (is_variable(variable))
        {
            const std::string variable_name =
                variable.substr(1);

            variables.erase(variable_name);
            variables.insert(std::make_pair(variable_name, id));
            result = true;
        }

        return result;
    }
}
}
