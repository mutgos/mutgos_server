    /*
 * dbtype_ContainerPropertyEntity.h
 */

#ifndef DBTYPE_CONTAINERPROPERTYENTITY_H_
#define DBTYPE_CONTAINERPROPERTYENTITY_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Group.h"
#include "dbtypes/dbtype_PropertyEntity.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"


namespace mutgos
{
namespace dbtype
{
    // TODO: Roll up 'contained by' call into existing references??
    // TODO: Shouldn't 'home' be here instead of subclasses?

    /**
     * An Entity that can have properties, and can also 'contain' other
     * entities and have programs associated with it.
     */
    class ContainerPropertyEntity : public PropertyEntity
    {
    public:
        /**
         * Constructor used for deserialization of a ContainerPropertyEntity.
         */
        ContainerPropertyEntity();

        /**
         * Constructs a ContainerPropertyEntity (final type).
         * @param id[in] The ID of the entity.
         */
        ContainerPropertyEntity(const Id &id);

        /**
         * Destructor.
         */
        virtual ~ContainerPropertyEntity();

        /**
         * Creates a copy of this ContainerPropertyEntity.
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
         * Sets what ContainerPropertyEntity contains this one.
         * @param id[in] The ID of the ContainerPropertyEntity that contains
         * this one.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_contained_by(
            const Id &id,
            concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * Sets what ContainerPropertyEntity contains this one.
         * @param id[in] The ID of the ContainerPropertyEntity that contains
         * this one.
         * @return True if successfully set.
         */
        bool set_contained_by(const Id &id);

        /**
         * @param token[in] The lock token.
         * @return The ID of the ContainerPropertyEntity that contains this
         * one, or default if error or not set.
         */
        Id get_contained_by(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The ID of the ContainerPropertyEntity that contains this
         * one.
         */
        Id get_contained_by(void);

        /**
         * Adds a program to the set of programs associated with this
         * ContainerPropertyEntity.
         * @param id[in] The program to add.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool add_program(const Id &id, concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * Adds a program to the set of programs associated with this
         * ContainerPropertyEntity.
         * @param id[in] The program to add.
         * @return True if success.
         */
        bool add_program(const Id &id);

        /**
         * Removes a program from the set of programs associated with this
         * ContainerPropertyEntity.
         * @param id[in] The program to remove.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool remove_program(const Id &id, concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * Removes a program from the set of programs associated with this
         * ContainerPropertyEntity.
         * @param id[in] The program to remove.
         * @return True if success.
         */
        bool remove_program(const Id &id);

        /**
         * Determines if a program is associated (linked) with this
         * ContainerPropertyEntity.
         * @param id[in] The program to check for.
         * @param token[in] The lock token.
         * @return True if associated/linked, false if not or error.
         */
        bool is_program_linked(
            const Id &id,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Determines if a program is associated (linked) with this
         * ContainerPropertyEntity.
         * @param id[in] The program to check for.
         * @return True if associated/linked, false if not or error.
         */
        bool is_program_linked(
            const Id &id);

        /**
         * @param token[in] The lock token.
         * @return The number of linked programs, or 0 if none or error.
         */
        size_t linked_programs_size(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The number of linked programs, or 0 if none or error.
         */
        size_t linked_programs_size(void);

        /**
         * Gets a copy of the associated/linked programs in this
         * ContainerPropertyEntity.
         * @param token[in] The lock token.
         * @return A copy of the linked programs or empty if error.
         */
        IdVector get_linked_programs(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Gets a copy of the associated/linked programs in this
         * ContainerPropertyEntity.
         * @return A copy of the linked programs or empty if error.
         */
        IdVector get_linked_programs(void);

        /**
         * Gets the first associated/linked program for this
         * ContainerPropertyEntity.
         * @param token[in] The lock token.
         * @return The first program associated/linked with this, or a
         * default ID if error or no linked programs.
         */
        Id get_first_linked_program(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Gets the first associated/linked program for this
         * ContainerPropertyEntity.
         * @return The first program associated/linked with this.
         */
        Id get_first_linked_program(void);

        /**
         * Gets the next associated/linked program for this
         * ContainerPropertyEntity.
         * @param id[in] The position in the list.  The ID after this one will
         * be returned.
         * @param token[in] The lock token.
         * @return The next program associated/linked with this after id, or
         * a default ID if the end, not found, or error.
         */
        Id get_next_linked_program(
            const Id &id,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Gets the next associated/linked program for this
         * ContainerPropertyEntity.
         * @param id[in] The position in the list.  The ID after this one will
         * be returned.
         * @return The next program associated/linked with this after id, or
         * a default ID if the end or not found.
         */
        Id get_next_linked_program(
            const Id &id);

        /**
         * Gets the last associated/linked program for this
         * ContainerPropertyEntity.
         * @param token[in] The lock token.
         * @return The last program associated/linked with this.
         */
        Id get_last_linked_program(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * Gets the last associated/linked program for this
         * ContainerPropertyEntity.
         * @return The last program associated/linked with this.
         */
        Id get_last_linked_program(void);

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
        ContainerPropertyEntity(
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
         * Copies fields from this ContainerPropertyEntity to the provided
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

        Id contained_by; ///< Who contains this instance
        IdSet linked_programs; ///< Programs linked into this instance

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & contained_by;
            ar & linked_programs;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & contained_by;
            ar & linked_programs;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_CONTAINERPROPERTYENTITY_H_ */
