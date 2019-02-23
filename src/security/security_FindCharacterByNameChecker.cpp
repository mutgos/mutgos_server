/*
 * security_FindCharacterByNameChecker.cpp
 */

#include "security_FindCharacterByNameChecker.h"
#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    FindCharacterByNameChecker::FindCharacterByNameChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    FindCharacterByNameChecker::~FindCharacterByNameChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result FindCharacterByNameChecker::security_check(
        const security::Operation operation,
        security::Context &context)
    {
        return (context.has_capability(CAPABILITY_CHARACTER_FIND_BY_NAME_AFAR) ?
            RESULT_ACCEPT : RESULT_DENY);
    }
}
}
