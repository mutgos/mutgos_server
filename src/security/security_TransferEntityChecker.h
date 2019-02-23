/*
 * security_TransferEntityChecker.h
 */

#ifndef MUTGOS_SECURITY_TRANSFERENTITY_H
#define MUTGOS_SECURITY_TRANSFERENTITY_H

// TODO Will need to add locks for taking things
// TODO This will probably need improvement (for instance, 'throwing' an object player to player).

#include "security_SecurityChecker.h"

namespace mutgos
{
namespace security
{
    /**
     * Checks the security for transferring an Entity to another room or
     * Entity (such as picking something up, dropping it, transferring
     * Player to Player, etc).
     *
     * Allow transfer if:
     *  From room to non-room container Entity (or vice versa):
     *    Always allowed if in same room.
     *    TODO Must pass lock if moving from room and not admin
     *    Only allowed if in different rooms if context is owner/admin of
     *        Entity being moved (or container), and destination Entity.
     *
     * From room to room:
     *    Only allowed if context is owner/admin of both rooms.
     *
     *  From container Entity to container Entity (neither are rooms, one is
     *  requester):
     *    Only allowed if owner/admin of Entity to move and destination.
     *
     *  Due to security risks, the checker does not support directly 'handing'
     *  something from Entity to Entity.  This can be implemented using
     *  privileged softcode.
     */
    class TransferEntityChecker : public SecurityChecker
    {
    public:
        /**
         * Constructor.
         */
        TransferEntityChecker(void);

        /**
         * Required destructor.
         */
        virtual ~TransferEntityChecker();

        /**
         * Performs a security check with the given parameters.
         * @param operation[in] The operation to check.
         * @param context[in] The context the check is made in.
         * @param entity_target[in] The destination or target Entity being
         * checked.
         * @param entity_source[in] The source of the entity_target, or the
         * source Entity being moved to entity_target.
         * @return The result of the security check.
         */
        virtual Result security_check(
            const Operation operation,
            Context &context,
            dbinterface::EntityRef &entity_target,
            dbinterface::EntityRef &entity_source);

    private:
        /**
         * Determines if the requester is the owner/admin of both entities
         * provided.
         * @param context[in] The context the check is made in.
         * @param requester[in] The requester's Entity.
         * @param program[in] The program being ran.
         * @param first[in] First Entity to check.
         * @param second[in] Second Entity to check.
         * @return The result of the security check.
         */
        Result check_for_owner(
            Context &context,
            dbinterface::EntityRef &requester,
            dbinterface::EntityRef &program,
            dbinterface::EntityRef &first,
            dbinterface::EntityRef &second);

        /**
         * Determines if the requester can move an Entity to or from a room
         * that it is located in.
         * It must be guaranteed that to_move's container and the target are
         * not both rooms, however one of them must be a room (just not both).
         * All inputs are assumed valid.
         * @param context[in] The context the check is made in.
         * @param requester[in] The requester's Entity.
         * @param program[in] The program being ran.
         * @param to_move[in] The Entity being moved.
         * @param to_move_container[in] The Entity's current container.
         * @param to_move_container_is_room[in] True if Entity's
         * container is a room.
         * @param target[in] Where to move the Entity to.
         * @param target_is_room[in] True if Entity's destination is a room.
         * @return The result of the security check.
         */
        Result check_room_to_entity(
            Context &context,
            dbinterface::EntityRef &requester,
            dbinterface::EntityRef &program,
            dbinterface::EntityRef &to_move,
            dbinterface::EntityRef &to_move_container,
            const bool to_move_container_is_room,
            dbinterface::EntityRef &target,
            const bool target_is_room);

    };
}
}

#endif //MUTGOS_SECURITY_TRANSFERENTITY_H
