/*
 * security_SendTextRoomChecker.cpp
 */

#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "security_SendTextRoomChecker.h"
#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // -----------------------------------------------------------------------
    SendTextRoomChecker::SendTextRoomChecker(void)
    {
    }

    // -----------------------------------------------------------------------
    SendTextRoomChecker::~SendTextRoomChecker()
    {
    }

    // -----------------------------------------------------------------------
    // For now, the actual check of if the text begins with the sender's ID
    // is in the primitive itself.
    Result SendTextRoomChecker::security_check(
        const security::Operation operation,
        security::Context &context,
        dbinterface::EntityRef &entity_target)
    {
        Result result = RESULT_SKIP;

        // If requester's location is the target, then always allow.
        // If not, only allow if unrestricted.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());

        if (entity_ref.valid())
        {
            dbtype::ContainerPropertyEntity * const cpe_ptr =
                dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    entity_ref.get());

            if (cpe_ptr)
            {
                if (cpe_ptr->get_contained_by() == entity_target.id())
                {
                    result = RESULT_ACCEPT;
                }
                else if (context.has_capability(
                    CAPABILITY_SEND_TEXT_ROOM_UNRESTRICTED))
                {
                    result = RESULT_ACCEPT;
                }
                else
                {
                    result = RESULT_DENY;
                }
            }
        }

        return result;
    }
}
}