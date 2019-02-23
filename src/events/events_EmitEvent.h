/*
 * events_EmitEvent.h
 */

#ifndef MUTGOS_EVENTS_EMITEVENT_H
#define MUTGOS_EVENTS_EMITEVENT_H

#include "events/events_Event.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"

#include "text/text_ExternalText.h"
#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents when an Entity emits text.  This can be due to 'say'ing,
     * posing, private messages, etc.
     *
     * The emit destination is typically a player / puppet or a room.  If it's
     * a room, it's considered a broadcast.
     */
    class EmitEvent : public Event
    {
    public:
        /**
         * Constructor that sets everything.
         * @param source[in] The source of the text.
         * @param target[in] The destination of the text.
         * @param exclude[in] An Entity that would normally be eligible to
         * receive text in this situation, but is to be excluded.  May be
         * default if N/A.
         * @param text[in,out] The actual text being emitted.  Ownership of the
         * pointers will be transferred to this class instance.  The contents
         * of text WILL BE CLEARED to reinforce this.
         * @param program[in] The program which created the emit event, or
         * default for 'native'.
         * @param program_pid[in] The PID of the program which created the
         * event, or 0 if MUTGOS internal.
         */
        EmitEvent(
            const dbtype::Id &source,
            const dbtype::Id &target,
            const dbtype::Id &exclude,
            text::ExternalTextLine &text,
            const dbtype::Id &program,
            const executor::PID &program_pid)
            : Event(Event::EVENT_EMIT),
              emit_source(source),
              emit_target(target),
              emit_exclude(exclude),
              emit_text(text),
              emit_program(program),
              emit_program_pid(program_pid),
              emit_timestamp(true)
          { text.clear(); }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        EmitEvent(const EmitEvent &rhs)
          : Event(rhs),
            emit_source(rhs.emit_source),
            emit_target(rhs.emit_target),
            emit_exclude(rhs.emit_exclude),
            emit_program(rhs.emit_program),
            emit_program_pid(rhs.emit_program_pid),
            emit_timestamp(rhs.emit_timestamp)
        { emit_text = text::ExternalText::clone_text_line(rhs.emit_text); }

        /**
         * Mandatory virtual destructor that also cleans up the emit text.
         */
        virtual ~EmitEvent()
          { text::ExternalText::clear_text_line(emit_text); }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return The source of the emit text.
         */
        const dbtype::Id &get_source(void) const
          { return emit_source; }

        /**
         * @return The target (destination) of the emit text.
         */
        const dbtype::Id &get_target(void) const
          { return emit_target; }

        /**
         * @return An Entity that would normally be eligible to
         * receive text in this situation, but is to be excluded.  May be
         * default if N/A.
         */
        const dbtype::Id &get_exclude(void) const
          { return emit_exclude; }

        /**
         * @return The text being admitted.  Do not attempt to delete pointers.
         * Pointers remained owned by this class; clone them if a permanent
         * copy is desired.
         */
        const text::ExternalTextLine &get_text(void) const
          { return emit_text; }

        /**
         * @return The entity ID of the program that generated this event,
         * or default for a 'native' program.
         */
        const dbtype::Id &get_program_id(void) const
          { return emit_program; }

        /**
         * @return The PID of the program that generated this event,
         * or 0 (invalid) if MUTGOS internal (rare).
         */
        const executor::PID get_program_pid(void) const
          { return emit_program_pid; }

        /**
         * @return The timestamp of when the event was generated.
         */
        const dbtype::TimeStamp &get_timestamp(void) const
          { return emit_timestamp; }

    private:
        const dbtype::Id emit_source; ///< The source of the text.
        const dbtype::Id emit_target; ///< The destination of the text.
        const dbtype::Id emit_exclude; ///< When target is a room, exclude this Entity from getting text
        text::ExternalTextLine emit_text; ///< The actual text.
        const dbtype::Id emit_program; ///< The program that created this event, or default for native.
        const executor::PID emit_program_pid; ///< The PID of the program that created this event, or 0 for MUTGOS internal.
        const dbtype::TimeStamp emit_timestamp; ///< When this event was created.
    };
}
}

#endif //MUTGOS_EVENTS_EMITEVENT_H
