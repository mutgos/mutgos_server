/*
 * dbtype_SetProperty.cpp
 */


#include <stddef.h>
#include <list>
#include <set>
#include <string>
#include <sstream>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"

#include "dbtypes/dbtype_SetProperty.h"
#include "osinterface/osinterface_OsTypes.h"
#include "utilities/mutgos_config.h"

namespace
{
    const MG_UnsignedInt SHORT_STRING_SIZE = 60;
    const MG_UnsignedInt MAX_STRING_SIZE = 32768; // for get_as_string()
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    SetProperty::SetProperty()
      : PropertyData(PROPERTYDATATYPE_set),
        property_data_set_type(PROPERTYDATATYPE_invalid)
    {
    }

    // ----------------------------------------------------------------------
    SetProperty::SetProperty(const SetProperty &rhs)
      : PropertyData(PROPERTYDATATYPE_set),
        property_data_set_type(rhs.property_data_set_type)
    {
        PropertyData *copy_ptr = 0;

        for (PropertyDataSet::const_iterator
               copy_iter = rhs.property_data_set.begin();
             copy_iter != rhs.property_data_set.end();
             ++copy_iter)
        {
            if (*copy_iter)
            {
                copy_ptr = (*copy_iter)->clone();

                if (copy_ptr)
                {
                    property_data_set.insert(copy_ptr);
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    SetProperty::~SetProperty()
    {
        clear();
    }

    // ------------------------------------------------------------------
    bool SetProperty::operator==(const PropertyData *rhs) const
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
            const SetProperty *rhs_casted =
                dynamic_cast<const SetProperty *>(rhs);

            if (rhs_casted)
            {
                if (property_data_set_type != rhs_casted->property_data_set_type)
                {
                    return false;
                }
                else if (property_data_set.size() !=
                    rhs_casted->property_data_set.size())
                {
                    return false;
                }
                else
                {
                    PropertyDataSet::const_iterator
                        lhs_iter = property_data_set.begin(),
                        rhs_iter = rhs_casted->property_data_set.begin();

                    while ((lhs_iter != property_data_set.end()) and
                           (rhs_iter != rhs_casted->property_data_set.end()))
                    {
                        // Guaranteed not to be a null pointer
                        if (**lhs_iter != **rhs_iter)
                        {
                            return false;
                        }

                        ++lhs_iter;
                        ++rhs_iter;
                    }

                    if ((lhs_iter == property_data_set.end()) and
                        (rhs_iter == rhs_casted->property_data_set.end()))
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }

        return false;
    }

    // ------------------------------------------------------------------
    bool SetProperty::operator<(const PropertyData *rhs) const
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
            const SetProperty *rhs_casted =
                dynamic_cast<const SetProperty *>(rhs);

            if (rhs_casted)
            {
                if (property_data_set_type < rhs_casted->property_data_set_type)
                {
                    return true;
                }
                else if (property_data_set.size() <
                    rhs_casted->property_data_set.size())
                {
                    return true;
                }
                else if (property_data_set.size() >
                    rhs_casted->property_data_set.size())
                {
                    return false;
                }
                else
                {
                    // This means the set size of this is the same as rhs.
                    // Compare one by one until we find something less than.
                    //
                    for (PropertyDataSet::const_iterator
                            iter = property_data_set.begin(),
                            rhs_iter = rhs_casted->property_data_set.begin();
                        (iter != property_data_set.end()) and
                          (rhs_iter != rhs_casted->property_data_set.end());
                        ++iter, ++rhs_iter)
                    {
                        // Guaranteed not to be a null pointer
                        if (**iter < **rhs_iter)
                        {
                            return true;
                        }
                    }

                    // Couldn't find anything less than in the contents
                    return false;
                }
            }

            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    PropertyData *SetProperty::clone(void) const
    {
        return new SetProperty(*this);
    }

    // ------------------------------------------------------------------
    std::string SetProperty::get_as_short_string(void) const
    {
        bool stop = false;
        std::ostringstream stream;

        stream << "{";

        for (PropertyDataSet::const_iterator iter = property_data_set.begin();
             (iter != property_data_set.end()) and (not stop);
             ++iter)
        {
            if (stream.tellp() >= SHORT_STRING_SIZE)
            {
                stop = true;
            }
            else
            {
                stream << " [";
                stream << (*iter)->get_as_short_string();
                stream << "]";
            }
        }

        if (stream.tellp() >= SHORT_STRING_SIZE)
        {
            // Too big, truncate what's at the end
            return (stream.str().substr(0, SHORT_STRING_SIZE - 4) + ".. }");
        }
        else
        {
            return (stream.str() + " }");
        }
    }

    // ------------------------------------------------------------------
    std::string SetProperty::get_as_string(void) const
    {
        bool stop = false;
        std::ostringstream stream;

        stream << "{";

        for (PropertyDataSet::const_iterator iter = property_data_set.begin();
             (iter != property_data_set.end()) and (not stop);
             ++iter)
        {
            if (stream.tellp() >= MAX_STRING_SIZE)
            {
                stop = true;
            }
            else
            {
                stream << " [";
                stream << (*iter)->get_as_string();
                stream << "]";
            }
        }

        if (stream.tellp() >= MAX_STRING_SIZE)
        {
            // Too big, truncate what's at the end
            return (stream.str().substr(0, MAX_STRING_SIZE - 4) + ".. }");
        }
        else
        {
            return (stream.str() + " }");
        }
    }

    // ------------------------------------------------------------------
    bool SetProperty::set_from_string(const std::string &str)
    {
        return false;
    }

    // ------------------------------------------------------------------
    size_t SetProperty::mem_used(void) const
    {
        size_t set_mem = PropertyData::mem_used() + sizeof(PropertyDataType);

        for (PropertyDataSet::const_iterator iter = property_data_set.begin();
             iter != property_data_set.end();
             ++iter)
        {
            set_mem += (*iter)->mem_used();
        }

        return set_mem;
    }

    // ----------------------------------------------------------------------
    void SetProperty::clear(void)
    {
        for (PropertyDataSet::iterator delete_iter = property_data_set.begin();
             delete_iter != property_data_set.end();
             ++delete_iter)
        {
            delete *delete_iter;
        }

        property_data_set.clear();
        property_data_set_type = PROPERTYDATATYPE_invalid;
    }

    // ----------------------------------------------------------------------
    bool SetProperty::add(PropertyData &item)
    {
        bool added = true;

        if (property_data_set_type == PROPERTYDATATYPE_invalid)
        {
            // First addition
            PropertyData *data_ptr = item.clone();

            if (data_ptr)
            {
                property_data_set_type = item.get_data_type();

                if (! property_data_set.insert(data_ptr).second)
                {
                    // Already existed???
                    delete data_ptr;
                    data_ptr = 0;

                    LOG(error, "dbtypes", "add()",
                      "Was supposedly empty but had duplicate insert!");
                }
            }
            else
            {
                added = false;
            }
        }
        else
        {
            // Make sure the item matches the type of the others
            if (property_data_set_type != item.get_data_type())
            {
                added = false;
            }
            else if (is_full())
            {
                // Correct type but we're full
                added = false;
            }
            else
            {
                // Correct type and there's room
                PropertyData *data_ptr = item.clone();

                if (data_ptr)
                {
                    if (! property_data_set.insert(data_ptr).second)
                    {
                        // Already existed.
                        delete data_ptr;
                        data_ptr = 0;
                    }
                }
                else
                {
                    added = false;
                }
            }
        }

        return added;
    }

    // ----------------------------------------------------------------------
    bool SetProperty::remove(PropertyData &item)
    {
        bool removed = true;

        if (property_data_set_type != item.get_data_type())
        {
            // Wrong type.
            removed = false;
        }
        else
        {
            // A little tricky, but must hold onto the pointer until
            // delete confirmed, then it can be deleted.
            //
            PropertyDataSet::iterator erase_iter =
                property_data_set.find(&item);

            if (erase_iter != property_data_set.end())
            {
                const PropertyData *data_ptr = *erase_iter;

                property_data_set.erase(erase_iter);

                delete data_ptr;
                data_ptr = 0;

                // If that's the last item, open us up to any data type again.
                if (property_data_set.empty())
                {
                    property_data_set_type = PROPERTYDATATYPE_invalid;
                }
            }
        }

        return removed;
    }

    // ----------------------------------------------------------------------
    bool SetProperty::contains(const PropertyData &item) const
    {
        bool contained = false;

        if (property_data_set_type == item.get_data_type())
        {
            contained =
                (property_data_set.find(&item) != property_data_set.end());
        }

        return contained;
    }

    // ----------------------------------------------------------------------
    bool SetProperty::is_full(void) const
    {
        return property_data_set.size() > config::db::limits_property_set_items();
    }

    // ----------------------------------------------------------------------
    const PropertyData *SetProperty::iter_first(void) const
    {
        if (property_data_set.empty())
        {
            return 0;
        }
        else
        {
            return *(property_data_set.begin());
        }
    }

    // ----------------------------------------------------------------------
    const PropertyData *SetProperty::iter_last(void) const
    {
        if (property_data_set.empty())
        {
            return 0;
        }
        else
        {
            return *(property_data_set.rbegin());
        }
    }

    // ----------------------------------------------------------------------
    const PropertyData *SetProperty::iter_next(const PropertyData *data) const
    {
        if (not data)
        {
            LOG(error, "dbtypes", "iter_next()", "data is null!");

            return 0;
        }

        if (property_data_set.empty())
        {
            return 0;
        }
        else
        {
            if (property_data_set_type != data->get_data_type())
            {
                return 0;
            }
            else
            {
                PropertyDataSet::const_iterator iter =
                    property_data_set.find(data);

                if (iter == property_data_set.end())
                {
                    return 0;
                }
                else
                {
                    ++iter;

                    if (iter == property_data_set.end())
                    {
                        return 0;
                    }
                    else
                    {
                        return *iter;
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    const PropertyData *SetProperty::iter_previous(
        const PropertyData *data) const
    {
        if (not data)
        {
            LOG(error, "dbtypes", "iter_previous()", "data is null!");

            return 0;
        }

        if (property_data_set.empty())
        {
            return 0;
        }
        else
        {
            if (property_data_set_type != data->get_data_type())
            {
                return 0;
            }
            else
            {
                PropertyDataSet::const_iterator iter =
                    property_data_set.find(data);

                if ((iter == property_data_set.end()) or
                    (iter == property_data_set.begin()))
                {
                    return 0;
                }
                else
                {
                    --iter;
                    return *iter;
                }
            }
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
