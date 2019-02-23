/*
 * dbtype_PropertyData.cpp
 */

#include <stddef.h>

#include "dbtypes/dbtype_PropertyData.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    PropertyData::PropertyData(const PropertyDataType type)
      : property_data_type(type)
    {
    }

    // ----------------------------------------------------------------------
    PropertyData::PropertyData(const PropertyData &rhs)
      : property_data_type(rhs.property_data_type)
    {
    }

    // ----------------------------------------------------------------------
    PropertyData::~PropertyData()
    {
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator==(const PropertyData &rhs) const
    {
        return operator==(&rhs);
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator==(const PropertyData *rhs) const
    {
        if (not rhs)
        {
            return false;
        }
        else if (rhs == this)
        {
            return true;
        }
        else
        {
            return property_data_type == rhs->property_data_type;
        }
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator!=(const PropertyData &rhs) const
    {
        return not operator==(&rhs);
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator!=(const PropertyData *rhs) const
    {
        return not operator==(rhs);
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator<(const PropertyData &rhs) const
    {
        return operator<(&rhs);
    }

    // ----------------------------------------------------------------------
    bool PropertyData::operator<(const PropertyData *rhs) const
    {
        if (not rhs)
        {
            return false;
        }
        else if (rhs == this)
        {
            return false;
        }
        else
        {
            return property_data_type < rhs->property_data_type;
        }
    }

    // ----------------------------------------------------------------------
    size_t PropertyData::mem_used(void) const
    {
        return sizeof(PropertyDataType);
    }

    // ----------------------------------------------------------------------
    void PropertyData::copy_fields(const PropertyData &source)
    {
        property_data_type = source.property_data_type;
    }
} /* namespace dbtype */
} /* namespace mutgos */
