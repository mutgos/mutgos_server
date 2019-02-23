/*
 * dbtype_IntegerProperty.h
 */

#ifndef MUTGOS_DBTYPE_INTEGERPROPERTY_H_
#define MUTGOS_DBTYPE_INTEGERPROPERTY_H_

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
     * A property containing an integer.
     */
    class IntegerProperty : public PropertyData
    {
    public:
        /**
         * Creates a default IntegerProperty.
         */
        IntegerProperty();

        /**
         * Creates an IntegerProperty with the provided data.
         * @param data[in] The integer data to use.
         */
        IntegerProperty(const MG_SignedInt data);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        IntegerProperty(const IntegerProperty &data);

        virtual ~IntegerProperty();

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
         * form, which in this case is the same as the usual string.
         */
        virtual std::string get_as_short_string(void) const;

        /**
         * @return The data contained by this instance as a string.
         */
        virtual std::string get_as_string(void) const;

        /**
         * Sets the integer data contained by this instance using a string.
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * Sets the integer data contained by this instance using an integer.
         * @param data[in] The integer data to set.
         * @return True if successfully set.
         */
        bool set(const MG_SignedInt data);

        /**
         * @return The data contained by this IntegerProperty.
         */
        MG_SignedInt get(void) const;

        /**
         * @return The approximate amount of memory used by this
         * IntegerProperty.
         */
        virtual size_t mem_used(void) const;

    protected:
        MG_SignedInt int_data; ///< The integer data.

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & int_data;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & int_data;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_INTEGERPROPERTY_H_ */
