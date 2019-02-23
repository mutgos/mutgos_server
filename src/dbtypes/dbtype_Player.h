/*
 * Player.h
 */

#ifndef MUTGOS_DBTYPE_PLAYER_H_
#define MUTGOS_DBTYPE_PLAYER_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"
#include "dbtype_ContainerPropertyEntity.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a player - basically a user account.  Users can log in and
     * be represented by this Entity.  Players are the only Entities that can
     * own other Entities.
     */
    class Player : public ContainerPropertyEntity
    {
    public:

        /**
         * Constructor used for deserialization of a Player.
         */
        Player();

        /**
         * Constructs a Player (final type).
         * @param id[in] The ID of the entity.
         */
        Player(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Player();

        /**
         * Creates a copy of this Player.
         * @param id[in] The new ID of the cloned Capability.
         * @param version[in] The new version # of this Capability.
         * @param instance[in] The new instance # of this Capability.
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
         * Sets the password.  The password will be encrypted before storing.
         * @param new_password[in] The new, unencrypted password.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        virtual bool set_password(
            const std::string &new_password,
            concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * Sets the password.  The password will be encrypted before storing.
         * @param new_password[in] The new, unencrypted password.
         * @return True if successfully set.
         */
        bool set_password(const std::string &new_password);

        /**
         * Checks to see if the provided unencrypted password matches the
         * encypted version.  Used for login checks.
         * @param password[in] The unencrypted password to compare the
         * stored password against.
         * @param token[in] The lock token.
         * @return True if the passwords match, false if not or error.
         */
        virtual bool check_password(
            const std::string &password,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Checks to see if the provided unencrypted password matches the
         * encypted version.  Used for login checks.
         * @param password[in] The unencrypted password to compare the
         * stored password against.
         * @return True if the passwords match, false if not or error.
         */
        bool check_password(const std::string &password);

        /**
         *  Sets the display name of a Player.  This is used in preference of
         *  the entity name when possible.
         *  @param name[in] The display name to set.
         *  @param lock[in] The lock token.
         *  @return True if successfully set.
         */
        virtual bool set_display_name(
            const std::string &name,
            concurrency::WriterLockToken &token);

        /**
         *  This method will automatically get a lock.
         *  Sets the display name of a Player.  This is used in preference of
         *  the entity name when possible.
         *  @param name[in] The display name to set.
         *  @return True if successfully set.
         */
        bool set_display_name(const std::string &name);

        /**
         * @param fallback[in] If true, use Entity name if display name is
         * empty.
         * @param lock[in] The lock token.
         * @return The display name, or empty string if error.
         */
        std::string get_display_name(
            const bool fallback,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param fallback[in] If true, use Entity name if display name is
         * empty.
         * @return The display name, or empty string if error.
         */
        std::string get_display_name(const bool fallback);

        /**
         * Sets the home for the player.  This is where they go when
         * 'sent home'.
         * @param home[in] The Entity the player goes home to.
         * @param lock[in] The lock token.
         * @return True if success.
         */
        virtual bool set_player_home(
            const Id &home,
            concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * Sets the home for the player.  This is where they go when
         * 'sent home'.
         * @param home[in] The Entity the player goes home to.
         * @return True if success.
         */
        bool set_player_home(const Id &home);

        /**
         * @param lock[in] The lock token.
         * @return The Player's home.
         */
        Id get_player_home(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The Player's home.
         */
        Id get_player_home(void);

        /**
         * Sets the time the player last connected to 'now'.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_last_connect_to_now(concurrency::WriterLockToken &token);

        /**
         * Sets the time the player last connected to 'now'.
         * This method will automatically get a lock.
         * @return True if successfully set.
         */
        bool set_last_connect_to_now(void);

        /**
         * @param token[in] The lock token.
         * @return The time the player last connected.  Will return a
         * default (0) time if player has never connected or error.
         */
        TimeStamp get_last_connect(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The time the player last connected.  Will return a
         * default (0) time if player has never connected or error.
         */
        TimeStamp get_last_connect(void);

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
        Player(
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
         * Copies fields from this Player to the provided
         * Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:

        std::string encrypted_password; ///< Password in encrypted form
        std::string display_name; ///< Nickname for the player
        Id player_home; ///< Where player goes if sent home
        TimeStamp player_last_connect; ///< When player last connected

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);

            ar & encrypted_password;
            ar & display_name;
            ar & player_home;
            ar & player_last_connect;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<ContainerPropertyEntity>(*this);

            ar & encrypted_password;
            ar & display_name;
            ar & player_home;
            ar & player_last_connect;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_PLAYER_H
