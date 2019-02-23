/*
 * security_CharacterOnlineChecker.cpp
 */

#include "security_CharacterOnlineChecker.h"
#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    CharacterOnlineChecker::CharacterOnlineChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    CharacterOnlineChecker::~CharacterOnlineChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result CharacterOnlineChecker::security_check(
        const Operation operation,
        Context &context)
    {
        return (context.has_capability(CAPABILITY_CONNECTION_CHECK) ?
            RESULT_ACCEPT : RESULT_DENY);
    }

    // -----------------------------------------------------------------------
    Result CharacterOnlineChecker::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return (context.has_capability(CAPABILITY_CONNECTION_CHECK) ?
            RESULT_ACCEPT : RESULT_DENY);
    }
}
}