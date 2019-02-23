/*
 * security_AdminSecurityChecker.cpp
 */

#include "security_AdminSecurityChecker.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    AdminSecurityChecker::AdminSecurityChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    AdminSecurityChecker::~AdminSecurityChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const security::Operation operation,
        security::Context &context)
    {
        return admin_check(context);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type)
    {
        return admin_check(context);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return cross_site_admin_check(context, entity_target);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        return cross_site_admin_check(context, entity_target);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        return cross_site_admin_check(context, entity_target);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        if (cross_site_admin_check(context, entity_target) == RESULT_SKIP)
        {
            return RESULT_SKIP;
        }

        return cross_site_admin_check(context, entity_source);
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::admin_check(security::Context &context)
    {
        if (context.has_admin())
        {
            return RESULT_ACCEPT_ALWAYS;
        }
        else
        {
            return RESULT_SKIP;
        }
    }

    // -----------------------------------------------------------------------
    Result AdminSecurityChecker::cross_site_admin_check(
        mutgos::security::Context &context,
        mutgos::dbinterface::EntityRef &entity_ref)
    {
        if (admin_check(context) == RESULT_SKIP)
        {
            return RESULT_SKIP;
        }

        if (context.get_requester().get_site_id() == entity_ref.id().get_site_id())
        {
            return RESULT_ACCEPT_ALWAYS;
        }
        else
        {
            return RESULT_SKIP;
        }
    }
}
}
