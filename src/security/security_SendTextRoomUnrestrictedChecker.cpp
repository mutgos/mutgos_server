/*
 * security_SendTextRoomUnlimitedChecker.cpp
 */

#include "security_SendTextRoomUnrestrictedChecker.h"
#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    SendTextRoomUnrestrictedChecker::SendTextRoomUnrestrictedChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    SendTextRoomUnrestrictedChecker::~SendTextRoomUnrestrictedChecker()
    {
    }

    // -----------------------------------------------------------------------
    Result SendTextRoomUnrestrictedChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        return (context.has_capability(CAPABILITY_SEND_TEXT_ROOM_UNRESTRICTED) ?
            RESULT_ACCEPT : RESULT_DENY);
    }
}
}