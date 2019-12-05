/*
 * dbtype_Puppet.cpp
 */

#include <string>
#include <iostream>

#include "dbtypes/dbtype_Puppet.h"
#include "dbtypes/dbtype_Thing.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#include "utilities/mutgos_config.h"
#include "text/text_Utf8Tools.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Puppet::Puppet()
      : Thing()
    {
    }

    // ----------------------------------------------------------------------
    Puppet::Puppet(const Id &id)
      : Thing(id, ENTITYTYPE_puppet, 0, 0)
    {
    }

    // ----------------------------------------------------------------------
    Puppet::~Puppet()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Puppet::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Puppet(
                id,
                ENTITYTYPE_puppet,
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
    std::string Puppet::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << Thing::to_string()
        << "Puppet display name: " << puppet_display_name
        << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Puppet::set_entity_name(
        const std::string &name,
        mutgos::concurrency::WriterLockToken &token)
    {
        if (text::utf8_size(name) > config::db::limits_player_puppet_name())
        {
            // Exceeds size.
            return false;
        }

        return Thing::set_entity_name(name, token);
    }

    // ----------------------------------------------------------------------
    bool Puppet::set_puppet_display_name(
        const std::string &name,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (text::utf8_size(name) > config::db::limits_string_size())
        {
            // Exceeds size.
            return false;
        }

        if (token.has_lock(*this))
        {
            puppet_display_name = name;
            notify_field_changed(ENTITYFIELD_puppet_display_name);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_puppet_display_name",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Puppet::set_puppet_display_name(
        const std::string &name)
    {
        concurrency::WriterLockToken token(*this);

        return set_puppet_display_name(name, token);
    }

    // ----------------------------------------------------------------------
    std::string Puppet::get_puppet_display_name(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = puppet_display_name;
        }
        else
        {
            LOG(error, "dbtype", "get_puppet_display_name",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string Puppet::get_puppet_display_name(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_puppet_display_name(token);
    }

    // ----------------------------------------------------------------------
    Puppet::Puppet(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : Thing(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    size_t Puppet::mem_used_fields(void)
    {
        size_t total_memory = Thing::mem_used_fields();

        total_memory += puppet_display_name.size() + sizeof(puppet_display_name);

        return total_memory;
    }

    // ----------------------------------------------------------------------
    void Puppet::copy_fields(Entity *entity_ptr)
    {
        Thing::copy_fields(entity_ptr);

        Puppet *cast_ptr = 0;

        // Only copy if this is also a Puppet.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Puppet *>(entity_ptr))) != 0))
        {
            cast_ptr->puppet_display_name = puppet_display_name;
            cast_ptr->notify_field_changed(ENTITYFIELD_puppet_display_name);
        }
    }

} /* namespace dbtype */
} /* namespace mutgos */