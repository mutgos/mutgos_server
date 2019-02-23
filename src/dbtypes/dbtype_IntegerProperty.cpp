/*
 * dbtype_IntegerProperty.cpp
 */

#include <stddef.h>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"

#include "dbtypes/dbtype_IntegerProperty.h"
#include "osinterface/osinterface_OsTypes.h"

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    IntegerProperty::IntegerProperty()
      : PropertyData(PROPERTYDATATYPE_integer),
        int_data(0)
    {
    }

    // ----------------------------------------------------------------------
    IntegerProperty::IntegerProperty(const MG_SignedInt data)
      : PropertyData(PROPERTYDATATYPE_integer),
        int_data(data)
    {
    }

    // ----------------------------------------------------------------------
    IntegerProperty::IntegerProperty(const IntegerProperty &data)
      : PropertyData(data),
        int_data(data.int_data)
    {
    }

    // ----------------------------------------------------------------------
    IntegerProperty::~IntegerProperty()
    {
    }

    // ------------------------------------------------------------------
    bool IntegerProperty::operator==(const PropertyData *rhs) const
    {
        if (not rhs)
        {
            return false;
        }
        else if (rhs == this)
        {
            return true;
        }
        else if (PropertyData::operator==(rhs))
        {
            const IntegerProperty *rhs_casted =
                dynamic_cast<const IntegerProperty *>(rhs);

            if (rhs_casted)
            {
                return int_data == rhs_casted->int_data;
            }
        }

        return false;
    }

    // ------------------------------------------------------------------
    bool IntegerProperty::operator<(const PropertyData *rhs) const
    {
        if (not rhs)
        {
            return false;
        }
        else if (rhs == this)
        {
            return false;
        }
        else if (not PropertyData::operator<(rhs))
        {
            const IntegerProperty *rhs_casted =
                dynamic_cast<const IntegerProperty *>(rhs);

            if (rhs_casted)
            {
                return int_data < rhs_casted->int_data;
            }

            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    PropertyData *IntegerProperty::clone(void) const
    {
        return new IntegerProperty(*this);
    }

    // ----------------------------------------------------------------------
    std::string IntegerProperty::get_as_short_string(void) const
    {
        return get_as_string();
    }

    // ----------------------------------------------------------------------
    std::string IntegerProperty::get_as_string(void) const
    {
        return text::to_string(int_data);
    }

    // ----------------------------------------------------------------------
    bool IntegerProperty::set_from_string(const std::string &str)
    {
        return text::from_string<MG_SignedInt>(str, int_data);
    }

    // ----------------------------------------------------------------------
    bool IntegerProperty::set(const MG_SignedInt data)
    {
        int_data = data;
        return true;
    }

    // ----------------------------------------------------------------------
    MG_SignedInt IntegerProperty::get(void) const
    {
        return int_data;
    }

    // ------------------------------------------------------------------
    size_t IntegerProperty::mem_used(void) const
    {
        return PropertyData::mem_used() + sizeof(int_data);
    }
} /* namespace dbtype */
} /* namespace mutgos */
