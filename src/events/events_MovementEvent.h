/*
 * events_MovementEvent.h
 */

#ifndef MUTGOS_EVENTS_MOVEMENTEVENT_H
#define MUTGOS_EVENTS_MOVEMENTEVENT_H

#include "dbtypes/dbtype_Id.h"

#include "events/events_Event.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents when an Entity moves within a site.  Movement is defined
     * as any time the ID of the Container holding the Entity has changed.
     */
    class MovementEvent : public Event
    {
    public:
        /**
         * Constructor that sets everything.
         * @param who[in] Which Entity is being moved.
         * @param from[in] Where is Entity being moved from, or default if
         * Entity is new and just placed in a container.
         * @param to[in] Where is Entity being moved to
         * @param program[in] True if a program/process of any sort is moving
         * the Entity, or false if an exit.
         * @param how[in] How did the Entity move?  If program is true, this
         * is the ID of the program that did the move (or default if
         * native/internal MUTGOS).  If program is false, this would be the
         * exit that moved it.
         */
        MovementEvent(
            const dbtype::Id &who,
            const dbtype::Id &from,
            const dbtype::Id &to,
            const bool program,
            const dbtype::Id &how)
            : Event(Event::EVENT_MOVEMENT),
              movement_who(who),
              movement_from(from),
              movement_to(to),
              movement_via_program(program),
              movement_how(how)
          { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        MovementEvent(const MovementEvent &rhs)
            : Event(rhs),
              movement_who(rhs.movement_who),
              movement_from(rhs.movement_from),
              movement_to(rhs.movement_to),
              movement_via_program(rhs.movement_via_program),
              movement_how(rhs.movement_how)
          { }

        /**
         * Required virtual destructor.
         */
        virtual ~MovementEvent()
          { }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return Which Entity is being moved.
         */
        const dbtype::Id &get_who(void) const
          { return movement_who; }

        /**
         * @return Where is Entity being moved from, or default if
         * Entity is new and just placed in a container.
         */
        const dbtype::Id &get_from(void) const
          { return movement_from; }

        /**
         * @return Where is Entity being moved to
         */
        const dbtype::Id &get_to(void) const
          { return movement_to; }

        /**
         * @return True if a program/process of any sort of moving
         * the Entity, or false if an exit.
         */
        const bool get_program_flag(void) const
          { return movement_via_program; }

        /**
         * @return How did the Entity move?  If program flag is true, this
         * is the ID of the program that did the move (or default if
         * native/internal MUTGOS).  If program flag is false, this would be
         * the exit that moved it.
         */
        const dbtype::Id &get_how(void) const
          { return movement_how; }

    private:
        const dbtype::Id movement_who; ///< Which Entity moved
        const dbtype::Id movement_from; ///< Where did Entity move from
        const dbtype::Id movement_to; ///< Where is Entity moving to
        const bool movement_via_program; ///< True if a program moved the Entity
        const dbtype::Id movement_how; ///< The program or exit that moved the Entity
    };
}
}

#endif //MUTGOS_EVENTS_MOVEMENTEVENT_H
