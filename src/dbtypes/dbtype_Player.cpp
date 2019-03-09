/*
 * Player.cpp
 */

#include <iostream>

#include <string>

#include "dbtype_Player.h"

#include "dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtype_ContainerPropertyEntity.h"

#include "bcrypt/BCrypt.hpp"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    // TODO(hyena): Consider making this a config parameter?
    const int password_workfactor = 10;

    // ----------------------------------------------------------------------
    Player::Player()
        : ContainerPropertyEntity(),
          player_last_connect(false)
    {
    }

    // ----------------------------------------------------------------------
    Player::Player(const Id &id)
        : ContainerPropertyEntity(id, ENTITYTYPE_player, 0, 0),
          player_last_connect(false)
    {
    }

    // ----------------------------------------------------------------------
    Player::~Player()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Player::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Player(
                id,
                ENTITYTYPE_player,
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
    size_t Player::mem_used_fields(void)
    {
        size_t total_memory = ContainerPropertyEntity::mem_used_fields();

        total_memory += sizeof(encrypted_password) + encrypted_password.size()
                     +  sizeof(display_name) + display_name.size()
                     +  player_home.mem_used()
                     +  player_last_connect.mem_used();

        return total_memory;
    }

    // ----------------------------------------------------------------------
    std::string Player::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << ContainerPropertyEntity::to_string()
                  << "Encrypted Password length: " << encrypted_password.size()
                  << std::endl
                  << "Display name: " << display_name << std::endl
                  << "Home: " << player_home.to_string() << std::endl
                  << "Last connect: " << player_last_connect.to_string()
                  << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Player::set_password(
        const std::string &new_password,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            encrypted_password = BCrypt::generateHash(new_password, password_workfactor);
            notify_field_changed(ENTITYFIELD_password);

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_password",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Player::set_password(const std::string &new_password)
    {
        concurrency::WriterLockToken token(*this);

        return set_password(new_password, token);
    }

    // ----------------------------------------------------------------------
    bool Player::check_password(
        const std::string &password,
        concurrency::ReaderLockToken &token)
    {
        std::cout << "Player::check_password(): Add encryption!" << std::endl;

        bool success = false;

        if (token.has_lock(*this))
        {
            success = BCrypt::validatePassword(password, encrypted_password);
        }
        else
        {
            LOG(error, "dbtype", "check_password",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Player::check_password(const std::string &password)
    {
        concurrency::ReaderLockToken token(*this);

        return check_password(password, token);
    }

    // ----------------------------------------------------------------------
    bool Player::set_display_name(
        const std::string &name,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            display_name = name;
            notify_field_changed(ENTITYFIELD_player_display_name);
            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_display_name",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Player::set_display_name(const std::string &name)
    {
        concurrency::WriterLockToken token(*this);

        return set_display_name(name, token);
    }

    // ----------------------------------------------------------------------
    std::string Player::get_display_name(
        const bool fallback,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (display_name.empty() and fallback)
            {
                // Doesn't have a display name, so use Entity name
                return get_entity_name(token);
            }
            else
            {
                return display_name;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_display_name",
                "Using the wrong lock token!");
        }

        return "";
    }

    // ----------------------------------------------------------------------
    std::string Player::get_display_name(const bool fallback)
    {
        concurrency::ReaderLockToken token(*this);

        return get_display_name(fallback, token);
    }

    // ----------------------------------------------------------------------
    bool Player::set_player_home(const Id &home,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            if (home != player_home)
            {
                set_single_id_field(ENTITYFIELD_player_home, player_home, home);
                player_home = home;
                notify_field_changed(ENTITYFIELD_player_home);
            }

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_player_home",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Player::set_player_home(const Id &home)
    {
        concurrency::WriterLockToken token(*this);

        return set_player_home(home, token);
    }

    // ----------------------------------------------------------------------
    Id Player::get_player_home(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            result = player_home;
        }
        else
        {
            LOG(error, "dbtype", "get_player_home",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Player::get_player_home(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_player_home(token);
    }

    // ----------------------------------------------------------------------
    bool Player::set_last_connect_to_now(concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            player_last_connect.set_to_now();
            notify_field_changed(ENTITYFIELD_player_last_connect);

            success = true;
        }
        else
        {
            LOG(error, "dbtype", "set_last_connect_to_now",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Player::set_last_connect_to_now(void)
    {
        concurrency::WriterLockToken token(*this);

        return set_last_connect_to_now(token);
    }

    // ----------------------------------------------------------------------
    TimeStamp Player::get_last_connect(concurrency::ReaderLockToken &token)
    {
        TimeStamp result(false);

        if (token.has_lock(*this))
        {
            result = player_last_connect;
        }
        else
        {
            LOG(error, "dbtype", "get_last_connect",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    TimeStamp Player::get_last_connect(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_connect(token);
    }

    // ----------------------------------------------------------------------
    Player::Player(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : ContainerPropertyEntity(id, type, version, instance, restoring),
        player_last_connect(false)
    {
    }

    // ----------------------------------------------------------------------
    void Player::copy_fields(Entity *entity_ptr)
    {
        ContainerPropertyEntity::copy_fields(entity_ptr);

        Player *cast_ptr = 0;

        // Only copy if this is also a Player.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Player *>(entity_ptr))) != 0))
        {
            cast_ptr->encrypted_password = encrypted_password;
            cast_ptr->notify_field_changed(ENTITYFIELD_password);

            cast_ptr->display_name = display_name;
            cast_ptr->notify_field_changed(ENTITYFIELD_player_display_name);

            cast_ptr->set_single_id_field(
                ENTITYFIELD_player_home,
                cast_ptr->player_home,
                player_home);
            cast_ptr->player_home = player_home;
            cast_ptr->notify_field_changed(ENTITYFIELD_player_home);

            cast_ptr->player_last_connect = player_last_connect;
            cast_ptr->notify_field_changed(ENTITYFIELD_player_last_connect);
        }
    }


} /* namespace dbtype */
} /* namespace mutgos */
