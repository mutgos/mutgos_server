/*
 * dbtype_FloatProperty.h
 */

#ifndef MUTGOS_DBTYPE_FLOATPROPERTY_H_
#define MUTGOS_DBTYPE_FLOATPROPERTY_H_

#include <stddef.h>

#include "dbtypes/dbtype_PropertyData.h"
#include "osinterface/osinterface_OsTypes.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

namespace mutgos
{
namespace dbtype
{
    /**
     * A property containing a float.
     */
    class FloatProperty : public PropertyData
    {
    public:
        /**
         * Creates a default FloatProperty.
         */
        FloatProperty();

        /**
         * Creates an FloatProperty with the provided data.
         * @param data[in] The float data to use.
         */
        FloatProperty(const MG_Float data);

        /**
         * Copy constructor.
         * @param rhs The source to copy from.
         */
        FloatProperty(const FloatProperty &data);

        virtual ~FloatProperty();

        /**
         * Equals operator.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        virtual bool operator==(const PropertyData *rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * @param rhs The source to compare.
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
         * form, which in this case is the same as the usual string.
         */
        virtual std::string get_as_short_string(void) const;

        /**
         * @return The data contained by this instance as a string.
         */
        virtual std::string get_as_string(void) const;

        /**
         * Sets the float data contained by this instance using a string.
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * Sets the float data contained by this instance using an float.
         * @param data[in] The float data to set.
         * @return True if successfully set.
         */
        bool set(const MG_Float data);

        /**
         * @return The data contained by this FloatProperty.
         */
        MG_Float get(void) const;

        /**
         * @return The approximate amount of memory used by this
         * FloatProperty.
         */
        virtual size_t mem_used(void) const;

    protected:
        MG_Float float_data; ///< The float data.

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & float_data;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & float_data;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_FLOATPROPERTY_H_ */
