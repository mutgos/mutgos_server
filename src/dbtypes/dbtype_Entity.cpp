/*
 * dbtype_Entity.cpp
 */

#include <string>
#include <ostream>
#include <limits>
#include <set>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_Utf8Tools.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DatabaseEntityChangeListener.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"

#include <boost/thread/shared_mutex.hpp>
#include <boost/algorithm/string.hpp>

namespace
{
    const MG_UnsignedInt MAX_STRING_LENGTH = 512;
}

namespace mutgos
{
namespace dbtype
{
    // Statics
    //
    Entity::DbListeners Entity::db_listeners;

    // -----------------------------------------------------------------------
    Entity::Entity(const Id &id)
      : entity_type(ENTITYTYPE_entity),
        entity_id(id),
        entity_instance(0),
        entity_version(0),
        entity_created_timestamp(true),
        entity_updated_timestamp(entity_created_timestamp),
        entity_accessed_timestamp(entity_created_timestamp),
        entity_access_count(1),
        entity_delete_batch_id(0),
        entity_deleted_flag(false),
        need_call_listener(true),
        dirty_flag(false),
        ignore_changes(false),
        locked_thread_id_valid(false),
        inner_lock_count(0)
    {
        notify_field_changed(ENTITYFIELD_type);
        notify_field_changed(ENTITYFIELD_id);
        notify_field_changed(ENTITYFIELD_version);
        notify_field_changed(ENTITYFIELD_instance);
        notify_field_changed(ENTITYFIELD_created_timestamp);
        notify_field_changed(ENTITYFIELD_updated_timestamp);
        notify_field_changed(ENTITYFIELD_accessed_timestamp);
        notify_field_changed(ENTITYFIELD_access_count);

        entity_references_field.resize(ENTITYFIELD_END, 0);

        notify_db_listener();
    }

    // -----------------------------------------------------------------------
    Entity::Entity()
      : entity_type(ENTITYTYPE_entity),
        entity_instance(0),
        entity_version(0),
        entity_created_timestamp(false),
        entity_updated_timestamp(false),
        entity_accessed_timestamp(false),
        entity_access_count(0),
        entity_delete_batch_id(0),
        entity_deleted_flag(false),
        need_call_listener(false),
        dirty_flag(false),
        ignore_changes(true),
        locked_thread_id_valid(false),
        inner_lock_count(0)
    {
        entity_references_field.resize(ENTITYFIELD_END, 0);
    }

    // -----------------------------------------------------------------------
    Entity::~Entity()
    {
        clear_entity_references_field();
    }

    // -----------------------------------------------------------------------
    // Subclasses should completely overide this to size their own class
    // instance.
    size_t Entity::mem_used(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return sizeof(*this) + mem_used_fields();
        }
        else
        {
            LOG(error, "dbtype", "mem_used",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // -----------------------------------------------------------------------
    size_t Entity::mem_used(void)
    {
        concurrency::ReaderLockToken token(*this);

        return mem_used(token);
    }

    // -----------------------------------------------------------------------
    Entity::Entity(
        const Id& id,
        const EntityType& type,
        const Entity::VersionType version,
        const Entity::InstanceType instance,
        const bool restoring)
      : entity_type(type),
        entity_id(id),
        entity_instance(instance),
        entity_version(version),
        entity_created_timestamp(not restoring),
        entity_updated_timestamp(false),
        entity_accessed_timestamp(false),
        entity_access_count(1),
        entity_delete_batch_id(0),
        entity_deleted_flag(false),
        need_call_listener(false),
        dirty_flag(false),
        ignore_changes(restoring),
        locked_thread_id_valid(false),
        inner_lock_count(0)
    {
        if (not restoring)
        {
            entity_updated_timestamp = entity_created_timestamp;
            entity_accessed_timestamp = entity_created_timestamp;
        }

        notify_field_changed(ENTITYFIELD_type);
        notify_field_changed(ENTITYFIELD_id);
        notify_field_changed(ENTITYFIELD_version);
        notify_field_changed(ENTITYFIELD_instance);
        notify_field_changed(ENTITYFIELD_created_timestamp);
        notify_field_changed(ENTITYFIELD_updated_timestamp);
        notify_field_changed(ENTITYFIELD_accessed_timestamp);
        notify_field_changed(ENTITYFIELD_access_count);

        entity_references_field.resize(ENTITYFIELD_END, 0);

        notify_db_listener();
    }

    // -----------------------------------------------------------------------
    Entity *Entity::clone(
        const Id &id,
        const Entity::VersionType version,
        const Entity::InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Entity(
                id,
                ENTITYTYPE_entity,
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

    // -----------------------------------------------------------------------
    // Subclasses do not need to override this since it is already generic.
    Entity *Entity::clone(
        const Id &id,
        const Entity::VersionType version,
        const Entity::InstanceType instance)
    {
        concurrency::ReaderLockToken token(*this);

        return clone(id, version, instance, token);
    }

    // -----------------------------------------------------------------------
    void Entity::register_change_listener(
        DatabaseEntityChangeListener *listener_ptr)
    {
        if (listener_ptr)
        {
            db_listeners.push_back(listener_ptr);
        }
    }

    // -----------------------------------------------------------------------
    void Entity::unregister_change_listener(
        DatabaseEntityChangeListener *listener_ptr)
    {
        if (listener_ptr)
        {
            DbListeners::iterator iter = db_listeners.begin();

            while (iter != db_listeners.end())
            {
                if ((*iter) == listener_ptr)
                {
                    iter = db_listeners.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    void Entity::restore_complete(void)
    {
        ignore_changes = false;
    }

    // -----------------------------------------------------------------------
    bool Entity::clear_dirty(concurrency::WriterLockToken &token)
    {
        bool success = true;

        if (token.has_lock(*this))
        {
            dirty_flag = false;
            dirty_fields.clear();
        }
        else
        {
            LOG(error, "dbtype", "clear_dirty",
                "Using the wrong lock token!");

            success = false;
        }

        return success;
    }

    // -----------------------------------------------------------------------
    bool Entity::is_dirty(void)
    {
        concurrency::ReaderLockToken token(*this);

        return is_dirty(token);
    }

    // -----------------------------------------------------------------------
    bool Entity::is_dirty(concurrency::ReaderLockToken &token)
    {
        bool dirty = false;

        if (token.has_lock(*this))
        {
            dirty = dirty_flag;
        }
        else
        {
            LOG(error, "dbtype", "is_dirty",
                "Using the wrong lock token!");

            dirty = false;
        }

        return dirty;
    }

    // -----------------------------------------------------------------------
    std::string Entity::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream
          << std::string("ID:        ") << entity_id.to_string() << std::endl
          << "Type:      " << entity_type_to_string(entity_type) << std::endl
          << "Flags:     ";

          for (FlagSet::const_iterator flag_iter = entity_flags.begin();
               flag_iter != entity_flags.end();
               ++flag_iter)
          {
              strstream << *flag_iter << " ";
          }

        strstream
          << std::endl
          << "Owner:     " << entity_owner.to_string() << std::endl
          << "Name:      " << entity_name << std::endl
          << "Version:   " << entity_version << std::endl
          << "Instance:  " << entity_instance << std::endl
          << "Note:      " << entity_note << std::endl
          << "Reg name:  " << entity_reg_name << std::endl
          << "Reg cat:   " << entity_reg_category << std::endl
          << "Security:  " << entity_security.to_string() << std::endl
          << "Created:   " << entity_created_timestamp.to_string() << std::endl
          << "Updated:   " << entity_updated_timestamp.to_string() << std::endl
          << "Accessed:  " << entity_accessed_timestamp.to_string() << std::endl
          << "#Accessed: " << entity_access_count << std::endl
          << "DeleteID:  " << entity_delete_batch_id << std::endl
          << "Deleted:   " << entity_deleted_flag << std::endl;

        return strstream.str();
    }

    // -----------------------------------------------------------------------
    void Entity::set_entity_created_timestamp(const TimeStamp &timestamp)
    {
        // No need to notify of a field change since this is only used when
        // restoring.
        //
        if (ignore_changes)
        {
            entity_created_timestamp = timestamp;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_created_timestamp",
                "Called when not deserializing!");
        }
    }

    // -----------------------------------------------------------------------
    void Entity::set_entity_updated_timestamp(const TimeStamp &timestamp)
    {
        // No need to notify of a field change since this is only used when
        // restoring.
        //
        if (ignore_changes)
        {
            entity_updated_timestamp = timestamp;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_updated_timestamp",
                "Called when not deserializing!");
        }
    }

    // -----------------------------------------------------------------------
    void Entity::set_entity_accessed_timestamp(const TimeStamp &timestamp)
    {
        // No need to notify of a field change since this is only used when
        // restoring.
        //
        if (ignore_changes)
        {
            entity_accessed_timestamp = timestamp;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_accessed_timestamp",
                "Called when not deserializing!");
        }
    }

    // -----------------------------------------------------------------------
    void Entity::set_entity_access_count(const AccessCountType count)
    {
        // No need to notify of a field change since this is only used when
        // restoring.
        //
        if (ignore_changes)
        {
            entity_access_count = count;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_access_count",
                "Called when not deserializing!");
        }
    }

    // -----------------------------------------------------------------------
    void Entity::set_entity_flags(const Entity::FlagSet &flags)
    {
        if (ignore_changes)
        {
            entity_flags = flags;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_flags",
                "Called when not deserializing!");
        }
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_name(concurrency::ReaderLockToken& token)
    {
        if (token.has_lock(*this))
        {
            return entity_name;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_name",
                "Using the wrong lock token!");

            return "";
        }
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_name(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_name;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_name(
        const std::string& name,
        concurrency::WriterLockToken& token)
    {
        if (name.empty())
        {
            // Cannot have an empty Entity name, or it would be hard to
            // find.
            return false;
        }

        if (text::utf8_size(name) > config::db::limits_entity_name())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            entity_name = name;
            notify_field_changed(ENTITYFIELD_name);
            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_name",
                "Using the wrong lock token!");
            return false;
        }
    }


    // -----------------------------------------------------------------------
    bool Entity::set_entity_name(const std::string& name)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_name(name, token);
    }

    // -----------------------------------------------------------------------
    Security Entity::get_entity_security(
            concurrency::ReaderLockToken& token)
    {
        if (token.has_lock(*this))
        {
            return entity_security;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_security",
                "Using the wrong lock token!");

            return Security();
        }
    }

    // -----------------------------------------------------------------------
    Security Entity::get_entity_security(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_security;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_security(
        const Security& security,
        concurrency::WriterLockToken& token)
    {
        if (token.has_lock(*this))
        {
            const Security::RemoveAddPair changes =
                entity_security.diff_ids(security);

            entity_security = security;
            notify_field_changed(ENTITYFIELD_security);

            // Add the diff info to the diff ID lists
            //
            for (Security::SecurityIds::const_iterator remove_iter =
                    changes.first.begin();
                 remove_iter != changes.first.end();
                 ++remove_iter)
            {
                removed_id(ENTITYFIELD_security, *remove_iter);
            }

            for (Security::SecurityIds::const_iterator add_iter =
                    changes.second.begin();
                 add_iter != changes.second.end();
                 ++add_iter)
            {
                added_id(ENTITYFIELD_security, *add_iter);
            }

            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_security",
                "Using the wrong lock token!");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_security(const Security& security)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_security(security, token);
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_note(concurrency::ReaderLockToken& token)
    {
        if (token.has_lock(*this))
        {
            return entity_note;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_note",
                "Using the wrong lock token!");

            return "";
        }
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_note(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_note;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_note(
        const std::string& note,
        concurrency::WriterLockToken& token)
    {
        if (text::utf8_size(note) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            // Limit the size of the note.
            entity_note = note.substr(0, MAX_STRING_LENGTH);
            notify_field_changed(ENTITYFIELD_note);
            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_note",
                "Using the wrong lock token!");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_note(const std::string& note)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_note(note, token);
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_registration_name(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = entity_reg_name;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_registration_name",
                "Using the wrong lock token!");
        }

        return result;
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_registration_name(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_entity_registration_name(token);
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_registration_name(
        const std::string &name,
        concurrency::WriterLockToken &token)
    {
        if (text::utf8_size(name) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            // Limit size of the name
            entity_reg_name = name.substr(0, MAX_STRING_LENGTH);
            notify_field_changed(ENTITYFIELD_reg_name);
            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_registration_name",
                "Using the wrong lock token!");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_registration_name(const std::string &name)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_registration_name(name, token);
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_registration_category(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = entity_reg_category;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_registration_category",
                "Using the wrong lock token!");
        }

        return result;
    }

    // -----------------------------------------------------------------------
    std::string Entity::get_entity_registration_category(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_entity_registration_category(token);
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_registration_category(
        const std::string &category,
        concurrency::WriterLockToken &token)
    {
        if (text::utf8_size(category) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            // Limit size of the category
            entity_reg_category = category.substr(0, MAX_STRING_LENGTH);
            notify_field_changed(ENTITYFIELD_reg_category);
            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_registration_category",
                "Using the wrong lock token!");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_registration_category(const std::string &category)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_registration_category(category, token);
    }

    // -----------------------------------------------------------------------
    TimeStamp Entity::get_entity_updated_timestamp(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_updated_timestamp;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_updated_timestamp",
                "Using the wrong lock token!");

            return TimeStamp(true);
        }
    }

    // -----------------------------------------------------------------------
    TimeStamp Entity::get_entity_updated_timestamp(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_updated_timestamp;
    }

    // -----------------------------------------------------------------------
    TimeStamp Entity::get_entity_accessed_timestamp(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_accessed_timestamp;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_accessed_timestamp",
                "Using the wrong lock token!");

            return TimeStamp(true);
        }
    }

    // -----------------------------------------------------------------------
    TimeStamp Entity::get_entity_accessed_timestamp(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_accessed_timestamp;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_accessed_timestamp(
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            entity_accessed_timestamp.set_to_now();
            notify_field_changed(ENTITYFIELD_accessed_timestamp);

            if (entity_access_count <
                std::numeric_limits<AccessCountType>::max())
            {
                ++entity_access_count;
            }

            notify_field_changed(ENTITYFIELD_access_count);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_accessed_timestamp",
                "Using the wrong lock token!");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_accessed_timestamp(void)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_accessed_timestamp(token);
    }

    // -----------------------------------------------------------------------
    Entity::AccessCountType Entity::get_entity_access_count(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_access_count;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_access_count",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // -----------------------------------------------------------------------
    Entity::AccessCountType Entity::get_entity_access_count(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_access_count;
    }

    // -----------------------------------------------------------------------
    Id Entity::get_entity_owner(concurrency::ReaderLockToken& token)
    {
        if (token.has_lock(*this))
        {
            return entity_owner;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_owner",
                "Using the wrong lock token!");

            return Id();
        }
    }

    // -----------------------------------------------------------------------
    Id Entity::get_entity_owner(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_owner;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_owner(
        const Id& owner,
        concurrency::WriterLockToken& token)
    {
        if (token.has_lock(*this))
        {
            if (entity_owner != owner)
            {
                set_single_id_field(ENTITYFIELD_owner, entity_owner, owner);
                entity_owner = owner;
                notify_field_changed(ENTITYFIELD_owner);
            }

            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_entity_owner",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_entity_owner(const Id& owner)
    {
        concurrency::WriterLockToken token(*this);

        return set_entity_owner(owner, token);
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::add_entity_flag(
        const FlagType& flag,
        concurrency::WriterLockToken& token)
    {
        std::string add_flag = flag;

        if (token.has_lock(*this))
        {
            if (entity_flags.find(add_flag) != entity_flags.end())
            {
                entity_flags.insert(add_flag);
                notify_field_changed(ENTITYFIELD_flags);
                added_flag(add_flag);
            }

            return FLAGRC_success;
        }
        else
        {
            LOG(error, "dbtype", "add_entity_flag",
                "Using the wrong lock token!");

            return FLAGRC_lock_error;
        }
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::add_entity_flag(const FlagType& flag)
    {
        concurrency::WriterLockToken token(*this);

        return add_entity_flag(flag, token);
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::remove_entity_flag(
        const FlagType& flag,
        concurrency::WriterLockToken& token)
    {
        std::string remove_flag = flag;

        if (token.has_lock(*this))
        {
            FlagSet::iterator iter = entity_flags.find(remove_flag);

            if (iter != entity_flags.end())
            {
                entity_flags.erase(iter);
                notify_field_changed(ENTITYFIELD_flags);
                removed_flag(remove_flag);
            }

            return FLAGRC_success;
        }
        else
        {
            LOG(error, "dbtype", "remove_entity_flag",
                "Using the wrong lock token!");

            return FLAGRC_lock_error;
        }
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::remove_entity_flag(
        const FlagType& flag)
    {
        concurrency::WriterLockToken token(*this);

        return remove_entity_flag(flag, token);
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::check_entity_flag(
        const FlagType& flag,
        concurrency::ReaderLockToken& token)
    {
        std::string check_flag = flag;

        if (token.has_lock(*this))
        {
            FlagSet::iterator iter = entity_flags.find(check_flag);

            if (iter != entity_flags.end())
            {
                entity_flags.erase(iter);
                notify_field_changed(ENTITYFIELD_flags);
                removed_flag(check_flag);
            }

            return ((entity_flags.find(check_flag) == entity_flags.end()) ?
                    FLAGRC_not_set :
                    FLAGRC_set);
        }
        else
        {
            LOG(error, "dbtype", "check_entity_flag",
                "Using the wrong lock token!");

            return FLAGRC_lock_error;
        }
    }

    // -----------------------------------------------------------------------
    Entity::EntityFlagReturnCode Entity::check_entity_flag(
        const FlagType& flag)
    {
        concurrency::ReaderLockToken token(*this);

        return check_entity_flag(flag, token);
    }

    // -----------------------------------------------------------------------
    Entity::FlagSet Entity::get_entity_flags(concurrency::ReaderLockToken& token)
    {
        if (token.has_lock(*this))
        {
            return entity_flags;
        }
        else
        {
            LOG(error, "dbtype", "get_entity_flags",
                "Using the wrong lock token!");

            return FlagSet();
        }
    }

    // -----------------------------------------------------------------------
    Entity::FlagSet Entity::get_entity_flags(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_flags;
    }

    // -----------------------------------------------------------------------
    bool Entity::add_entity_reference(
        const Id &id,
        const EntityField field,
        concurrency::WriterLockToken &token)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "add_entity_reference",
                "field out of range!");
            return false;
        }

        if (token.has_lock(*this))
        {
            entity_references[id].insert(field);

            IdSet *ids_ptr = entity_references_field[field];

            if (not ids_ptr)
            {
                ids_ptr = new IdSet();
                entity_references_field[field] = ids_ptr;
            }

            ids_ptr->insert(id);

            notify_field_changed(ENTITYFIELD_references);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "add_entity_reference",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::add_entity_reference(
        const Id &id,
        const EntityField field)
    {
        concurrency::WriterLockToken token(*this);

        return add_entity_reference(id, field, token);
    }

    // -----------------------------------------------------------------------
    bool Entity::remove_entity_reference(
        const Id &id,
        const EntityField field,
        concurrency::WriterLockToken &token)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "remove_entity_reference",
                "field out of range!");
            return false;
        }

        if (token.has_lock(*this))
        {
            IdFieldsMap::iterator entity_iter =
                entity_references.find(id);

            if (entity_iter != entity_references.end())
            {
                entity_iter->second.erase(field);

                if (entity_iter->second.empty())
                {
                    // No more references from ID, so remove it
                    entity_references.erase(entity_iter);
                }
            }

            IdSet *ids_ptr = entity_references_field[field];

            if (ids_ptr)
            {
                ids_ptr->erase(id);
                if (ids_ptr->empty())
                {
                    delete ids_ptr;
                    ids_ptr = 0;
                    entity_references_field[field] = 0;
                }
            }

            notify_field_changed(ENTITYFIELD_references);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "remove_entity_reference",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::remove_entity_reference(const Id &id, const EntityField field)
    {
        concurrency::WriterLockToken token(*this);

        return remove_entity_reference(id, field, token);
    }

    // -----------------------------------------------------------------------
    bool Entity::remove_entity_reference(
        const Id &id,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            IdFieldsMap::iterator iter = entity_references.find(id);

            if (iter != entity_references.end())
            {
                IdSet *ids_ptr = 0;

                for (EntityFieldSet::const_iterator field_iter =
                        iter->second.begin();
                    field_iter != iter->second.end();
                    ++field_iter)
                {
                    ids_ptr = entity_references_field[*field_iter];

                    if (ids_ptr)
                    {
                        ids_ptr->erase(id);

                        if (ids_ptr->empty())
                        {
                            delete ids_ptr;
                            ids_ptr = 0;
                            entity_references_field[*field_iter] = 0;
                        }
                    }
                }

                entity_references.erase(iter);
                notify_field_changed(ENTITYFIELD_references);
            }

            return true;
        }
        else
        {
            LOG(error, "dbtype", "remove_entity_reference (id only)",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::remove_entity_reference(const Id &id)
    {
        concurrency::WriterLockToken token(*this);

        return remove_entity_reference(id, token);
    }

    // -----------------------------------------------------------------------
    Entity::EntityFieldSet Entity::get_field_references(
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            IdFieldsMap::iterator entity_iter =
                entity_references.find(id);

            if (entity_iter != entity_references.end())
            {
                return entity_iter->second;
            }
            else
            {
                // No references, so return empty.
                return EntityFieldSet();
            }
        }
        else
        {
            LOG(error, "dbtype", "get_field_references",
                "Using the wrong lock token!");

            return EntityFieldSet();
        }
    }

    // -----------------------------------------------------------------------
    Entity::EntityFieldSet Entity::get_field_references(const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_field_references(id, token);
    }

    // -----------------------------------------------------------------------
    Entity::IdSet Entity::get_reference_ids(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            IdSet ids;

            for (IdFieldsMap::const_iterator id_iter =
                    entity_references.begin();
                 id_iter != entity_references.end();
                 ++id_iter)
            {
                ids.insert(id_iter->first);
            }

            return ids;
        }
        else
        {
            LOG(error, "dbtype", "get_reference_ids",
                "Using the wrong lock token!");

            return IdSet();
        }
    }

    // -----------------------------------------------------------------------
    Entity::IdSet Entity::get_reference_ids(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_reference_ids(token);
    }

    // -----------------------------------------------------------------------
    Entity::IdFieldsMap Entity::get_all_references(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_references;
        }
        else
        {
            LOG(error, "dbtype", "get_all_references",
                "Using the wrong lock token!");

            return IdFieldsMap();
        }
    }

    // -----------------------------------------------------------------------
    Entity::IdFieldsMap Entity::get_all_references(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_all_references(token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_first_reference(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (not entity_references.empty())
            {
                return entity_references.begin()->first;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_reference",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_first_reference(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_reference(token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_next_reference(
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            IdFieldsMap::const_iterator entity_iter =
                entity_references.find(id);

            if (entity_iter != entity_references.end())
            {
                ++entity_iter;

                if (entity_iter != entity_references.end())
                {
                    return entity_iter->first;
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_reference",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_next_reference(const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_reference(id, token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_last_reference(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (not entity_references.empty())
            {
                return (--(entity_references.end()))->first;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_reference",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_last_reference(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_reference(token);
    }

    // -----------------------------------------------------------------------
    Entity::IdVector Entity::get_reference_ids(
        const EntityField field,
        concurrency::ReaderLockToken &token)
    {
        IdVector result;

        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "get_reference_ids(field)",
                "field out of range!");
        }
        else if (token.has_lock(*this))
        {
            IdSet *id_set_ptr = entity_references_field[field];

            if (id_set_ptr and (not id_set_ptr->empty()))
            {
                result.reserve(id_set_ptr->size());
                result.insert(
                    result.end(),
                    id_set_ptr->begin(),
                    id_set_ptr->end());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_reference_ids(field)",
                "Using the wrong lock token!");
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Entity::IdVector Entity::get_reference_ids(
        const EntityField field)
    {
        concurrency::ReaderLockToken token(*this);

        return get_reference_ids(field, token);
    }

    // -----------------------------------------------------------------------
    bool Entity::get_reference_ids_append(
        const EntityField field,
        Entity::IdVector &ids,
        concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "get_reference_ids_append(field)",
                "field out of range!");
        }
        else if (token.has_lock(*this))
        {
            IdSet *id_set_ptr = entity_references_field[field];

            if (id_set_ptr and (not id_set_ptr->empty()))
            {
                result = true;
                ids.reserve(ids.size() + id_set_ptr->size());
                ids.insert(
                    ids.end(),
                    id_set_ptr->begin(),
                    id_set_ptr->end());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_reference_ids_append(field)",
                "Using the wrong lock token!");
        }

        return result;
    }

    // -----------------------------------------------------------------------
    bool Entity::get_reference_ids_append(
        const EntityField field,
        Entity::IdVector &ids)
    {
        concurrency::ReaderLockToken token(*this);

        return get_reference_ids_append(field, ids, token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_first_reference(
        const EntityField field,
        concurrency::ReaderLockToken &token)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "get_first_reference(field)",
                "field out of range!");
        }
        else if (token.has_lock(*this))
        {
            IdSet *id_set_ptr = entity_references_field[field];

            if (id_set_ptr and (not id_set_ptr->empty()))
            {
                return *(id_set_ptr->begin());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_reference(field)",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_first_reference(const EntityField field)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_reference(field, token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_next_reference(
        const EntityField field,
        const Id &id,
        concurrency::ReaderLockToken &token)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "get_next_reference(field)",
                "field out of range!");
        }
        else if (token.has_lock(*this))
        {
            IdSet *id_set_ptr = entity_references_field[field];

            if (id_set_ptr and (not id_set_ptr->empty()))
            {
                IdSet::const_iterator id_set_iter =
                    id_set_ptr->find(id);

                if (id_set_iter != id_set_ptr->end())
                {
                    ++id_set_iter;

                    if (id_set_iter != id_set_ptr->end())
                    {
                        return *id_set_iter;
                    }
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_reference(field)",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_next_reference(const EntityField field, const Id &id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_reference(field, id, token);
    }

    // -----------------------------------------------------------------------
    Id Entity::get_last_reference(
        const EntityField field,
        concurrency::ReaderLockToken &token)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_BEGIN))
        {
            LOG(fatal, "dbtype", "get_last_reference(field)",
                "field out of range!");
        }
        else if (token.has_lock(*this))
        {
            IdSet *id_set_ptr = entity_references_field[field];

            if (id_set_ptr and (not id_set_ptr->empty()))
            {
                return *(--(id_set_ptr->end()));
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_reference(field)",
                "Using the wrong lock token!");
        }

        return Id();
    }

    // -----------------------------------------------------------------------
    Id Entity::get_last_reference(const EntityField field)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_reference(field, token);
    }

    // -----------------------------------------------------------------------
    bool Entity::clear_all_references(concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            entity_references.clear();
            clear_entity_references_field();
            notify_field_changed(ENTITYFIELD_references);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "clear_all_references",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::clear_all_references(void)
    {
        concurrency::WriterLockToken token(*this);

        return clear_all_references(token);
    }

    // -----------------------------------------------------------------------
    Entity::DeleteBatchId Entity::get_delete_batch_id(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_delete_batch_id;
        }
        else
        {
            LOG(error, "dbtype", "get_delete_batch_id",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // -----------------------------------------------------------------------
    Entity::DeleteBatchId Entity::get_delete_batch_id(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_delete_batch_id;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_delete_batch_id(
        const Entity::DeleteBatchId delete_id,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            entity_delete_batch_id = delete_id;
            notify_field_changed(ENTITYFIELD_delete_batch_id);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_delete_batch_id",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_delete_batch_id(
        const Entity::DeleteBatchId delete_id)
    {
        concurrency::WriterLockToken token(*this);

        return set_delete_batch_id(delete_id, token);
    }

    // -----------------------------------------------------------------------
    bool Entity::get_deleted_flag(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return entity_deleted_flag;
        }
        else
        {
            LOG(error, "dbtype", "get_deleted_flag",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::get_deleted_flag(void)
    {
        concurrency::ReaderLockToken token(*this);

        return entity_deleted_flag;
    }

    // -----------------------------------------------------------------------
    bool Entity::set_deleted_flag(
        const bool deleted,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            entity_deleted_flag = deleted;
            notify_field_changed(ENTITYFIELD_deleted_flag);

            return true;
        }
        else
        {
            LOG(error, "dbtype", "set_deleted_flag",
                "Using the wrong lock token!");

            return false;
        }
    }

    // -----------------------------------------------------------------------
    bool Entity::set_deleted_flag(const bool deleted)
    {
        concurrency::WriterLockToken token(*this);

        return set_deleted_flag(deleted, token);
    }

    // -----------------------------------------------------------------------
    void Entity::copy_fields(Entity *entity_ptr)
    {
        if (entity_ptr)
        {
            // ID, type, instance, and version have already been copied.
            //
            entity_ptr->entity_name = entity_name;
            entity_ptr->notify_field_changed(ENTITYFIELD_name);

            entity_ptr->entity_note = entity_note;
            entity_ptr->notify_field_changed(ENTITYFIELD_note);

            entity_ptr->entity_reg_name = entity_reg_name;
            entity_ptr->notify_field_changed(ENTITYFIELD_reg_name);

            entity_ptr->entity_reg_category = entity_reg_category;
            entity_ptr->notify_field_changed(ENTITYFIELD_reg_category);

            entity_ptr->entity_security = entity_security;
            entity_ptr->notify_field_changed(ENTITYFIELD_security);

            const Security::SecurityIds &security_admin_ids =
                entity_security.get_admin_ids();

            for (Security::SecurityIds::const_iterator sec_iter =
                security_admin_ids.begin();
                 sec_iter != security_admin_ids.end();
                 ++sec_iter)
            {
                entity_ptr->added_id(ENTITYFIELD_security, *sec_iter);
            }

            const Security::SecurityIds &security_list_ids =
                entity_security.get_list_ids();

            for (Security::SecurityIds::const_iterator ids_iter =
                security_list_ids.begin();
                 ids_iter != security_list_ids.end();
                 ++ids_iter)
            {
                entity_ptr->added_id(ENTITYFIELD_security, *ids_iter);
            }

            // The created field is special since a copy is newly created.
            //
            entity_ptr->entity_created_timestamp.set_to_now();
            entity_ptr->notify_field_changed(ENTITYFIELD_created_timestamp);

            // The updated field is special since a copy is newly created.
            //
            entity_ptr->entity_updated_timestamp =
                entity_ptr->entity_created_timestamp;
            entity_ptr->notify_field_changed(ENTITYFIELD_updated_timestamp);

            // The accessed field is special since a copy is newly created.
            //
            entity_ptr->entity_accessed_timestamp =
                entity_ptr->entity_created_timestamp;
            entity_ptr->notify_field_changed(ENTITYFIELD_accessed_timestamp);
            entity_ptr->entity_access_count = 0;
            entity_ptr->notify_field_changed(ENTITYFIELD_access_count);

            entity_ptr->added_id(ENTITYFIELD_owner, entity_ptr->entity_owner);
            entity_ptr->entity_owner = entity_owner;
            entity_ptr->notify_field_changed(ENTITYFIELD_owner);
            entity_ptr->added_id(ENTITYFIELD_owner, entity_owner);

            entity_ptr->entity_flags = entity_flags;
            entity_ptr->notify_field_changed(ENTITYFIELD_flags);

            for (FlagSet::const_iterator flag_iter = entity_flags.begin();
                 flag_iter != entity_flags.end();
                 ++flag_iter)
            {
                entity_ptr->added_flag(*flag_iter);
            }

            // If this is a new version or instance of an existing Entity,
            // then copy the references and other ID specific data.  The
            // database backend will take care of any cleanup of old versions,
            // etc.  New IDs won't have any references to them, so don't copy
            // in that case.
            //
            if (entity_id == entity_ptr->entity_id)
            {
                entity_ptr->entity_references = entity_references;
                entity_ptr->populate_entity_references_field();
                entity_ptr->notify_field_changed(ENTITYFIELD_references);

                if (entity_delete_batch_id)
                {
                    entity_ptr->entity_delete_batch_id = entity_delete_batch_id;
                    notify_field_changed(ENTITYFIELD_delete_batch_id);
                }

                if (entity_deleted_flag)
                {
                    entity_ptr->entity_deleted_flag = true;
                    notify_field_changed(ENTITYFIELD_deleted_flag);
                }
            }

            entity_ptr->notify_db_listener();
        }
    }

    // -----------------------------------------------------------------------
    size_t Entity::mem_used_fields(void)
    {
        // Add up all the non-simple types and their contents that are
        // serialized and active in memory.
        //
        size_t memory = entity_id.mem_used()
            + entity_security.mem_used()
            + sizeof(entity_name) + entity_name.size()
            + sizeof(entity_note) + entity_note.size()
            + sizeof(entity_reg_name) + entity_reg_name.size()
            + sizeof(entity_reg_category) + entity_reg_category.size()
            + entity_created_timestamp.mem_used()
            + entity_updated_timestamp.mem_used()
            + entity_accessed_timestamp.mem_used()
            + entity_owner.mem_used();

        // Flags
        //
        memory += sizeof(entity_flags);

        for (FlagSet::const_iterator flag_iter = entity_flags.begin();
             flag_iter != entity_flags.end();
             ++flag_iter)
        {
            memory += sizeof(*flag_iter) + flag_iter->size();
        }

        // References
        //
        memory += sizeof(entity_references);

        for (IdFieldsMap::const_iterator ref_iter =
                entity_references.begin();
            ref_iter != entity_references.end();
             ++ref_iter)
        {
            memory += (2 * ref_iter->first.mem_used())
                   + sizeof(ref_iter->second)
                   + ref_iter->second.size()
                   + (ref_iter->second.size() * sizeof(EntityField));
        }

        return memory;
    }

    // -----------------------------------------------------------------------
    void Entity::added_id(const EntityField field, const Id &id_added)
    {
        ChangedIdFieldsMap::iterator
            field_entry = diff_ids_changed.find(field);

        dirty_flag = true;

        if (field_entry == diff_ids_changed.end())
        {
            // New entry
            IdsRemovedAdded &ids = diff_ids_changed[field];

            ids.second.insert(id_added);
        }
        else
        {
            // Existing entry
            if (not field_entry->second.first.erase(id_added))
            {
                // Nothing erased from the remove list, so it's truely a new
                // ID.
                field_entry->second.second.insert(id_added);
            }
        }
    }

    // -----------------------------------------------------------------------
    void Entity::removed_id(const EntityField field, const Id &id_removed)
    {
        ChangedIdFieldsMap::iterator
            field_entry = diff_ids_changed.find(field);

        dirty_flag = true;

        if (field_entry == diff_ids_changed.end())
        {
            // New entry
            IdsRemovedAdded &ids = diff_ids_changed[field];

            ids.first.insert(id_removed);
        }
        else
        {
            // Existing entry

            if (field_entry->second.second.find(id_removed) !=
                field_entry->second.second.end())
            {
                // They added it, but then changed their mind and
                // removed it.  End result is nothing happened.
                field_entry->second.second.erase(id_removed);
            }
            else
            {
                field_entry->second.first.insert(id_removed);
            }
        }
    }

    // -----------------------------------------------------------------------
    void Entity::set_single_id_field(
        const EntityField field,
        const Id &old_id,
        const Id &new_id)
    {
        ChangedIdFieldsMap::iterator
            field_entry = diff_ids_changed.find(field);

        dirty_flag = true;

        if (field_entry == diff_ids_changed.end())
        {
            // New entry
            IdsRemovedAdded &ids = diff_ids_changed[field];

            ids.first.insert(old_id);
            ids.second.insert(new_id);
        }
        else
        {
            // Existing entry.  Leave the removed entry alone
            // because intermediate changes are not important, only the
            // final change.
            field_entry->second.second.clear();
            field_entry->second.second.insert(new_id);
        }
    }

    // -----------------------------------------------------------------------
    void Entity::clear_id_journal(const EntityField field)
    {
        diff_ids_changed.erase(field);
    }

    // -----------------------------------------------------------------------
    void Entity::added_flag(const FlagType &flag_added)
    {
        dirty_flag = true;
        diff_flags_changed.first.erase(flag_added);
        diff_flags_changed.second.insert(flag_added);
    }

    // -----------------------------------------------------------------------
    void Entity::removed_flag(const FlagType &flag_removed)
    {
        dirty_flag = true;
        diff_flags_changed.first.insert(flag_removed);
        diff_flags_changed.second.erase(flag_removed);
    }

    // -----------------------------------------------------------------------
    bool Entity::lock(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        ++inner_lock_count;
                    }
                }
            }

            if (not already_have_lock)
            {
                entity_lock.lock();

                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);
                locked_thread_id_valid = true;
                locked_thread_id = my_thread_id;
            }

            return true;
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Entity::try_lock(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        ++inner_lock_count;
                    }
                }
            }

            if (already_have_lock)
            {
                return true;
            }
            else if (entity_lock.try_lock())
            {
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);
                locked_thread_id_valid = true;
                locked_thread_id = my_thread_id;
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "try_lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Entity::try_lock_shared(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        ++inner_lock_count;
                    }
                }
            }

            if (already_have_lock)
            {
                return true;
            }
            else if (entity_lock.try_lock_shared())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "try_lock_shared",
                "Trying to get shared lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Entity::lock_shared(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        ++inner_lock_count;
                    }
                }
            }

            if (not already_have_lock)
            {
                entity_lock.lock_shared();
            }

            return true;
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "lock_shared",
                "Trying to get shared lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Entity::unlock(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;
            bool primary_unlock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        if (not inner_lock_count)
                        {
                            primary_unlock = true;
                        }
                        else
                        {
                            --inner_lock_count;
                        }
                    }
                }
            }

            // Now that all changes have completed, call the listener.
            // This allows for batch changes.
            //
            if (already_have_lock and primary_unlock)
            {
                notify_db_listener();
                entity_lock.unlock();

                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);
                locked_thread_id_valid = false;
            }

            return true;
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "unlock",
                "Trying to release exclusive lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Entity::unlock_shared(void)
    {
        try
        {
            osinterface::ThreadUtils::ThreadId my_thread_id =
                osinterface::ThreadUtils::get_thread_id();
            bool already_have_lock = false;

            // Scope for mutex
            {
                // See if this thread already has the lock
                //
                boost::lock_guard<boost::mutex> thread_lock(
                    exclusive_thread_lock);

                if (locked_thread_id_valid)
                {
                    already_have_lock =
                        osinterface::ThreadUtils::thread_id_equal(
                            locked_thread_id,
                            my_thread_id);

                    if (already_have_lock)
                    {
                        if (not inner_lock_count)
                        {
                            LOG(fatal, "dbtype", "unlock_shared",
                              "Unlocking too many times on exclusive thread!");
                        }
                        else
                        {
                            --inner_lock_count;
                        }
                    }
                }
            }

            if (not already_have_lock)
            {
                entity_lock.unlock_shared();
            }

            return true;
        }
        catch (...)
        {
            LOG(fatal, "dbtype", "unlock_shared",
                "Trying to release shared lock threw an exception!");
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Entity::notify_field_changed(const EntityField field)
    {
        if ((not ignore_changes) and (! db_listeners.empty()))
        {
            dirty_flag = true;
            dirty_fields.insert(field);
            diff_callback_fields.insert(field);
            need_call_listener = true;
        }
    }

    // -----------------------------------------------------------------------
    void Entity::notify_db_listener(void)
    {
        if (need_call_listener and (! db_listeners.empty()))
        {
            // Something changed, so set the 'last updated' timestamp to now.
            // Only change the field if more than just the access field and
            // access count changed, since that isn't considered an update.
            //
            if (not ((dirty_fields.size() <= 2) and
                dirty_fields.find(ENTITYFIELD_accessed_timestamp)
                  != dirty_fields.end()))
            {
                entity_updated_timestamp.set_to_now();
                diff_callback_fields.insert(ENTITYFIELD_updated_timestamp);
                dirty_fields.insert(ENTITYFIELD_updated_timestamp);
            }

            for (DbListeners::iterator iter = db_listeners.begin();
                iter != db_listeners.end();
                ++iter)
            {
                (*iter)->entity_changed(
                    this,
                    diff_callback_fields,
                    diff_flags_changed,
                    diff_ids_changed);
            }

            need_call_listener = false;
            diff_callback_fields.clear();
            diff_flags_changed.first.clear();
            diff_flags_changed.second.clear();
            diff_ids_changed.clear();
        }
    }

    // -----------------------------------------------------------------------
    void Entity::clear_entity_references_field(void)
    {
        // Clear the array first
        for (FieldIdsArray::iterator ref_iter =
            entity_references_field.begin();
             ref_iter != entity_references_field.end();
             ++ref_iter)
        {
            delete *ref_iter;
            *ref_iter = 0;
        }
    }

    // -----------------------------------------------------------------------
    void Entity::populate_entity_references_field(void)
    {
        clear_entity_references_field();

        IdSet *ids_ptr = 0;

        for (IdFieldsMap::const_iterator refs_iter =
                entity_references.begin();
            refs_iter != entity_references.end();
             ++refs_iter)
        {
            for (EntityFieldSet::const_iterator field_iter =
                    refs_iter->second.begin();
                field_iter != refs_iter->second.end();
                ++field_iter)
            {
                ids_ptr = entity_references_field[*field_iter];

                if (not ids_ptr)
                {
                    ids_ptr = new IdSet();
                    entity_references_field[*field_iter] = ids_ptr;
                }

                ids_ptr->insert(refs_iter->first);
            }
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
