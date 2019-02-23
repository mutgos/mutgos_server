/*
 * message_ClientExecuteEntity.h
 */

#ifndef MUTGOS_MESSAGE_CLIENTEXECUTEENTITY_H
#define MUTGOS_MESSAGE_CLIENTEXECUTEENTITY_H

#include <string>
#include <vector>

#include "clientmessages/message_ClientMessage.h"

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace message
{
    /**
     * Used by enhanced clients to request an Entity, such as an Action or
     * Program, be execute.
     */
    class ClientExecuteEntity : public ClientMessage
    {
    public:
        typedef std::vector<std::string> ProgramArguments;

        /**
         * Used by the factory to make a new message.
         * @return A new instance of the message.  Caller controls the pointer.
         */
        static ClientMessage *make_instance(void);

        /**
         * Standard constructor.
         */
        ClientExecuteEntity(void);

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ClientExecuteEntity(const ClientExecuteEntity &rhs);

        /**
         * Required virtual destructor.
         */
        virtual ~ClientExecuteEntity();

        /**
         * @return A pointer to a copy of this ClientMessage.  Caller
         * takes ownership of the pointer.
         */
        virtual ClientMessage *clone(void) const;

        /**
         * @return The ID of the Entity to execute.  This ID has not been
         * validated.
         */
        const dbtype::Id &get_entity_id(void) const
          { return entity_id; }

        /**
         * Sets the entity ID to be executed.
         * @param id[in] The Entity ID to be executed.
         */
        void set_entity_id(const dbtype::Id &id)
          { entity_id = id; }

        /**
         * Only used if Entity is a program or an action that runs a program.
         * @return The program arguments, if any.
         */
        const ProgramArguments &get_program_arguments(void) const
        { return program_arguments; }

        /**
         * Adds a program argument to the end of the existing program arguments.
         * Only used if Entity is a program or an action that runs a program.
         * @param argument[in] The argument to add.
         */
        void add_program_argument(const std::string &argument)
          { program_arguments.push_back(argument); }

        /**
         * Only used if Entity is a program or an action that runs a program.
         * Replaces the program arguments with the provided ones.
         * @param arguments[in] The new program arguments.
         */
        void set_program_arguments(const ProgramArguments &arguments)
          { program_arguments = arguments; }

        /**
         * @return The desired Channel subtype name, if a Channel needs to be
         * opened.  Only used when running programs.
         */
        const std::string &get_channel_subtype(void) const
          { return channel_subtype; }

        /**
         * Sets the desired Channel subtype to use, if executing causes a
         * Channel to be opened.  Only used when running programs.
         * @param subtype[in] The desired Channel subtype name.
         */
        void set_channel_subtype(const std::string &subtype)
          { channel_subtype = subtype; }

        /**
         * Saves this message to the provided document.
         * This is normally not used, but is available for debugging/testing.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this message from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

    private:

        dbtype::Id entity_id; ///< Id of Entity to execute
        ProgramArguments program_arguments; ///< Optional arguments if entity is program
        std::string channel_subtype; ///< Subtype of channel if one has to be opened during execution
    };
}
}

#endif //MUTGOS_MESSAGE_CLIENTEXECUTEENTITY_H
