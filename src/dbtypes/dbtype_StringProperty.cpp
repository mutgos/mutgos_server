/*
 * dbtype_StringProperty.cpp
 */

#include <string>
#include <stddef.h>

#include "dbtypes/dbtype_StringProperty.h"
#include "utilities/mutgos_config.h"
#include "text/text_Utf8Tools.h"

#define SHORT_STRING_LENGTH 60

namespace mutgos
{
    namespace dbtype
    {
        // ------------------------------------------------------------------
        StringProperty::StringProperty()
          : PropertyData(PROPERTYDATATYPE_string)
        {
        }

        // ------------------------------------------------------------------
        StringProperty::StringProperty(const StringProperty &data)
          : PropertyData(data),
            string_data(data.string_data)
        {
        }

        // ------------------------------------------------------------------
        StringProperty::~StringProperty()
        {
        }

        // ------------------------------------------------------------------
        bool StringProperty::operator==(const PropertyData *rhs) const
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
                const StringProperty *rhs_casted =
                    dynamic_cast<const StringProperty *>(rhs);

                if (rhs_casted)
                {
                    return (string_data == rhs_casted->string_data);
                }
            }

            return false;
        }

        // ------------------------------------------------------------------
        bool StringProperty::operator<(const PropertyData *rhs) const
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
                const StringProperty *rhs_casted =
                    dynamic_cast<const StringProperty *>(rhs);

                if (rhs_casted)
                {
                    return (string_data.compare(rhs_casted->string_data) < 0);
                }

                return false;
            }

            return true;
        }

        // ------------------------------------------------------------------
        PropertyData *StringProperty::clone(void) const
        {
            return new StringProperty(*this);
        }

        // ------------------------------------------------------------------
        std::string StringProperty::get_as_short_string(void) const
        {
            return string_data.substr(0, SHORT_STRING_LENGTH);
        }

        // ------------------------------------------------------------------
        std::string StringProperty::get_as_string(void) const
        {
            return string_data;
        }

        // ------------------------------------------------------------------
        bool StringProperty::set_from_string(const std::string &str)
        {
            return set(str);
        }

        // ------------------------------------------------------------------
        bool StringProperty::set(const std::string &str)
        {
            if (text::utf8_size(str) > config::db::limits_string_size())
            {
                return false;
            }

            string_data = str;

            return true;
        }

        // ------------------------------------------------------------------
        const std::string &StringProperty::get(void) const
        {
            return string_data;
        }

        // ------------------------------------------------------------------
        size_t StringProperty::mem_used(void) const
        {
            return PropertyData::mem_used() + string_data.capacity();
        }
    } /* namespace dbtype */
} /* namespace mutgos */
