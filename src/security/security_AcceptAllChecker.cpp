/*
 * security_AcceptAllChecker.cpp
 */

#include "security_AcceptAllChecker.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    AcceptAllChecker::AcceptAllChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    AcceptAllChecker::~AcceptAllChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const security::Operation operation,
        security::Context &context)
    {
        return RESULT_ACCEPT;
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type)
    {
        return RESULT_ACCEPT;
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return RESULT_ACCEPT;
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        return RESULT_ACCEPT;
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        return RESULT_ACCEPT;
    }

    // -----------------------------------------------------------------------
    Result AcceptAllChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        return RESULT_ACCEPT;
    }
}
}
