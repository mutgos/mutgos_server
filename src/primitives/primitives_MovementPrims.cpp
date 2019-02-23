/*
 * primitives_MovementPrims.cpp
 */

#include "concurrency/concurrency_WriterLockToken.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_ActionEntity.h"
#include "dbtypes/dbtype_Exit.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Region.h"
#include "dbtypes/dbtype_Room.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "security/security_SecurityException.h"
#include "security/security_SecurityAccess.h"
#include "security/security_Context.h"

#include "events/events_EventAccess.h"
#include "events/events_EmitEvent.h"
#include "events/events_MovementEvent.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalTextConverter.h"
#include "text/text_ExternalIdText.h"

#include "primitives_Result.h"
#include "primitives_MovementPrims.h"

namespace
{
    const std::string SPACE_SEPARATOR = " ";
    const std::string YOU_PREFIX = "You ";
}

namespace mutgos
{
namespace primitives
{
    // -----------------------------------------------------------------------
    MovementPrims::MovementPrims(void)
    {
    }

    // -----------------------------------------------------------------------
    MovementPrims::~MovementPrims()
    {
    }

    // -----------------------------------------------------------------------
    // TODO Will need to be enhanced for puppets
    Result MovementPrims::move_entity(
        security::Context &context,
        const dbtype::Id &entity,
        const dbtype::Id &entity_destination,
        const bool throw_on_violation)
    {
        Result result;

        if (entity.is_default() or entity_destination.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        if (entity.get_entity_id() < 4)
        {
            // The system level rooms and players cannot be moved.
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        if (entity == entity_destination)
        {
            // Cannot move an entity into itself!
            result.set_status(Result::STATUS_IMPOSSIBLE);
            return result;
        }

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();
        dbinterface::EntityRef entity_ref = db_access->get_entity(entity);
        dbinterface::EntityRef destination_ref = db_access->get_entity(
                entity_destination);

        if ((not entity_ref.valid()) or (not destination_ref.valid()))
        {
            // Cannot move anywhere invalid.
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security.
        //
        const bool security_success =
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_TRANSFER_ENTITY,
                context,
                destination_ref,
                entity_ref,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            bool good_to_move = true;
            dbtype::ContainerPropertyEntity * const entity_ptr =
                dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    entity_ref.get());
            dbtype::ActionEntity * const action_entity_ptr =
                dynamic_cast<dbtype::ActionEntity *>(entity_ref.get());
            dbtype::ContainerPropertyEntity * const destination_ptr =
                dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    destination_ref.get());

            if ((not (entity_ptr or action_entity_ptr)) or
                (not destination_ptr))
            {
                // Entity to move and destination must be
                // ContainerPropertyEntities.
                //
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
                return result;
            }

            concurrency::WriterLockToken token(*entity_ref.get());

            // Determine if we're moving the entity to where it already is.
            // We can exit immediately if so.
            //
            if ((entity_ptr and (entity_ptr->get_contained_by(token) ==
                    entity_destination)) or
                (action_entity_ptr and
                    (action_entity_ptr->get_action_contained_by(token) ==
                        entity_destination)))
            {
                return result;
            }

            if (dynamic_cast<dbtype::Room *>(entity_ref.get()))
            {
                // If entity is a Room, it can only be moved between Regions.
                good_to_move =
                    dynamic_cast<dbtype::Region *>(destination_ref.get());
            }
            else if (dynamic_cast<dbtype::Player *>(entity_ref.get()))
            {
                // If Entity is a Player, it can only be moved between rooms.
                good_to_move =
                    dynamic_cast<dbtype::Room *>(destination_ref.get());
            }

            if (not good_to_move)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                // Everything checks out, do the movement and send out the
                // MovementEvent.
                //
                dbtype::Id entity_from;

                if (entity_ptr)
                {
                    entity_from = entity_ptr->get_contained_by(token);
                    entity_ptr->set_contained_by(entity_destination, token);
                }
                else if (action_entity_ptr)
                {
                    entity_from =
                        action_entity_ptr->get_action_contained_by(token);
                    action_entity_ptr->set_action_contained_by(
                        entity_destination,
                        token);
                }

                events::EventAccess::instance()->publish_event(
                    new events::MovementEvent(
                        entity,
                        entity_from,
                        entity_destination,
                        true,
                        context.get_program()));
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    // TODO Maybe send movement event to invalid dbref, make move, then a new movement event for better privacy?
    Result MovementPrims::move_requester_with_exit(
        security::Context &context,
        const dbtype::Id &exit,
        const bool emit_result_messages,
        const bool emit_arrive_messages,
        const bool throw_on_violation)
    {
        Result result;

        if (context.get_requester().is_default() or exit.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();

        dbinterface::EntityRef entity_ref = db_access->get_entity(
            context.get_requester());
        dbinterface::EntityRef exit_ref = db_access->get_entity(exit);


        if ((not entity_ref.valid()) or (not exit_ref.valid()))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        bool destination_good = true;
        dbtype::ContainerPropertyEntity * const entity_ptr =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                entity_ref.get());
        dbtype::Exit * const exit_ptr =
            dynamic_cast<dbtype::Exit *>(exit_ref.get());
        concurrency::WriterLockToken token(*entity_ref.get());
        const std::string &entity_name = entity_ptr->get_entity_name(token);
        const dbtype::Id default_id;

        // Confirm entity is a CPE and exit is really an exit.
        //
        if ((not entity_ptr) or (not exit_ptr))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Get and confirm destination is really a CPE.
        // Confirm destination does not loop back to requester
        // TODO Need to support randomly picking destination if more than one
        //
        dbinterface::EntityRef destination_ref = db_access->get_entity(
            exit_ptr->get_first_action_target());

        if (not destination_ref.valid())
        {
            destination_good = false;
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }

        if (destination_ref.id() == context.get_requester())
        {
            destination_good = false;
            result.set_status(Result::STATUS_IMPOSSIBLE);
        }

        dbtype::ContainerPropertyEntity * const destination_ptr =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                destination_ref.get());

        if (not destination_ptr)
        {
            destination_good = false;
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }

        if (not destination_good)
        {
            // If something is wrong with the destination, emit failure
            // messages and stop here.
            //
            if (emit_result_messages)
            {
                // Emit failure messages before returning.
                //
                const std::string fail_str =
                    exit_ptr->get_action_fail_message();
                const std::string room_fail_str =
                    exit_ptr->get_action_fail_room_message();

                if (not fail_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        fail_str,
                        default_id);
                }

                if (not room_fail_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        room_fail_str,
                        entity_ptr->get_contained_by(token));
                }
            }

            return result;
        }

        // Confirm security
        //
        const bool security_success =
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_USE_ACTION,
                context,
                exit_ref,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);

            if (emit_result_messages)
            {
                // Emit failure messages before returning.
                //
                const std::string fail_str =
                    exit_ptr->get_action_fail_message();
                const std::string room_fail_str =
                    exit_ptr->get_action_fail_room_message();

                if (not fail_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        fail_str,
                        default_id);
                }

                if (not room_fail_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        room_fail_str,
                        entity_ptr->get_contained_by(token));
                }
            }
        }
        else
        {
            // Everything is good to go.  Emit any needed text and do the
            // move.
            //
            if (emit_result_messages)
            {
                const std::string success_str =
                    exit_ptr->get_action_success_message();
                const std::string room_success_str =
                    exit_ptr->get_action_success_room_message();

                if (not success_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        success_str,
                        default_id);
                }

                if (not room_success_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        room_success_str,
                        entity_ptr->get_contained_by(token));
                }
            }

            const dbtype::Id entity_from =
                entity_ptr->get_contained_by(token);
            entity_ptr->set_contained_by(destination_ref.id(), token);

            if (emit_arrive_messages)
            {
                const std::string arrive_str =
                    exit_ptr->get_exit_arrive_message();
                const std::string room_arrive_str =
                    exit_ptr->get_exit_arrive_room_message();

                if (not arrive_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        arrive_str,
                        default_id);
                }

                if (not room_arrive_str.empty())
                {
                    emit_movement_message(
                        context,
                        entity_name,
                        room_arrive_str,
                        destination_ref.id());
                }
            }

            events::EventAccess::instance()->publish_event(
                new events::MovementEvent(
                    context.get_requester(),
                    entity_from,
                    destination_ref.id(),
                    false,
                    exit_ref.id()));
        }

        return result;
    }

    // -----------------------------------------------------------------------
    void MovementPrims::emit_movement_message(
        security::Context &context,
        const std::string &requester_name,
        const std::string &message,
        const mutgos::dbtype::Id &target)
    {
        text::ExternalTextLine text_line;
        text::ExternalTextLine text_line_temp =
            text::ExternalTextConverter::to_external(message);

        if (target.is_default())
        {
            text_line.push_back(new text::ExternalPlainText(YOU_PREFIX));
        }
        else
        {
            text_line.push_back(new text::ExternalIdText(
                context.get_requester(),
                requester_name,
                text::ExternalIdText::ID_TYPE_ENTITY));
            text_line.push_back(new text::ExternalPlainText(SPACE_SEPARATOR));
        }

        text_line.reserve(text_line.size() + text_line_temp.size());
        text_line.insert(
            text_line.end(),
            text_line_temp.begin(),
            text_line_temp.end());
        text_line_temp.clear();

        if (target.is_default())
        {
            events::EventAccess::instance()->publish_event(
                new events::EmitEvent(
                    context.get_requester(),
                    context.get_requester(),
                    dbtype::Id(),
                    text_line,
                    context.get_program(),
                    context.get_pid()));
        }
        else
        {
            events::EventAccess::instance()->publish_event(
                new events::EmitEvent(
                    context.get_requester(),
                    target,
                    context.get_requester(),
                    text_line,
                    context.get_program(),
                    context.get_pid()));
        }
    }
}
}
