/*
 * dbtype_PropertyDataType.cpp
 */

#include <string>

#include "dbtypes/dbtype_PropertyDataType.h"

namespace
{
    const static std::string PROPERTY_DATA_TYPE_AS_STRING[] =
    {
        "invalid",
        "string",
        "integer",
        "float",
        "boolean",
        "id",
        "document",
        "set"
    };

    const static std::string PROPERTY_DATA_TYPE_AS_SHORT_STRING[] =
    {
        "inv",
        "str",
        "int",
        "flo",
        "boo",
        "id ",
        "doc",
        "set"
    };
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    const std::string &property_data_type_to_string(const PropertyDataType type)
    {
        if ((type >= PROPERTYDATATYPE_END) or (type <= PROPERTYDATATYPE_invalid))
        {
            return PROPERTY_DATA_TYPE_AS_STRING[0];
        }

        return PROPERTY_DATA_TYPE_AS_STRING[type];
    }

    // ----------------------------------------------------------------------
    const std::string &property_data_type_to_short_string(
        const PropertyDataType type)
    {
        if ((type >= PROPERTYDATATYPE_END) or (type <= PROPERTYDATATYPE_invalid))
        {
            return PROPERTY_DATA_TYPE_AS_SHORT_STRING[0];
        }

        return PROPERTY_DATA_TYPE_AS_SHORT_STRING[type];
    }

    // ----------------------------------------------------------------------
    PropertyDataType string_to_property_data_type(const std::string &str)
    {
        PropertyDataType type = PROPERTYDATATYPE_invalid;

        for (int index = 1; index < PROPERTYDATATYPE_END; ++index)
        {
            if (PROPERTY_DATA_TYPE_AS_STRING[index] == str)
            {
                type = (PropertyDataType) index;
                break;
            }

            if (PROPERTY_DATA_TYPE_AS_SHORT_STRING[index] == str)
            {
                type = (PropertyDataType) index;
                break;
            }
        }

        return type;
    }
} /* namespace dbtype */
} /* namespace mutgos */
