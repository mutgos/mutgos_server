/*
 * dbtype_Vehicle.h
 */

#ifndef MUTGOS_DBTYPE_VEHICLE_H
#define MUTGOS_DBTYPE_VEHICLE_H

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
    /**
     * Represents a vehicle.  A Vehicle is a Thing that represents the exterior
     * of a room that can 'move' between rooms, like a car, bus, spaceship, etc.
     */
    class Vehicle : public Thing
    {
    public:
        /**
         * Constructor used for deserialization of a Vehicle.
         */
        Vehicle();

        /**
         * Constructs a Vehicle (final type).
         * @param id[in] The ID of the entity.
         */
        Vehicle(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Vehicle();

        /**
         * Creates a copy of this Vehicle.
         * @param id[in] The new ID of the cloned Vehicle.
         * @param version[in] The new version # of this Vehicle.
         * @param instance[in] The new instance # of this Vehicle.
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
         * Sets the ID referencing the vehicle's interior.
         * @param interior[in] The ID for the vehicle's interior.
         * @param token[in] The lock token.
         * @return True if successfully set, false if error (such as incorrect
         * lock token).
         */
        bool set_vehicle_interior(
            const Id &interior,
            concurrency::WriterLockToken &token);

        /**
         * Sets the ID referencing the vehicle's interior.
         * This method will automatically get a lock.
         * @param interior[in] The ID for the vehicle's interior.
         * @return True if successfully set, false if error.
         */
        bool set_vehicle_interior(
            const Id &interior);

        /**
         * @param token[in] The lock token.
         * @return The ID referencing the vehicle's interior, or default if
         * error.
         */
        Id get_vehicle_interior(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The ID referencing the vehicle's interior, or default if
         * error.
         */
        Id get_vehicle_interior(void);

        /**
         * Sets the ID referencing the vehicle's controller.
         * @param controller[in] The ID for the vehicle's controller.
         * @param token[in] The lock token.
         * @return True if successfully set, false if error (such as incorrect
         * lock token).
         */
        bool set_vehicle_controller(
            const Id &controller,
            concurrency::WriterLockToken &token);

        /**
         * Sets the ID referencing the vehicle's controller.
         * This method will automatically get a lock.
         * @param controller[in] The ID for the vehicle's controller.
         * @return True if successfully set, false if error.
         */
        bool set_vehicle_controller(const Id &controller);

        /**
         * @param token[in] The lock token.
         * @return The ID referencing the vehicle's controller, or default if
         * error.
         */
        Id get_vehicle_controller(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The ID referencing the vehicle's controller, or default if
         * error.
         */
        Id get_vehicle_controller(void);

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
        Vehicle(
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
         * Copies fields from this Vehicle to the provided
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

        Id vehicle_interior;  ///< ID of interior part of vehicle.
        Id vehicle_controller; ///< Who can control this vehicle

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Thing>(*this);

            ar & vehicle_interior;
            ar & vehicle_controller;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Thing>(*this);

            ar & vehicle_interior;
            ar & vehicle_controller;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */


#endif //MUTGOS_DBTYPE_VEHICLE_H
