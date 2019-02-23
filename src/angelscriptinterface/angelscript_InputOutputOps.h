/*
 * angelscript_InputOutputOps.h
 */

#ifndef MUTGOS_ANGELSCRIPT_INPUTOUTPUTOPS_H
#define MUTGOS_ANGELSCRIPT_INPUTOUTPUTOPS_H

#include <string>
#include <angelscript.h>

#include "angelscript_AEntity.h"
#include "angelscript_AString.h"

namespace mutgos
{
namespace angelscript
{
    class InputOutputOps
    {
    public:
        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * Using generic interface to get needed engine pointer.
         * This will convert the given string into external text, prepend
         * the requester (if asked), and send to everyone in the given room
         * except for the requester (if present).
         *
         * Actual method signature:
         * void emit_to_room(AEntity *room, AString *text, bool prepend_self);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @see primitives::EventPrims::send_text_to_room() for documentation.
         */
        static void emit_to_room(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         * This will convert the given string into external text, prepend
         * the requester (if asked), and send to everyone in the given room
         * including the requester (if present).
         *
         * Actual method signature:
         * void broadcast_to_room(
         *     AEntity *room,
         *     AString *text,
         *     bool prepend_self);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @see primitives::EventPrims::send_text_to_room() for documentation.
         */
        static void broadcast_to_room(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         * This will convert the given string into external text and send
         * it to the given Entity (typically a Thing, Player, or Puppet).
         *
         * Actual method signature:
         * void send_to_entity(
         *     AEntity *target,
         *     AString *text,
         *     bool prepend_self);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         * @see primitives::EventPrims::send_text_to_entity() for documentation.
         */
        static void send_to_entity(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         * This will convert the given string into external text and send it
         * on the output channel.
         *
         * Actual method signature:
         * void println(AString *text);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         */
        static void println(asIScriptGeneric *gen_ptr);

        /**
         * Using generic interface to get needed engine pointer.
         * This will convert the given string into external text and send it
         * on the output channel.  It assumes the text is multiple lines (CR
         * delineated) but will not process it for formatting codes.
         *
         * Actual method signature:
         * void mprintln(AString *text);
         * @param gen_ptr[in] Generic interface to get and set arguments and
         * return value.
         */
        static void mprintln(asIScriptGeneric *gen_ptr);

    private:

        /**
         * Sends a text event to an Entity.  The raw text will be converted to
         * external text.  If the Entity is a room, it will broadcast it to
         * everyone in the room; other types will get a direct event.
         * @param engine_ptr[in] The AngelScript engine executing the code.
         * @param method[in] The AngelScript method signature which called
         * this method; used for logging and errors.
         * @param entity[in] The Entity to send the text event to.
         * @param entity_is_room[in] True if Entity is a room (to do a
         * broadcast), false otherwise.
         * @param raw_text[in] The text to send.  It will be converted to an
         * ExternalText.
         * @param prepend_self[in] True to prepend self (the requester) in
         * front of the formatted text.
         * @param exclude_requester[in] If sending to a room, true to not
         * also send the text to the requester.  When not sending to a room,
         * this parameter does nothing.
         * @throws exception Throws any exceptions caught by it, after setting
         * the exception info.
         */
        static void send_event(
            asIScriptEngine * const engine_ptr,
            const std::string &method,
            AEntity &entity,
            const bool entity_is_room,
            AString &raw_text,
            const bool prepend_self,
            const bool exclude_requester = true);

        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_INPUTOUTPUTOPS_H
