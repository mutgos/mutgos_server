/*
 * dbtype_PropertySecurity.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYSECURITY_H_
#define MUTGOS_DBTYPE_PROPERTYSECURITY_H_

#include "dbtypes/dbtype_Security.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

namespace mutgos
{
namespace dbtype
{
    /**
     * Modifies the behavior of Security slightly to allow only the flags
     * used by properties.
     */
    class PropertySecurity : public Security
    {
    public:
        /**
         * Standard constructor.
         */
        PropertySecurity(void);

        /**
         * Copy constructor.
         * @param[in] rhs The source to copy from.
         */
        PropertySecurity(const PropertySecurity &rhs);

        /**
         * Standard destructor.
         */
        virtual ~PropertySecurity();

        /**
         * Assignment operator.
         * @param[in] rhs The source to copy from.
         * @return The copy.
         */
        virtual PropertySecurity &operator=(const PropertySecurity &rhs);

        /**
         * Compares against another PropertySecurity instance.
         * @param[in] rhs The PropertySecurity instance to compare against.
         * @return True if the two instances are the same.
         */
        virtual bool operator==(const PropertySecurity &rhs) const;

        /**
         * @return This PropertySecurity instance as a string.
         */
        virtual std::string to_string(void) const;

    protected:
        /**
         * Determines if a flag is allowed to be used.  For this class,
         * only read and write flags are allowed.
         * @param flag[in] The flag to check.
         * @return True if the flag can be used.
         */
        virtual bool allow_flag(const SecurityFlag flag) const;

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Security>(*this);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Security>(*this);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_PROPERTYSECURITY_H_ */
