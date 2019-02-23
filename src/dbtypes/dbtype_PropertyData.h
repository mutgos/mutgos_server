/*
 * dbtype_PropertyData.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYDATA_H_
#define MUTGOS_DBTYPE_PROPERTYDATA_H_

#include <string>
#include <stddef.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_PropertyDataType.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Top level representation of the data portion of a property.
     * This and the various subclasses are NOT thread-safe; that will be
     * handled by whoever owns the instances.
     * This is an abstract class and most methods will be implemented by the
     * various data types.
     */
    class PropertyData
    {
    public:
        /**
         * Constructor.
         * @param type[in] The property data type represented by this instance.
         */
        PropertyData(const PropertyDataType type);

        /**
         * Copy constructor.
         * @param rhs Source to copy.
         */
        PropertyData(const PropertyData &rhs);

        /**
         * Destructor.
         */
        virtual ~PropertyData();

        /**
         * Creates a clone of the given property data.  Caller must manage
         * returned pointer.
         * @return A pointer to the newly cloned instance of PropertyData.
         * Caller must manage pointer.
         */
        virtual PropertyData *clone(void) const =0;

        /**
         * Equals operator.  Calls the pointer version.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        bool operator==(const PropertyData &rhs) const;

        /**
         * Equals operator.  Subclasses are expected to override and call this.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        virtual bool operator==(const PropertyData *rhs) const;

        /**
         * Not equals operator, for convenience.
         * This just calls operator== and inverts the result.
         * Do not use this call as part of an operator== implementation because
         * of this!
         * @param rhs The source to compare.
         * @return True if contents are not equal.
         */
        bool operator!=(const PropertyData &rhs) const;

        /**
         * Not equals operator, for convenience.
         * This just calls operator== and inverts the result.
         * Do not use this call as part of an operator== implementation because
         * of this!
         * @param rhs The source to compare.
         * @return True if contents are not equal.
         */
        bool operator!=(const PropertyData *rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * Calls the pointer version.
         * @param rhs The source to compare.
         * @return True if the contents of this is < rhs.
         */
        bool operator<(const PropertyData &rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * Subclasses are expected to override and call this.
         * @param rhs The source to compare.
         * @return True if the contents of this is < rhs.
         */
        virtual bool operator<(const PropertyData *rhs) const;

        /**
         * @return The property data type represented by this instance.
         */
        PropertyDataType get_data_type(void) const
        {
            return property_data_type;
        }

        /**
         * A convenience method,  This can be done with PropertyDataType
         * directly if desired.
         * @return The property data type as a 'short' string, used for
         * condensed output screens.
         */
        const std::string &get_data_type_string_short(void) const
        {
            return property_data_type_to_short_string(property_data_type);
        }

        /**
         * A convenience method,  This can be done with PropertyDataType
         * directly if desired.
         * @return The property data type as a string, used for output screens.
         */
        const std::string &get_data_type_string(void) const
        {
            return property_data_type_to_string(property_data_type);
        }

        /**
         * @return The data contained by this instance as a 'short' string
         * form.  Sometimes, that's the same as get_as_string(), but if the
         * data is very long, it may for instance return only the first line,
         * X number of characters, or some sort of summary.  Used for
         * condensed output screens.
         */
        virtual std::string get_as_short_string(void) const =0;

        /**
         * @return The data contained by this instance as a string.
         */
        virtual std::string get_as_string(void) const =0;

        /**
         * Sets the data contained by this instance via a string, converting
         * it to the native data format as needed.  The format of the string
         * will vary based on the data type.
         * Subclasses MUST do bounds/size/sanity/error checking!
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str) =0;

        /**
         * Subclasses must override and call parent.
         * @return The approximate amount of memory used by this PropertyData.
         */
        virtual size_t mem_used(void) const;

    protected:
        PropertyDataType property_data_type; ///< The actual type of the data

        /**
         * Copies all PropertyData fields from source.  Called by subclasses
         * as needed.
         * @param source[in] The source to copy PropertyData fields from.
         */
        void copy_fields(const PropertyData &source);

    private:

        // Disabled - doesn't make sense.
        //
        PropertyData &operator=(const PropertyData &rhs);
        PropertyData &operator=(const PropertyData *rhs);

        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & property_data_type;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & property_data_type;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_PROPERTYDATA_H_ */
