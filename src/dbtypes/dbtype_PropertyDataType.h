/*
 * dbtype_PropertyDataType.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYDATATYPE_H_
#define MUTGOS_DBTYPE_PROPERTYDATATYPE_H_

#include <string>

namespace mutgos
{
namespace dbtype
{
    /**
     * An enumeration of the available property data types.
     */
    enum PropertyDataType
    {
        PROPERTYDATATYPE_invalid = 0, ///< Invalid type, do not use!
        PROPERTYDATATYPE_string,    ///< String
        PROPERTYDATATYPE_integer,   ///< Signed integer
        PROPERTYDATATYPE_float,   ///< Signed floating point number
        PROPERTYDATATYPE_boolean,  ///< Boolean
        PROPERTYDATATYPE_id,    ///< Database identifier
        PROPERTYDATATYPE_document, ///< Document (many strings)
        PROPERTYDATATYPE_set,     ///< A set of data
        PROPERTYDATATYPE_END       ///< Always at the end
    };

    /**
     * Given a PropertyDataType enum, return the string equivalent.
     * @param type[in] The type to convert.
     * @return The property data type enum as a string.
     */
    const std::string &property_data_type_to_string(const PropertyDataType type);

    /**
     * Given a PropertyDataType enum, return the short string equivalent.
     * For instance, the normal to_string() might return 'integer', but this
     * one would return 'int'.
     * @param type[in] The type to convert.
     * @return The property data type enum as a three character string.
     */
    const std::string &property_data_type_to_short_string(
        const PropertyDataType type);

    /**
     * Given a string (normal or short), convert it to the
     * @param[in] str The string to convert.  Must not have excess whitespace
     * and be an exact match.
     * @return The string as a PropertyDataType enum, or invalid.
     * @see PropertyDataType
     */
    PropertyDataType string_to_property_data_type(const std::string &str);
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_PROPERTYDATATYPE_H_ */
