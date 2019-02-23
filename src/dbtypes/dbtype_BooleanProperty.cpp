/*
 * dbtype_BooleanProperty.cpp
 */

#include <stddef.h>

#include "dbtypes/dbtype_BooleanProperty.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace mutgos
{
namespace dbtype
{
    // ------------------------------------------------------------------
    BooleanProperty::BooleanProperty()
      : PropertyData(PROPERTYDATATYPE_boolean),
        bool_data(false)
    {
    }

    // ------------------------------------------------------------------
    BooleanProperty::BooleanProperty(const bool data)
      : PropertyData(PROPERTYDATATYPE_boolean),
        bool_data(data)
    {
    }

    // ------------------------------------------------------------------
    BooleanProperty::BooleanProperty(const BooleanProperty &data)
      : PropertyData(data),
        bool_data(data.bool_data)
    {
    }

    // ------------------------------------------------------------------
    BooleanProperty::~BooleanProperty()
    {
    }

    // ------------------------------------------------------------------
    bool BooleanProperty::operator==(const PropertyData *rhs) const
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
            const BooleanProperty *rhs_casted =
                dynamic_cast<const BooleanProperty *>(rhs);

            if (rhs_casted)
            {
                return bool_data == rhs_casted->bool_data;
            }
        }

        return false;
    }

    // ------------------------------------------------------------------
    bool BooleanProperty::operator<(const PropertyData *rhs) const
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
            const BooleanProperty *rhs_casted =
                dynamic_cast<const BooleanProperty *>(rhs);

            if (rhs_casted)
            {
                return ((int) bool_data) < ((int) rhs_casted->bool_data);
            }

            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    PropertyData *BooleanProperty::clone(void) const
    {
        return new BooleanProperty(*this);
    }

    // ------------------------------------------------------------------
    std::string BooleanProperty::get_as_short_string(void) const
    {
        return get_as_string();
    }

    // ------------------------------------------------------------------
    std::string BooleanProperty::get_as_string(void) const
    {
        return (bool_data ? "True" : "False");
    }

    // ------------------------------------------------------------------
    bool BooleanProperty::set_from_string(const std::string &str)
    {
        bool result = true;

        std::string lower(str);
        boost::to_lower(lower);

        if ((lower == "t") || (lower == "true") || (lower == "yes") ||
            (lower =="y"))
        {
            bool_data = true;
        }
        else if ((lower == "f") || (lower == "false") || (lower == "no") ||
                (lower =="n"))
        {
            bool_data = false;
        }
        else
        {
            result = false;
        }

        return result;
    }

    // ------------------------------------------------------------------
    bool BooleanProperty::set(const bool data)
    {
        bool_data = data;

        return true;
    }

    // ------------------------------------------------------------------
    bool BooleanProperty::get(void) const
    {
        return bool_data;
    }

    // ------------------------------------------------------------------
    size_t BooleanProperty::mem_used(void) const
    {
        return PropertyData::mem_used() + sizeof(bool_data);
    }
} /* namespace dbtype */
} /* namespace mutgos */
