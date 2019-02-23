/*
 * dbtype_IdProperty.cpp
 */

#include <stddef.h>

#include "dbtypes/dbtype_IdProperty.h"
#include "dbtypes/dbtype_Id.h"

#include <string>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "text/text_StringConversion.h"

namespace mutgos
{
namespace dbtype
{
    // ------------------------------------------------------------------
    IdProperty::IdProperty()
        : PropertyData(PROPERTYDATATYPE_id)
    {
    }

    // ------------------------------------------------------------------
    IdProperty::IdProperty(const Id &data)
        : PropertyData(PROPERTYDATATYPE_id),
          id_data(data)
    {
    }

    // ------------------------------------------------------------------
    IdProperty::IdProperty(const IdProperty &data)
      : PropertyData(data),
        id_data(data.id_data)
    {
    }

    // ------------------------------------------------------------------
    IdProperty::~IdProperty()
    {
    }

    // ------------------------------------------------------------------
    bool IdProperty::operator==(const PropertyData *rhs) const
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
            const IdProperty *rhs_casted = dynamic_cast<const IdProperty *>(rhs);

            if (rhs_casted)
            {
                return id_data == rhs_casted->id_data;
            }
        }

        return false;
    }

    // ------------------------------------------------------------------
    bool IdProperty::operator<(const PropertyData *rhs) const
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
            const IdProperty *rhs_casted = dynamic_cast<const IdProperty *>(rhs);

            if (rhs_casted)
            {
                return id_data < rhs_casted->id_data;
            }

            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    PropertyData *IdProperty::clone(void) const
    {
        return new IdProperty(*this);
    }

    // ------------------------------------------------------------------
    std::string IdProperty::get_as_short_string(void) const
    {
        return id_data.to_string(false);
    }

    // ------------------------------------------------------------------
    std::string IdProperty::get_as_string(void) const
    {
        return id_data.to_string(true);
    }

    // ------------------------------------------------------------------
    bool IdProperty::set_from_string(const std::string &str)
    {
        bool result = true;
        const std::string trimmed_str = boost::trim_copy(str);

        if (trimmed_str.empty())
        {
            result = false;
        }
        else if (trimmed_str[0] != '#')
        {
            result = false;
        }
        else
        {
            std::string site_id_str;
            std::string entity_id_str;

            boost::char_separator<char> sep(" #-");
            boost::tokenizer<boost::char_separator<char> >
                tokens(trimmed_str, sep);

            // An ID can currently only be in the form of #A-B, where
            // A is the site ID, and B is the entity ID.
            // The tokenizer should return back exactly two positive
            // numbers.  If anything else is returned, then it is invalid.
            //
            for (boost::tokenizer<boost::char_separator<char> >::iterator
                    tok_iter = tokens.begin();
                 tok_iter != tokens.end(); ++tok_iter)
            {
                if (site_id_str.empty())
                {
                    site_id_str = *tok_iter;
                }
                else if (entity_id_str.empty())
                {
                    entity_id_str = *tok_iter;
                }
                else
                {
                    // Should only be two items.
                    result = false;
                    break;
                }
            }

            if (result)
            {
                // Convert from string to int.
                //
                Id::SiteIdType site_id = 0;

                if (not text::from_string<Id::SiteIdType>(
                    site_id_str,
                    site_id))
                {
                    result = false;
                }

                Id::EntityIdType entity_id = 0;

                if (not text::from_string<Id::EntityIdType>(
                    entity_id_str,
                    entity_id))
                {
                    result = false;
                }

                // If converted, set the ID.
                //
                if (result)
                {
                    id_data = Id(site_id, entity_id);
                }
            }
        }

        return result;
    }

    // ------------------------------------------------------------------
    bool IdProperty::set(const Id &data)
    {
        id_data = data;

        return true;
    }

    // ------------------------------------------------------------------
    const Id &IdProperty::get(void) const
    {
        return id_data;
    }

    // ------------------------------------------------------------------
    size_t IdProperty::mem_used(void) const
    {
        return PropertyData::mem_used() + id_data.mem_used();
    }
} /* namespace dbtype */
} /* namespace mutgos */
