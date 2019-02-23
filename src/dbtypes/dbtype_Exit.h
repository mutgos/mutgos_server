/*
 * dbtype_Exit.h
 */

#ifndef MUTGOS_DBTYPE_EXIT_H
#define MUTGOS_DBTYPE_EXIT_H

#include <string>

#include "dbtype_Entity.h"
#include "dbtype_ActionEntity.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents an Exit, which transports entities to the room the Exit
     * is linked to, when the name/command is typed.
     */
    class Exit : public ActionEntity
    {
    public:
        /**
         *  Constructor used for deserialization of an Exit.
         */
        Exit();

        /**
         * Constructs an Exit (final type).
         * @param id[in] The ID of the entity.
         */
        Exit(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Exit();

        /**
         * Creates a copy of this Exit.
         * @param id[in] The new ID of the cloned Exit.
         * @param version[in] The new version # of this Exit.
         * @param instance[in] The new instance # of this Exit.
         * @param token[in] The lock token.
         * @return The clone as a pointer.  Caller must manage the pointer!
         * Null is returned if there is an error, such as an incorrect
         * lock token.
         */
        virtual Entity *clone(
            const Id &id,
            const VersionType version,
            const InstanceType instance,
            concurrency::ReaderLockToken &token);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);

        /**
         * Sets the message seen when entering a room via an exit.
         * @param message[in] The message.
         * @param token[in] The lock token.
         * @return True if success, false if error.
         */
        bool set_exit_arrive_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the message seen when entering a room via an exit.
         * This method automatically gets a lock.
         * @param message[in] The message.
         * @return True if success, false if error.
         */
        bool set_exit_arrive_message(
            const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The message seen when entering a room via an exit, or
         * empty if none or error.
         */
        std::string get_exit_arrive_message(concurrency::ReaderLockToken &token);

        /**
         * This method automatically gets a lock.
         * @return The message seen when entering a room via an exit, or
         * empty if none or error.
         */
        std::string get_exit_arrive_message(void);

        /**
         * Sets the message seen by others when entering a room via an exit.
         * @param message[in] The message.
         * @param token[in] The lock token.
         * @return True if success, false if error.
         */
        bool set_exit_arrive_room_message(
            const std::string &message,
            concurrency::WriterLockToken &token);

        /**
         * Sets the message seen by others when entering a room via an exit.
         * This method automatically gets a lock.
         * @param message[in] The message.
         * @return True if success, false if error.
         */
        bool set_exit_arrive_room_message(
            const std::string &message);

        /**
         * @param token[in] The lock token.
         * @return The message seen by others when entering a room via an exit,
         * or empty if none or error.
         */
        std::string get_exit_arrive_room_message(
            concurrency::ReaderLockToken &token);

        /**
         * This method automatically gets a lock.
         * @return The message seen by others when entering a room via an exit,
         * or empty if none or error.
         */
        std::string get_exit_arrive_room_message(void);

    protected:
        /**
         * Constructs an Entity with a provided type.  Used by subclasses.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        Exit(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error.
         */
        virtual size_t mem_used_fields(void);

        /**
         * Copies fields from this Exit to the provided Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:

        std::string exit_arrive_message; ///< Shown to Entity when arrived in room
        std::string exit_arrive_room_message; ///< Shown to room when Entity arrives

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<ActionEntity>(*this);

            ar & exit_arrive_message;
            ar & exit_arrive_room_message;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<ActionEntity>(*this);

            ar & exit_arrive_message;
            ar & exit_arrive_room_message;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_EXIT_H
