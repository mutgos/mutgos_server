/**
 * message_LocationInfoChange.h
 */

#ifndef MUTGOS_MESSAGE_LOCATIONINFOCHANGE_H
#define MUTGOS_MESSAGE_LOCATIONINFOCHANGE_H

#include <string>

#include "dbtypes/dbtype_Id.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace message
{
    /**
     * This one-way message is sent to the client whenever some
     * aspect of the conected Player's location changes.  Examples
     * include changing the location entirely, and Entities entering
     * and leaving the room.
     *
     * This message is only partially complete right now; more can be
     * added as needed.
     */
    class LocationInfoChange : public ClientMessage
    {
    public:
        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        LocationInfoChange(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        LocationInfoChange(const LocationInfoChange &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~LocationInfoChange();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * Sets the new room ID.
         * @param room_id[in] The new room ID.
         */
        void set_room_id(const dbtype::Id &room_id)
        { new_room_id = room_id; }

        /**
         * @return The new room ID, or invalid if the room has not changed.
         */
        const dbtype::Id &get_room_id(void) const
        { return new_room_id; }

        /**
         * Sets the new/updated room name.  This is used for efficiency,
         * so the client won't have to immediately send a message back
         * to get this info.
         * If this is left unset when the room ID changes, then it implies
         * the room has no name.
         * @param name[in] The new/updated room name.
         */
        void set_room_name(const std::string &name)
        { new_room_name = name; }

        /**
         * @return The new/updated room name.
         */
        const std::string &get_room_name(void) const
        { return new_room_name; }

        /**
         * Saves this message to the provided document.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * This is normally not used, but is available for debugging/testing.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:
        dbtype::Id new_room_id;  ///< Valid when moved to a new room.  Indicates new location.
        std::string new_room_name; ///< Non-empty when changing room or it renames.  This is the new/updated room name.
    };
}
}

#endif //MUTGOS_MESSAGE_LOCATIONINFOCHANGE_H
