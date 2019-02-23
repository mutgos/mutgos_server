/*
 * primitives_EventPrims.cpp
 */

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Room.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "events/events_EventAccess.h"
#include "events/events_EmitEvent.h"

#include "security/security_Context.h"
#include "security/security_SecurityException.h"
#include "security/security_SecurityAccess.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalIdText.h"

#include "primitives_Result.h"

#include "primitives_EventPrims.h"

namespace mutgos
{
namespace primitives
{
    // -----------------------------------------------------------------------
    EventPrims::EventPrims(void)
    {
    }

    // -----------------------------------------------------------------------
    EventPrims::~EventPrims()
    {
    }

    // -----------------------------------------------------------------------
    Result EventPrims::send_text_to_room(
        security::Context &context,
        const dbtype::Id &room,
        text::ExternalTextLine &text_line,
        const bool exclude_requester,
        const bool throw_on_violation)
    {
        Result result;

        // Basic input checks
        //
        if (room.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Confirm requester is a CPE.
        //
        dbinterface::EntityRef requester_entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbtype::ContainerPropertyEntity *cpe_ptr = 0;

        if (requester_entity_ref.valid())
        {
            cpe_ptr = dynamic_cast<dbtype::ContainerPropertyEntity *>(
                requester_entity_ref.get());
        }

        if (not cpe_ptr)
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Confirm room is indeed a Room
        //
        dbinterface::EntityRef room_entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(room);
        dbtype::Room *room_ptr = 0;

        if (room_entity_ref.valid())
        {
            room_ptr = dynamic_cast<dbtype::Room *>(room_entity_ref.get());
        }

        if (not room_ptr)
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Inputs look good.  Determine if text has the sender's ID at the
        // front.
        //
        bool text_contains_id = false;

        if (not text_line.empty())
        {
            text::ExternalText * const text_ptr = text_line.front();

            if (text_ptr and (text_ptr->get_text_type() ==
                                text::ExternalText::TEXT_TYPE_ID))
            {
                text::ExternalIdText * const id_text_ptr =
                    static_cast<text::ExternalIdText *>(text_ptr);

                text_contains_id = (id_text_ptr and  (id_text_ptr->get_id() ==
                    context.get_requester()));
            }
        }

        // Check the appropriate operation based on if it contains the ID.
        //
        bool security_success = false;

        if (text_contains_id)
        {
            security_success = security::SecurityAccess::instance()->
                security_check(
                    security::OPERATION_SEND_TEXT_ROOM,
                    context,
                    room_entity_ref,
                    throw_on_violation);
        }
        else
        {
            security_success = security::SecurityAccess::instance()->
                security_check(
                    security::OPERATION_SEND_TEXT_ROOM_UNRESTRICTED,
                    context,
                    room_entity_ref,
                    throw_on_violation);
        }

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // Everything is good.  Send the text event.
            //
            events::EventAccess::instance()->publish_event(
                new events::EmitEvent(
                    context.get_requester(),
                    room,
                    (exclude_requester ?  context.get_requester() : dbtype::Id()),
                    text_line,
                    context.get_program(),
                    context.get_pid()));
        }

        text::ExternalText::clear_text_line(text_line);

        return result;
    }

    // -----------------------------------------------------------------------
    Result EventPrims::send_text_to_entity(
        security::Context &context,
        const dbtype::Id &target,
        text::ExternalTextLine &text_line,
        const bool throw_on_violation)
    {
        Result result;

        // Basic input checks
        //
        if (target.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Confirm requester is a CPE.
        //
        dbinterface::EntityRef requester_entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                context.get_requester());
        dbtype::ContainerPropertyEntity *cpe_ptr = 0;

        if (requester_entity_ref.valid())
        {
            cpe_ptr = dynamic_cast<dbtype::ContainerPropertyEntity *>(
                requester_entity_ref.get());
        }

        if (not cpe_ptr)
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef target_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(target);

        // Do security check.
        //
        const bool security_success = security::SecurityAccess::instance()->
            security_check(
                security::OPERATION_SEND_TEXT_ENTITY,
                context,
                target_ref,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // Confirm target exists
            //
            if (not target_ref.valid())
            {
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
                return result;
            }

            // Everything is good.  Send the text event.
            //
            events::EventAccess::instance()->publish_event(
                new events::EmitEvent(
                    context.get_requester(),
                    target,
                    dbtype::Id(),
                    text_line,
                    context.get_program(),
                    context.get_pid()));
        }

        text::ExternalText::clear_text_line(text_line);

        return result;
    }
}
}
