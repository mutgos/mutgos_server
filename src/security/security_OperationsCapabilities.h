/*
 * security_OperationsCapabilities.h
 */

#ifndef MUTGOS_SECURITY_OPERATIONSCAPABILITIES_H
#define MUTGOS_SECURITY_OPERATIONSCAPABILITIES_H

#include <string>

namespace mutgos
{
namespace security
{
    // TODO When setting 'disabled IDs' for group, the behavior will have to do manual validation if security check fails
    // TODO Figure out how to prevent looking at properties remotely when not allowed, even if have read permission.  'other'?

    //
    // Has all the enums representing possible operations, the result
    // of evaluating the, operation, and special capabilities.
    //
    // NOTE: Remember to keep string version of each enum in synch, in the CPP.
    //

    /**
     * Has all the possible operations the security subsystem can check for.
     * Refer to each operation to determine the correct security checker call.
     */
    enum Operation
    {
        // Do not move from first, used for bounds checking
        /** Temp operation to show all running processes.
            Need context only.
            NOTE: Handled by AdminSecurityChecker for now. */
        OPERATION_GET_FORMATTED_PROCESSES,
        /** Matches a partial string to a name and gets an ID, for Entities in
            the local area of the requester.
            Need context only.
            NOTE: Handled by AcceptAllChecker for now. */
        OPERATION_FIND_BY_NAME_RELATIVE,
        /** Matches a partial or full string to a player/puppet name anywhere
            in the site and gets an ID.
            Need context only. */
        OPERATION_FIND_CHARACTER_BY_NAME,
        /** Determines if the given player/character is currently online, or
            to get a list of currently online players/characters.
            Need Entity target (player to check), or context only if getting
            a list of everyone online. */
        OPERATION_CHARACTER_ONLINE,
        /** Gets what other Entities the given Entity contains.  Also known
            as an 'inventory' in some situations.  Excludes actions.
            Need Entity target. */
        OPERATION_GET_CONTAINS,
        /** Gets what actions the given Entity contains.
            Need Entity target. */
        OPERATION_GET_ACTIONS,
        /** Creates an Entity.
            Need Entity type to create. */
        OPERATION_CREATE_ENTITY,
        /** Deletes an Entity.
            Need Entity target to delete. */
        OPERATION_DELETE_ENTITY,
        /** Gets the contents of a certain Entity field, except Properties.
            When Properties is the field, it means get a LIST of available
            applications, not their contents.
            Need Entity target and the field. */
        OPERATION_GET_ENTITY_FIELD,
        /** Sets the contents of a certain Entity field, except Properties.
            When Properties is the field, it means to add a new application
            property.
            Need Entity target and the field. */
        OPERATION_SET_ENTITY_FIELD,
        /** Gets the Security settings of an application.
            Need Entity target and the application/property name. */
        OPERATION_GET_APPLICATION_SECURITY,
        /** Sets the Security settings of an application.
            Need Entity target and the application/property name. */
        OPERATION_SET_APPLICATION_SECURITY,
        /** Gets a property on an Entity.
            Need Entity target and the application/property to get */
        OPERATION_GET_APPLICATION_PROPERTY,
        /** Sets or deletes a specific property on an Entity.
            Need Entity target and the application/property to set or delete */
        OPERATION_SET_APPLICATION_PROPERTY,
        /** Deletes all properties associated with an application.
            Need Entity target and the application to delete */
        OPERATION_DELETE_APPLICATION_PROPERTY,
        /** Temp operation to do a 'to_string()' on an Entity.
            Need Entity target. */
        OPERATION_ENTITY_TOSTRING,
        /** Transfers an Entity into another Entity, including actions.
            Need Entity target to drop into, entity source is what's being moved. */
        OPERATION_TRANSFER_ENTITY,
        /** Allows a program to broadcast text to a room or rooms without
            prepending an Entity name in front of it.
            Need Entity target where the text will be broadcast. */
        OPERATION_SEND_TEXT_ROOM_UNRESTRICTED,
        /** Allows a program to broadcast text to a room, but must
            prepend the Entity ExternalId to the front.
            Need Entity target where the text will be broadcast. */
        OPERATION_SEND_TEXT_ROOM,
        /** Allows a program to send text to a specific Entity without
            prepending an Entity name in front of it.
            Need Entity target where the text will be sent. */
        OPERATION_SEND_TEXT_ENTITY,
        /** Allows Entity to use/activate an action.
            Need the specific action as the Entity target */
        OPERATION_USE_ACTION,
        /** Do not use; for counting and bounds checking only. */
        OPERATION_END_INVALID
    };

    /**
     * Given an Operation, return it as a string.
     * @param[in] operation The operation to convert.
     * @return operation as a string.
     */
    const std::string &operation_to_string(const Operation operation);

    /**
     * The result of evaluating an operation request.
     */
    enum Result
    {
        // Do not move from first, used for bounds checking
        /** SecurityChecker determines the operation is allowed */
        RESULT_ACCEPT,
        /** SecurityChecker determines the operation is allowed, and overrides
            decision by any other checker.  Priority over deny always is
            based on order of evaluation.  Use very sparingly. */
        RESULT_ACCEPT_ALWAYS,
        /** SecurityChecker determines the operation is denied, and overrides
            decision by any other checker.  Priority over accept always is
            based on order of evaluation.  Use very sparingly. */
        RESULT_DENY_ALWAYS,
        /** SecurityChecker determines the operation is denied. */
        RESULT_DENY,
        /** SecurityChecker is unable to determine if the operation is allowed.
            If all checkers return 'skip', the operation will be denied.
            Otherwise, skips are not factored into the final result.  */
        RESULT_SKIP,
        /** Not valid for use - for counting and boundary checking only */
        RESULT_END_INVALID
    };

    /**
     * Given a result, return it as a string.
     * @param[in] result The result to convert.
     * @return result as a string.
     */
    const std::string &result_to_string(const Result result);

    /**
     * All special capabilities that can be assigned to characters or programs.
     */
    enum Capability
    {
        // Do not move from first, used for bounds checking
        /** Has full permissions to do anything */
        CAPABILITY_ADMIN,
        /** Has permissions to create players */
        CAPABILITY_CREATE_PLAYER,
        /** Has permissions to create most Entity types */
        CAPABILITY_BUILDER,
        /** Can send freeform (any) text to the current room the Entity is in */
        CAPABILITY_SEND_TEXT_ROOM_UNRESTRICTED,
        /** Can send any text to a specific character anywhere in the site */
        CAPABILITY_SEND_TEXT_ENTITY,
        /** Can find any character by name, no matter where they are */
        CAPABILITY_CHARACTER_FIND_BY_NAME_AFAR,
        /** Can convert any ID to name or display name, no matter who owns it
            or where */
        CAPABILITY_ANY_ID_TO_NAME,
        /** Can determine if the character is online */
        CAPABILITY_CONNECTION_CHECK,
        /** Used to indicate program can run as the user running it */
        CAPABILITY_RUN_AS_USER,
        /** Not valid for use - for counting and boundary checking only */
        CAPABILITY_END_INVALID
    };

    /**
     * Given a capability, return it as a string.
     * @param[in] capability The capability to convert.
     * @return capability as a string.
     */
    const std::string &capability_to_string(const Capability capability);
}
}

#endif //MUTGOS_SECURITY_OPERATIONSCAPABILITIES_H
