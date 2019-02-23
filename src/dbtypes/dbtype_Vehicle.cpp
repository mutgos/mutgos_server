/*
 * dbtype_Vehicle.cpp
 */

#include <string>
#include <iostream>

#include "dbtypes/dbtype_Vehicle.h"
#include "dbtypes/dbtype_Thing.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Vehicle::Vehicle()
      : Thing()
    {
    }

    // ----------------------------------------------------------------------
    Vehicle::Vehicle(const Id &id)
        : Thing(id, ENTITYTYPE_vehicle, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Vehicle::~Vehicle()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Vehicle::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Vehicle(
                id,
                ENTITYTYPE_vehicle,
                version,
                instance);

            copy_fields(copy_ptr);

            return copy_ptr;
        }
        else
        {
            LOG(error, "dbtype", "clone",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // ----------------------------------------------------------------------
    std::string Vehicle::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << Thing::to_string()
                  << "Vehicle interior: " << vehicle_interior.to_string()
                  << std::endl
                  << "Vehicle controller: " << vehicle_controller.to_string()
                  << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Vehicle::set_vehicle_interior(
        const Id &interior,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            set_single_id_field(
                ENTITYFIELD_vehicle_interior,
                vehicle_interior,
                interior);
            vehicle_interior = interior;
            notify_field_changed(ENTITYFIELD_vehicle_interior);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_vehicle_interior",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Vehicle::set_vehicle_interior(const Id &interior)
    {
        concurrency::WriterLockToken token(*this);

        return set_vehicle_interior(interior, token);
    }

    // ----------------------------------------------------------------------
    Id Vehicle::get_vehicle_interior(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = vehicle_interior;
        }
        else
        {
            LOG(error, "dbtype", "get_vehicle_interior",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Vehicle::get_vehicle_interior(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_vehicle_interior(token);
    }

    // ----------------------------------------------------------------------
    bool Vehicle::set_vehicle_controller(
        const Id &controller,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            set_single_id_field(
                ENTITYFIELD_vehicle_controller,
                vehicle_controller,
                controller);
            vehicle_controller = controller;
            notify_field_changed(ENTITYFIELD_vehicle_controller);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_vehicle_controller",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Vehicle::set_vehicle_controller(const Id &controller)
    {
        concurrency::WriterLockToken token(*this);

        return set_vehicle_controller(controller, token);
    }

    // ----------------------------------------------------------------------
    Id Vehicle::get_vehicle_controller(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = vehicle_controller;
        }
        else
        {
            LOG(error, "dbtype", "get_vehicle_controller",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Vehicle::get_vehicle_controller(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_vehicle_controller(token);
    }

    // ----------------------------------------------------------------------
    Vehicle::Vehicle(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : Thing(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    size_t Vehicle::mem_used_fields(void)
    {
        size_t total_memory = Thing::mem_used_fields();

        total_memory += vehicle_interior.mem_used() + vehicle_controller.mem_used();

        return total_memory;
    }

    // ----------------------------------------------------------------------
    void Vehicle::copy_fields(Entity *entity_ptr)
    {
        Thing::copy_fields(entity_ptr);

        Vehicle *cast_ptr = 0;

        // Only copy if this is also a Vehicle.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Vehicle *>(entity_ptr))) != 0))
        {
            cast_ptr->set_single_id_field(
                ENTITYFIELD_vehicle_interior,
                cast_ptr->vehicle_interior,
                vehicle_interior);
            cast_ptr->vehicle_interior = vehicle_interior;
            cast_ptr->notify_field_changed(ENTITYFIELD_vehicle_interior);

            cast_ptr->set_single_id_field(
                ENTITYFIELD_vehicle_controller,
                cast_ptr->vehicle_controller,
                vehicle_controller);
            cast_ptr->vehicle_controller = vehicle_controller;
            cast_ptr->notify_field_changed(ENTITYFIELD_vehicle_controller);
        }
    }

} /* namespace dbtype */
} /* namespace mutgos */