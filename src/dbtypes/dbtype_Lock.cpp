/*
 * Lock.cpp
 */

#include <string>
#include <ostream>

#include "logging/log_Logger.h"

#include "dbtype_Id.h"
#include "dbtype_Entity.h"
#include "dbtype_Group.h"
#include "dbtype_PropertyEntity.h"
#include "dbtype_PropertyData.h"
#include "dbtype_PropertyDataSerializer.h"
#include "dbtype_PropertyDirectory.h"
#include "dbtype_BooleanProperty.h"

#include "concurrency/concurrency_ReaderLockToken.h"

#include "dbtype_Lock.h"

namespace
{
    const static std::string EMPTY_STRING;
    const static mutgos::dbtype::Id DEFAULT_ID;
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Lock::Lock()
      : lock_type(LOCK_INVALID),
        lock_id(0),
        lock_path(0),
        lock_path_data(0),
        operation_not(false)
    {
    }

    // ----------------------------------------------------------------------
    Lock::Lock(const Lock &rhs)
      : lock_type(rhs.lock_type),
        lock_id(0),
        lock_path(0),
        lock_path_data(0),
        operation_not(rhs.operation_not)
    {
        if (rhs.lock_id)
        {
            lock_id = new Id(*rhs.lock_id);
        }

        if (rhs.lock_path)
        {
            lock_path = new PropertyDirectory::PathString(*rhs.lock_path);
        }

        if (rhs.lock_path_data)
        {
            lock_path_data = rhs.lock_path_data->clone();
        }
    }

    // ----------------------------------------------------------------------
    Lock::~Lock()
    {
        unlock();
    }

    // ----------------------------------------------------------------------
    Lock &Lock::operator=(const Lock &rhs)
    {
        if (&rhs != this)
        {
            unlock();

            lock_type = rhs.lock_type;

            if (rhs.lock_id)
            {
                lock_id = new Id(*rhs.lock_id);
            }

            if (rhs.lock_path)
            {
                lock_path = new PropertyDirectory::PathString(*rhs.lock_path);
            }

            if (rhs.lock_path_data)
            {
                lock_path_data = rhs.lock_path_data->clone();
            }

            operation_not = rhs.operation_not;
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    size_t Lock::mem_used(void) const
    {
        size_t size = sizeof(this);

        if (lock_id)
        {
            size += lock_id->mem_used();
        }

        if (lock_path)
        {
            size += sizeof(*lock_path) + lock_path->size();
        }

        if (lock_path_data)
        {
            size += lock_path_data->mem_used();
        }

        size += sizeof(operation_not);

        return size;
    }

    // ----------------------------------------------------------------------
    std::string Lock::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "Lock type: ";

        if (operation_not)
        {
            strstream << " (NOT) ";
        }

        switch (lock_type)
        {
            case LOCK_INVALID:
            {
                strstream << "unlocked" << std::endl;
                break;
            }

            case LOCK_BY_GROUP:
            case LOCK_BY_ID:
            {
                if (lock_type == LOCK_BY_ID)
                {
                    strstream << "by ID" << std::endl;
                }
                else
                {
                    strstream << "by group" << std::endl;
                }

                strstream << "Lock ID: "
                          << (lock_id ? lock_id->to_string() : "*INVALID*")
                          << std::endl;

                break;
            }

            case LOCK_BY_PROPERTY:
            {
                strstream << "by property"
                          << std::endl
                          << "Property: "
                          << (lock_path ? *lock_path : "*INVALID*")
                          << std::endl
                          << "Property value: "
                          << (lock_path_data ? lock_path_data->get_as_string() :
                              "*INVALID*")
                          << std::endl;
                break;
            }

            default:
            {
                strstream << "*UNKNOWN*" << std::endl;
                break;
            }
        }

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Lock::lock_valid(void) const
    {
        return lock_type != LOCK_INVALID;
    }

    // ----------------------------------------------------------------------
    Lock::LockType Lock::get_lock_type(void) const
    {
        return lock_type;
    }

    // ----------------------------------------------------------------------
    const Id &Lock::get_id(void) const
    {
        if (lock_id and
            ((lock_type == LOCK_BY_GROUP) or (lock_type == LOCK_BY_ID)))
        {
            return *lock_id;
        }
        else
        {
            return DEFAULT_ID;
        }
    }

    // ----------------------------------------------------------------------
    const PropertyDirectory::PathString &Lock::get_path(void) const
    {
        if (lock_path and (lock_type == LOCK_BY_PROPERTY))
        {
            return *lock_path;
        }
        else
        {
            return EMPTY_STRING;
        }
    }

    // ----------------------------------------------------------------------
    const PropertyData *Lock::get_path_data(void) const
    {
        if (lock_path_data and (lock_type == LOCK_BY_PROPERTY))
        {
            return lock_path_data;
        }
        else
        {
            return 0;
        }
    }

    // ----------------------------------------------------------------------
    void Lock::unlock(void)
    {
        lock_type = LOCK_INVALID;
        operation_not = false;

        delete lock_id;
        lock_id = 0;
        delete lock_path;
        lock_path = 0;
        delete lock_path_data;
        lock_path_data = 0;
    }

    // ----------------------------------------------------------------------
    bool Lock::lock_by_entity(
        Entity *entity,
        concurrency::ReaderLockToken &token,
        const bool not_result)
    {
        bool success = false;

        if (entity)
        {
            unlock();

            lock_type =
                dynamic_cast<Group *>(entity) ? LOCK_BY_GROUP : LOCK_BY_ID;

            lock_id = new Id(entity->get_entity_id());
            operation_not = not_result;
            success = true;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Lock::lock_by_property(
        const PropertyDirectory::PathString &path,
        const PropertyData &data,
        const bool not_result)
    {
        bool success = false;

        if (not path.empty())
        {
            unlock();

            lock_type = LOCK_BY_PROPERTY;
            lock_path = new PropertyDirectory::PathString(path);
            lock_path_data = data.clone();

            if (lock_path_data)
            {
                operation_not = not_result;
                success = true;
            }
            else
            {
                unlock();
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Lock::evaluate(Entity *entity_ptr, concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (not lock_valid())
        {
            success = true;
        }
        else if (entity_ptr and (lock_type == LOCK_BY_ID) and lock_id)
        {
            success = (entity_ptr->get_entity_id() == *lock_id);

            if (operation_not)
            {
                success = not success;
            }
        }
        else if (entity_ptr and (lock_type == LOCK_BY_PROPERTY) and lock_path
                 and lock_path_data)
        {
            // We can only do this if the entity can have properties
            PropertyEntity *prop_entity = dynamic_cast<PropertyEntity *>(
                entity_ptr);

            if (prop_entity)
            {
                PropertyData *data =
                    prop_entity->get_property(*lock_path, token);

                if (data)
                {
                    success = ((*data) == *lock_path_data);

                    delete data;
                    data = 0;
                }

                if (operation_not)
                {
                    success = not success;
                }
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Lock::evaluate(
        Entity *entity_ptr,
        Group *group_ptr,
        concurrency::ReaderLockToken &group_token)
    {
        bool success = false;

        if (not lock_valid())
        {
            success = true;
        }
        else if (entity_ptr and group_ptr and (lock_type == LOCK_BY_GROUP))
        {
            success = group_ptr->is_in_group(
                entity_ptr->get_entity_id(), group_token);

            if (operation_not)
            {
                success = not success;
            }
        }

        return success;
    }
} /* namespace dbtype */
} /* namespace mutgos */
