/*
 * dbtype_ApplicationProperties.cpp
 */

#include "dbtypes/dbtype_ApplicationProperties.h"

#include <string>
#include <ostream>
#include <sstream>

#include "dbtypes/dbtype_PropertySecurity.h"
#include "dbtypes/dbtype_PropertyDirectory.h"
#include "dbtypes/dbtype_Id.h"


namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    ApplicationProperties::ApplicationProperties()
    {
    }

    // ----------------------------------------------------------------------
    ApplicationProperties::ApplicationProperties(
        const std::string &name,
        const Id &owner)
      : application_name(name),
        application_owner(owner)
    {
    }

    // ----------------------------------------------------------------------
    ApplicationProperties::ApplicationProperties(
        const ApplicationProperties &rhs)
      : application_name(rhs.application_name),
        application_owner(rhs.application_owner),
        security(rhs.security),
        properties(rhs.properties)
    {
    }

    // ----------------------------------------------------------------------
    ApplicationProperties::~ApplicationProperties()
    {
    }

    // ----------------------------------------------------------------------
    size_t ApplicationProperties::mem_used(void)
    {
        return sizeof(ApplicationProperties)
             + sizeof(application_name)
             + application_name.size()
             + application_owner.mem_used()
             + security.mem_used()
             + properties.mem_used();
    }

    // ----------------------------------------------------------------------
    ApplicationProperties &ApplicationProperties::operator=(
        const ApplicationProperties &rhs)
    {
        if (&rhs != this)
        {
            application_name = rhs.application_name;
            application_owner = rhs.application_owner;
            security = rhs.security;
            properties = rhs.properties;
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    ApplicationProperties *ApplicationProperties::clone(void) const
    {
        return new ApplicationProperties(*this);
    }

    // ----------------------------------------------------------------------
    bool ApplicationProperties::operator==(
        const ApplicationProperties &rhs) const
    {
        // Application names are unique for an entity.
        return application_name == rhs.application_name;
    }

    // ----------------------------------------------------------------------
    bool ApplicationProperties::operator!=(
        const ApplicationProperties &rhs) const
    {
        return not operator==(rhs);
    }

    // ----------------------------------------------------------------------
    bool ApplicationProperties::operator<(
        const ApplicationProperties &rhs) const
    {
        return (application_name.compare(rhs.application_name) < 0);
    }

    // ----------------------------------------------------------------------
    std::string ApplicationProperties::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "Application name: "
                  << application_name
                  << std::endl
                  << "Owner: "
                  << application_owner.to_string(true)
                  << std::endl
                  << "Security: "
                  << security.to_string()
                  << std::endl
                  << "Properties:"
                  << std::endl
                  << properties.to_string();

        return strstream.str();
    }

} /* namespace dbtype */
} /* namespace mutgos */
