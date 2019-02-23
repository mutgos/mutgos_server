/*
 * dbtype_ApplicationProperties.h
 */

#ifndef MUTGOS_DBTYPE_APPLICATIONPROPERTIES_H_
#define MUTGOS_DBTYPE_APPLICATIONPROPERTIES_H_

#include <string>
#include <stddef.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_PropertySecurity.h"
#include "dbtypes/dbtype_PropertyDirectory.h"
#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents application properties, which is the highest level for
     * partitioning property data.  This class contains information about the
     * partition (such as the application name and security settings) and the
     * properties themselves.
     *
     * This class is not thread safe and cannot be subclassed.
     * This class is not designed to be inherited from or overridden.
     */
    class ApplicationProperties
    {
    public:
        /**
         * Default constructor for deserialization only!
         */
        ApplicationProperties();

        /**
         * Constructs with the given application name and owner.
         * @param name The application name.
         * @param owner The owner of the application properties.
         */
        ApplicationProperties(
            const std::string &name,
            const Id &owner);

        /**
         * Copy constructor.
         * @param rhs The ApplicationProperties to clone.
         */
        ApplicationProperties(const ApplicationProperties &rhs);


        ~ApplicationProperties();

        /**
         * @return The approximate memory (in bytes) used by this instance of
         * ApplicationProperties.  Includes all properties contained by this
         * instance.
         */
        size_t mem_used(void);

        /**
         * Assignment operator.
         * @param rhs Source to copy.
         * @return Itself.
         */
        ApplicationProperties &operator=(const ApplicationProperties &rhs);

        /**
         * Clones this property directory, and all subdirectories.
         * @return A pointer to the cloned PropertyDirectory.  Caller must
         * manage this pointer!
         */
        ApplicationProperties *clone(void) const;

        /**
         * Equals operator.  This is a basic comparison for performance
         * reasons.  It only checks the application name, which should be
         * unique.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        bool operator==(const ApplicationProperties &rhs) const;

        /**
         * Not equals operator, for convenience.
         * This just calls operator== and inverts the result.
         * Do not use this call as part of an operator== implementation because
         * of this!
         * @param rhs The source to compare.
         * @return True if contents are not equal.
         */
        bool operator!=(const ApplicationProperties &rhs) const;

        /**
         * Less than operator.  This is a basic comparison for performance
         * reasons.  It only checks the application name, which should be
         * unique.
         * @param rhs The source to compare.
         * @return True if this < rhs.
         */
        bool operator<(const ApplicationProperties &rhs) const;

        /**
         * This call has a length limit.  If the size of the resulting string
         * is too big, it will truncate the listings.
         * @return A recursive listing of all properties and any other related
         * information about this ApplicationProperties.
         * instance.  The data fields are shown in 'short' form.
         */
        std::string to_string(void) const;


        /**
         * @return The application name.
         */
        const std::string &get_application_name(void) const
        {
            return application_name;
        }

        /**
         * @return The application owner.
         */
        const Id &get_application_owner(void) const
        {
            return application_owner;
        }

        /**
         * Sets the application owner.
         * @param owner The new owner.
         */
        void set_application_owner(const Id &owner)
        {
            application_owner = owner;
        }

        /**
         * @return The security for this application.
         */
        PropertySecurity &get_security(void)
        {
            return security;
        }

        /**
         * @return The properties for this application.
         */
        PropertyDirectory &get_properties(void)
        {
            return properties;
        }

    private:
        std::string application_name; ///< Name of the application for props
        Id application_owner; ///< Dbref (prog, player, etc) of owner

        PropertySecurity security; ///< Security for these properties
        PropertyDirectory properties; ///< Properties for the application

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & application_name;
            ar & application_owner;
            ar & security;
            ar & properties;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & application_name;
            ar & application_owner;
            ar & security;
            ar & properties;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_APPLICATIONPROPERTIES_H_ */
