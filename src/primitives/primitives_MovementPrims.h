/*
 * primitives_MovementPrims.h
 */

#ifndef MUTGOS_PRIMITIVES_MOVEMENTPRIMS_H
#define MUTGOS_PRIMITIVES_MOVEMENTPRIMS_H

#include "dbtypes/dbtype_Id.h"
#include "security/security_Context.h"

#include "primitives_Result.h"


namespace mutgos
{
namespace primitives
{
    /**
     * Primitives relating to moving Entities around (usually
     * ContainerPropertyEntities)) will be here.
     */
    class MovementPrims
    {
    public:
        /**
         * Constructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        MovementPrims(void);

        /**
         * Destructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        ~MovementPrims();

        /**
         * Used when moving entities when not going through an action, such as
         * with get/drop/teleport.  This will transfer the Entity and send out
         * any needed events.
         * Note this will NOT emit any text indicating an Entity has been
         * moved; that is the job of the program doing the moving.
         * @param context[in] The security context.
         * @param entity[in] The Entity to move.
         * @param entity_destination[in] Where the Entity should be moved to.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result move_entity(
            security::Context &context,
            const dbtype::Id &entity,
            const dbtype::Id &entity_destination,
            const bool throw_on_violation = true);

        /**
         * Used when an Entity uses an Exit to move to another room.  This will
         * handle security checks, moving the Entity, and optionally emitting
         * the success, failure, and arrive  messages as present.
         * @param context[in] The security context.
         * @param exit[in] The exit for the requester to use.
         * @param emit_result_messages[in] If true, emit success and failure
         * messages to the requester and originating room if messages are
         * present. Note that if throw_on_violation is true and the security
         * check fails, no messages will be seen.
         * @param emit_arrive_messages[in] If true, emit the arrival messages
         * to the requester and destination room if messages are present.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result move_requester_with_exit(
            security::Context &context,
            const dbtype::Id &exit,
            const bool emit_result_messages,
            const bool emit_arrive_messages,
            const bool throw_on_violation = true);

    private:

        /**
         * Emits the given message to either a room or the requester and
         * applies the appropriate prefix.  This is used to emit
         * success or failure messages either to the requester or room.
         * @param context[in] The security context.
         * @param requester_name[in] The name of the requester in the security
         * context.
         * @param message[in] The message to output.  The appropriate prefix
         * will be automatically added.
         * @param target[in] What room receives the message, or default if
         * only the requester is to see the message.
         */
        void emit_movement_message(
            security::Context &context,
            const std::string &requester_name,
            const std::string &message,
            const dbtype::Id &target);

    };
}
}

#endif //MUTGOS_PRIMITIVES_MOVEMENTPRIMS_H
