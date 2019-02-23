/*
 * dbtype_PropertySecurity.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include <bitset>
#include <set>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_PropertySecurity.h"

namespace
{
    const static MG_UnsignedInt FLAG_SIZE = 2;
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    PropertySecurity::PropertySecurity()
      : Security()
    {
    }

    // ----------------------------------------------------------------------
    PropertySecurity::PropertySecurity(const PropertySecurity &rhs)
      : Security(rhs)
    {
    }

    // ----------------------------------------------------------------------
    PropertySecurity::~PropertySecurity()
    {
    }

    // ----------------------------------------------------------------------
    PropertySecurity &PropertySecurity::operator=(const PropertySecurity &rhs)
    {
        if (&rhs != this)
        {
            Security::operator=(rhs);
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    bool PropertySecurity::operator==(const PropertySecurity &rhs) const
    {
        return Security::operator==(rhs);
    }

    // ----------------------------------------------------------------------
    std::string PropertySecurity::to_string(void) const
    {
        std::string output;

        to_string_internal(FLAG_SIZE, output);

        return output;
    }

    // ----------------------------------------------------------------------
    bool PropertySecurity::allow_flag(const SecurityFlag flag) const
    {
        return flag < SECURITYFLAG_basic;
    }
} /* namespace dbtype */
} /* namespace mutgos */
