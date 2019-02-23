/*
 * dbtype_IdProperty.h
 */

#ifndef DBTYPE_IDPROPERTY_H_
#define DBTYPE_IDPROPERTY_H_

#include <stddef.h>
#include <string>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_Id.h"
#include <boost/serialization/string.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

namespace mutgos
{
namespace dbtype
{
    /**
     * A property containing an Id (database reference).
     */
    class IdProperty : public PropertyData
    {
    public:
        /**
         * Constructs a default instance.
         */
        IdProperty();

        /**
         * Constructs an instance with the provided ID.
         * @param data[in] The ID data.
         */
        IdProperty(const Id &data);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        IdProperty(const IdProperty &data);

        virtual ~IdProperty();

        /**
         * Equals operator.
         * @param rhs[in] The source to compare.
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
         * Sets the ID data contained by this instance using a string.
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * Sets the ID data contained by this instance using an ID.
         * @param data[in] The ID data to set.
         * @return True if successfully set.
         */
        bool set(const Id &data);

        /**
         * @return The data contained by this IdProperty.
         */
        const Id &get(void) const;

        /**
         * @return The approximate amount of memory used by this
         * IdProperty.
         */
        virtual size_t mem_used(void) const;

    protected:
        Id id_data;  ///< The ID Data

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & id_data;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & id_data;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_IDPROPERTY_H_ */
