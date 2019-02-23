/*
 * primitives_EventPrims.h
 */

#ifndef MUTGOS_PRIMITIVES_EVENTPRIMS_H
#define MUTGOS_PRIMITIVES_EVENTPRIMS_H

#include "dbtypes/dbtype_Id.h"

#include "security/security_Context.h"

#include "text/text_ExternalText.h"

#include "primitives_Result.h"

namespace mutgos
{
namespace primitives
{
    /**
     * Primitives that relate to sending out events (such as sending text to a
     * room) will be here.
     */
    class EventPrims
    {
    public:
        /**
         * Constructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        EventPrims(void);

        /**
         * Destructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        ~EventPrims();

        /**
         * Sends the given text line to the room the requester is in.
         * @param context[in] The execution context.
         * @param room[in] The room to send the text to.
         * @param text_line[in] The formatted text to send.  Ownership of the
         * text will transfer to this method if success; text_line will then
         * be cleared.
         * @param exclude_requester[in] True if the requester should not see
         * the text being sent to the room.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result send_text_to_room(
            security::Context &context,
            const dbtype::Id &room,
            text::ExternalTextLine &text_line,
            const bool exclude_requester,
            const bool throw_on_violation = true);

        /**
         * Sends the given text line to the given Entity target.
         * @param context[in] The execution context.
         * @param target[in] The Entity to send the text to.
         * @param text_line[in] The formatted text to send.  Ownership of the
         * text will transfer to this method; text_line will be cleared.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result send_text_to_entity(
            security::Context &context,
            const dbtype::Id &target,
            text::ExternalTextLine &text_line,
            const bool throw_on_violation = true);
    };
}
}

#endif //MUTGOS_PRIMITIVES_EVENTPRIMS_H
