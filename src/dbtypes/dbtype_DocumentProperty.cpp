/*
 * dbtype_DocumentProperty.cpp
 */

#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_PropertyData.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/mutgos_config.h"
#include "text/text_Utf8Tools.h"

#include <boost/tokenizer.hpp>
#include "text/text_StringConversion.h"

#define SHORT_STRING_LENGTH 60

namespace
{
    const std::string EMPTY_STRING;
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    DocumentProperty::DocumentProperty()
      : PropertyData(PROPERTYDATATYPE_document),
        max_lines(config::db::limits_property_document_lines())
    {
    }

    // ----------------------------------------------------------------------
    DocumentProperty::DocumentProperty(const DocumentProperty &data)
      : PropertyData(PROPERTYDATATYPE_document),
        max_lines(data.max_lines)
    {
        set(data.document_data);
    }

    // ----------------------------------------------------------------------
    DocumentProperty::~DocumentProperty()
    {
        clear();
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::operator==(const PropertyData *rhs) const
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
            const DocumentProperty *rhs_casted =
                dynamic_cast<const DocumentProperty *>(rhs);

            if (rhs_casted and
                (document_data.size() == rhs_casted->document_data.size()))
            {
                bool result = true;

                for (DocumentData::const_iterator iter = document_data.begin(),
                        iter_rhs = rhs_casted->document_data.begin();
                     (iter != document_data.end()) and
                       (iter_rhs != rhs_casted->document_data.end());
                     ++iter, ++iter_rhs)
                {
                    if (*iter and *iter_rhs)
                    {
                        if (**iter != **iter_rhs)
                        {
                            result = false;
                            break;
                        }
                    }
                    else
                    {
                        // One or both of them is null, which should never
                        // happen.  Not equal.
                        result = false;
                        break;
                    }
                }

                return result;
            }
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::operator<(const PropertyData *rhs) const
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
            const DocumentProperty *rhs_casted =
                dynamic_cast<const DocumentProperty *>(rhs);

            if (rhs_casted)
            {
                bool result = false;

                for (DocumentData::const_iterator iter = document_data.begin(),
                        iter_rhs = rhs_casted->document_data.begin();
                     (iter != document_data.end()) and
                       (iter_rhs != rhs_casted->document_data.end());
                     ++iter, ++iter_rhs)
                {
                    if (*iter and *iter_rhs)
                    {
                        if ((**iter).compare(**iter_rhs) < 0)
                        {
                            result = true;
                            break;
                        }
                    }
                    else
                    {
                        // One of the documents has a bad pointer.  Should
                        // NEVER happen.
                        //
                        result = true;
                        break;
                    }
                }

                if (not result)
                {
                    // A tie.  See if one document is longer than the other
                    result = document_data.size() <
                        rhs_casted->document_data.size();
                }

                return result;
            }

            return false;
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void DocumentProperty::set_max_lines(
        const osinterface::OsTypes::UnsignedInt max)
    {
        if (max > 0)
        {
            max_lines = max;
        }
    }

    // ------------------------------------------------------------------
    PropertyData *DocumentProperty::clone(void) const
    {
        return new DocumentProperty(*this);
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::append_line(const std::string &data)
    {
        if (is_full())
        {
            return false;
        }
        else if (text::utf8_size(data) > config::db::limits_string_size())
        {
            // Too long a line.
            return false;
        }
        else
        {
            document_data.push_back(new std::string(data));
            return true;
        }
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::insert_line(
        const std::string &data,
        const MG_UnsignedInt line)
    {
        bool success = true;

        if (is_full())
        {
            success = false;
        }
        else if (text::utf8_size(data) > config::db::limits_string_size())
        {
            // Too long a line.
            success = false;
        }
        else
        {
            if (line >= get_number_lines())
            {
                // If out of range, then just append.
                document_data.push_back(new std::string(data));
            }
            else
            {
                document_data.insert(
                    document_data.begin() + line,
                    new std::string(data));
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::delete_line(const MG_UnsignedInt line)
    {
        bool success = true;

        if (get_number_lines() <= line)
        {
            // Trying to delete past the end
            success = false;
        }
        else
        {
            delete document_data[line];
            document_data.erase(document_data.begin() + line);
        }

        return success;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt DocumentProperty::get_number_lines(void) const
    {
        return document_data.size();
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::is_full(void) const
    {
        return document_data.size() > max_lines;
    }

    // ----------------------------------------------------------------------
    const std::string &DocumentProperty::get_line(
        const MG_UnsignedInt line) const
    {
        if (line < get_number_lines())
        {
            return *(document_data[line]);
        }
        else
        {
            // Out of range.
            return EMPTY_STRING;
        }
    }

    // ----------------------------------------------------------------------
    void DocumentProperty::clear(void)
    {
        // Need to clean up memory!
        //
        for (DocumentData::iterator iter = document_data.begin();
             iter != document_data.end();
             ++iter)
        {
            delete *iter;
        }

        document_data.clear();
    }

    // ----------------------------------------------------------------------
    std::string DocumentProperty::get_as_short_string(void) const
    {
        if (document_data.empty())
        {
            return EMPTY_STRING;
        }
        else
        {
            return (*(document_data[0])).substr(0, SHORT_STRING_LENGTH);
        }
    }

    // ----------------------------------------------------------------------
    std::string DocumentProperty::get_as_string(void) const
    {
        std::ostringstream stream;

        for (MG_UnsignedInt index = 0; index < document_data.size(); ++index)
        {
            stream << *(document_data[index]) << std::endl;
        }

        return stream.str();
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::set_from_string(const std::string &str)
    {
        bool success = true;
        clear();

        // Split it out by newlines, and put them one at a time into the
        // document.
        //
        boost::char_separator<char> sep(MG_NewLine);
        boost::tokenizer<boost::char_separator<char> >
            tokens(str, sep);

        for (boost::tokenizer<boost::char_separator<char> >::iterator
                tok_iter = tokens.begin();
             tok_iter != tokens.end(); ++tok_iter)
        {
            if (not append_line(*tok_iter))
            {
                success = false;
                break;
            }
        }

        if (not success)
        {
            clear();
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::set(const std::vector<std::string> &data)
    {
        bool success = true;
        clear();

        for (MG_UnsignedInt index = 0; index < data.size(); ++index)
        {
            if (not append_line(data[index]))
            {
                success = false;
                break;
            }
        }

        if (not success)
        {
            clear();
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DocumentProperty::set(const DocumentData &data)
    {
        if (&data == &document_data)
        {
            // This is us!
            return true;
        }

        clear();

        for (DocumentData::const_iterator iter = data.begin();
             iter != data.end();
             ++iter)
        {
            std::string * const string_ptr = new std::string(**iter);
            document_data.push_back(string_ptr);
        }

        return true;
    }

    // ----------------------------------------------------------------------
    const DocumentProperty::DocumentData &DocumentProperty::get(void) const
    {
        return document_data;
    }

    // ------------------------------------------------------------------
    size_t DocumentProperty::mem_used(void) const
    {
        size_t string_mem = PropertyData::mem_used();

        for (DocumentData::const_iterator iter = document_data.begin();
             iter != document_data.end();
             ++iter)
        {
            string_mem += (*iter)->capacity() + sizeof(void *)
                          + sizeof(max_lines);
        }

        return string_mem;
    }
} /* namespace dbtype */
} /* namespace mutgos */
