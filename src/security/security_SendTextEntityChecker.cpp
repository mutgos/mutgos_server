/*
 * security_SendTextEntityChecker.cpp
 */

#include "security_SendTextEntityChecker.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    SendTextEntityChecker::SendTextEntityChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    SendTextEntityChecker::~SendTextEntityChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result SendTextEntityChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return (context.has_capability(CAPABILITY_SEND_TEXT_ENTITY) ?
            RESULT_ACCEPT : RESULT_DENY);
    }
}
}
