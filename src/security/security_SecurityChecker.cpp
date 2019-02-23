/*
 * security_SecurityChecker.cpp
 */

#include "security_SecurityChecker.h"
#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    SecurityChecker::~SecurityChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const security::Operation operation,
        security::Context &context)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    Result SecurityChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source)
    {
        return RESULT_SKIP;
    }

    // -----------------------------------------------------------------------
    SecurityChecker::SecurityChecker(void)
    {
    }
}
}
