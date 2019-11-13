/*
 * dbtype_StringProperty.h
 */

#ifndef MUTGOS_DBTYPE_STRINGPROPERTY_H_
#define MUTGOS_DBTYPE_STRINGPROPERTY_H_

#include <string>
#include <stddef.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * A string property.
     */
    class StringProperty : public PropertyData
    {
    public:
        /**
         * Creates with empty string.
         */
        StringProperty();

        /**
         * Copy constructor.
         * @param rhs The source to copy from.
         */
        StringProperty(const StringProperty &data);

        virtual ~StringProperty();

        /**
         * Equals operator.
         * @param rhs[in] The source to compare.
         * @return True if contents are equal.
         */
        virtual bool operator==(const PropertyData *rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * @param rhs[in] The source to compare.
         * @return True if the contents of this is < rhs.
         */
        virtual bool operator<(const PropertyData *rhs) const;

        /**
         * Creates a clone of the given property data.  Caller must manage
         * returned pointer.
         * @return A pointer to the newly cloned instance of PropertyData.
         * Caller must manage pointer.
         */
        virtual PropertyData *clone(void) const;

        /**
         * @return The data contained by this instance as a 'short' string
         * form, which in this case is 60 characters.  Used for
         * condensed output screens.
         */
        virtual std::string get_as_short_string(void) const;

        /**
         * @return The data contained by this instance as a string.
         */
        virtual std::string get_as_string(void) const;

        /**
         * Sets the string data contained by this instance using a string.
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * Sets the string data contained by this instance using a string.
         * @param str[in] The data to set.
         * @return True if successfully set.
         */
        bool set(const std::string &str);

        /**
         * @return The data contained by this StringProperty.
         */
        const std::string &get(void) const;

        /**
         * @return The approximate amount of memory used by this
         * StringProperty.
         */
        virtual size_t mem_used(void) const;

    protected:
        std::string string_data; ///< The string data.

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            // serialize base class information
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & string_data;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            // serialize base class information
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & string_data;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_STRINGPROPERTY_H_ */
