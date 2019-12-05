/*
 * dbtype_Exit.cpp
 */


#include <string>

#include "dbtype_Entity.h"
#include "dbtype_ActionEntity.h"
#include "dbtype_Exit.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_Utf8Tools.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Exit::Exit()
      : ActionEntity()
    {
    }

    // ----------------------------------------------------------------------
    Exit::Exit(const Id &id)
        : ActionEntity(id, ENTITYTYPE_exit, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Exit::~Exit()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Exit::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Exit(
                id,
                ENTITYTYPE_exit,
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
    std::string Exit::to_string()
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << ActionEntity::to_string()
                  << "Arrival Message: "
                  << exit_arrive_message << std::endl
                  << "Arrival Message (room): " << exit_arrive_room_message
                  << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Exit::set_exit_arrive_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (text::utf8_size(message) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            exit_arrive_message = message;
            notify_field_changed(ENTITYFIELD_exit_arrive_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_exit_arrive_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Exit::set_exit_arrive_message(const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_exit_arrive_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string Exit::get_exit_arrive_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = exit_arrive_message;
        }
        else
        {
            LOG(error, "dbtype", "get_exit_arrive_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string Exit::get_exit_arrive_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_exit_arrive_message(token);
    }

    // ----------------------------------------------------------------------
    bool Exit::set_exit_arrive_room_message(
        const std::string &message,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (text::utf8_size(message) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            exit_arrive_room_message = message;
            notify_field_changed(ENTITYFIELD_exit_arrive_room_msg);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_exit_arrive_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Exit::set_exit_arrive_room_message(const std::string &message)
    {
        concurrency::WriterLockToken token(*this);

        return set_exit_arrive_room_message(message, token);
    }

    // ----------------------------------------------------------------------
    std::string Exit::get_exit_arrive_room_message(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = exit_arrive_room_message;
        }
        else
        {
            LOG(error, "dbtype", "get_exit_arrive_room_message",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string Exit::get_exit_arrive_room_message(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_exit_arrive_room_message(token);
    }

    // ----------------------------------------------------------------------
    Exit::Exit(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
        : ActionEntity(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    size_t Exit::mem_used_fields(void)
    {
        const size_t field_sizes = ActionEntity::mem_used_fields()
             + sizeof(exit_arrive_message) + exit_arrive_message.size()
             + sizeof(exit_arrive_room_message) + exit_arrive_room_message.size();

        return field_sizes;
    }

    // ----------------------------------------------------------------------
    void Exit::copy_fields(Entity *entity_ptr)
    {
        ActionEntity::copy_fields(entity_ptr);

        Exit *cast_ptr = 0;

        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Exit *>(entity_ptr))) != 0))
        {
            cast_ptr->exit_arrive_message = exit_arrive_message;
            notify_field_changed(ENTITYFIELD_exit_arrive_msg);

            cast_ptr->exit_arrive_room_message = exit_arrive_room_message;
            notify_field_changed(ENTITYFIELD_exit_arrive_room_msg);
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
