/*
 * security_CrossSiteChecker.cpp
 */

#include "dbtypes/dbtype_Id.h"
#include "security_SecurityChecker.h"
#include "security_CrossSiteChecker.h"
#include "dbinterface/dbinterface_CommonTypes.h"

namespace mutgos
{
namespace security
{
    // ----------------------------------------------------------------------
    CrossSiteChecker::CrossSiteChecker()
    {
    }

    // ----------------------------------------------------------------------
    CrossSiteChecker::~CrossSiteChecker()
    {
    }

    // ----------------------------------------------------------------------
    Result CrossSiteChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return check_site(
            context.get_requester(),
            entity_target.id());
    }

    // ----------------------------------------------------------------------
    Result CrossSiteChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        return check_site(
            context.get_requester(),
            entity_target.id());
    }

    // ----------------------------------------------------------------------
    Result CrossSiteChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        return check_site(
            context.get_requester(),
            entity_target.id());
    }

    // ----------------------------------------------------------------------
    Result CrossSiteChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        const Result target_result = check_site(
            context.get_requester(),
            entity_target.id());
        const Result source_result = check_site(
            context.get_requester(),
            entity_source.id());

        if ((target_result == RESULT_ACCEPT) and
            (source_result == RESULT_ACCEPT))
        {
            return RESULT_ACCEPT;
        }

        return RESULT_DENY_ALWAYS;
    }

    // ----------------------------------------------------------------------
    Result CrossSiteChecker::check_site(
        const dbtype::Id &requester_id,
        const dbtype::Id &target_id)
    {
        if ((requester_id.get_site_id() == dbinterface::GLOBAL_SITE_ID) or
            ((target_id.get_site_id() == requester_id.get_site_id()) or
            (target_id.get_site_id() == dbinterface::GLOBAL_SITE_ID)))
        {
            return RESULT_ACCEPT;
        }

        return RESULT_DENY_ALWAYS;
    }
}
}
