/*
 * dbtype_FloatProperty.cpp
 */

#include <stddef.h>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"

#include "dbtypes/dbtype_FloatProperty.h"
#include "osinterface/osinterface_OsTypes.h"

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    FloatProperty::FloatProperty()
      : PropertyData(PROPERTYDATATYPE_float),
        float_data(0)
    {
    }

    // ----------------------------------------------------------------------
    FloatProperty::FloatProperty(const MG_Float data)
      : PropertyData(PROPERTYDATATYPE_float),
        float_data(data)
    {
    }

    // ----------------------------------------------------------------------
    FloatProperty::FloatProperty(const FloatProperty &data)
      : PropertyData(data),
        float_data(data.float_data)
    {
    }

    // ----------------------------------------------------------------------
    FloatProperty::~FloatProperty()
    {
    }

    // ------------------------------------------------------------------
    bool FloatProperty::operator==(const PropertyData *rhs) const
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
            const FloatProperty *rhs_casted =
                dynamic_cast<const FloatProperty *>(rhs);

            if (rhs_casted)
            {
                return float_data == rhs_casted->float_data;
            }
        }

        return false;
    }

    // ------------------------------------------------------------------
    bool FloatProperty::operator<(const PropertyData *rhs) const
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
            const FloatProperty *rhs_casted =
                dynamic_cast<const FloatProperty *>(rhs);

            if (rhs_casted)
            {
                return float_data < rhs_casted->float_data;
            }

            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    PropertyData *FloatProperty::clone(void) const
    {
        return new FloatProperty(*this);
    }

    // ----------------------------------------------------------------------
    std::string FloatProperty::get_as_short_string(void) const
    {
        return get_as_string();
    }

    // ----------------------------------------------------------------------
    std::string FloatProperty::get_as_string(void) const
    {
        return text::to_string(float_data);
    }

    // ----------------------------------------------------------------------
    bool FloatProperty::set_from_string(const std::string &str)
    {
        return text::from_string<MG_Float>(str, float_data);
    }

    // ----------------------------------------------------------------------
    bool FloatProperty::set(const MG_Float data)
    {
        float_data = data;
        return true;
    }

    // ----------------------------------------------------------------------
    MG_Float FloatProperty::get(void) const
    {
        return float_data;
    }

    // ------------------------------------------------------------------
    size_t FloatProperty::mem_used(void) const
    {
        return PropertyData::mem_used() + sizeof(float_data);
    }
} /* namespace dbtype */
} /* namespace mutgos */
