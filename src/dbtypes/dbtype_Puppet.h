/*
 * dbtype_Puppet.h
 */

#ifndef MUTGOS_DBTYPE_PUPPET_H
#define MUTGOS_DBTYPE_PUPPET_H

#include <string>

#include "dbtypes/dbtype_Thing.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Lock.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"


namespace mutgos
{
namespace dbtype
{
    // TODO: Make a parent class shared by Puppet and Player for display name, etc.

    /**
     * Represents a puppet.  A puppet is a Thing that in some respects
     * acts like a player, however a player must control it.
     */
    class Puppet : public Thing
    {
    public:
        /**
         * Constructor used for deserialization of a Puppet.
         */
        Puppet();

        /**
         * Constructs a Puppet (final type).
         * @param id[in] The ID of the entity.
         */
        Puppet(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Puppet();

        /**
         * Creates a copy of this Puppet.
         * @param id[in] The new ID of the cloned Puppet.
         * @param version[in] The new version # of this Puppet.
         * @param instance[in] The new instance # of this Puppet.
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
         * Sets the Puppet's name.
         * @param name[in] The name of the Puppet.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        virtual bool set_entity_name(
            const std::string &name,
            concurrency::WriterLockToken &token);

        /**
         * Sets the puppet display name.
         * @param name[in] The puppet display name.
         * @param token[in] The lock token.
         * @return True if successfully set, false if error (such as incorrect
         * lock token).
         */
        bool set_puppet_display_name(
            const std::string &name,
            concurrency::WriterLockToken &token);

        /**
         * Sets the puppet display name.
         * This method will automatically get a lock.
         * @param name[in] The puppet display name.
         * @return True if successfully set, false if error.
         */
        bool set_puppet_display_name(
            const std::string &name);

        /**
         * @param token[in] The lock token.
         * @return The puppet display name, or empty string if error.
         */
        std::string get_puppet_display_name(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The puppet display name.
         */
        std::string get_puppet_display_name(void);

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
        Puppet(
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
         * Copies fields from this Puppet to the provided
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

        std::string puppet_display_name; ///< Puppet display name

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Thing>(*this);

            ar & puppet_display_name;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Thing>(*this);

            ar & puppet_display_name;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_PUPPET_H
