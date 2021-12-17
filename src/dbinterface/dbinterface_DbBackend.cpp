/*
 * dbinterface_DbBackend.cpp
 */

#include "dbinterface_DbBackend.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Id.h"
#include "concurrency/concurrency_WriterLockToken.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "dbtypes/dbtype_Group.h"
#include "dbtypes/dbtype_Capability.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Region.h"
#include "dbtypes/dbtype_Room.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Guest.h"
#include "dbtypes/dbtype_Thing.h"
#include "dbtypes/dbtype_Puppet.h"
#include "dbtypes/dbtype_Vehicle.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_Command.h"
#include "dbtypes/dbtype_Exit.h"

#include <boost/thread/shared_mutex.hpp>

namespace mutgos
{
namespace dbinterface
{
    // ----------------------------------------------------------------------
    DbBackend::DbBackend(void)
    {
    }

    // ----------------------------------------------------------------------
    DbBackend::~DbBackend()
    {
    }

    // ----------------------------------------------------------------------
    bool DbBackend::init()
    {
        return true;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::shutdown()
    {
        return true;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::added_mem_owned(dbtype::Entity *entity_ptr)
    {
        bool success = entity_ptr;

        if (success)
        {
            boost::unique_lock<boost::shared_mutex> write_lock(
                entity_mem_map_mutex);

            success =
                owned_entity_mem_map[entity_ptr->get_entity_id().get_site_id()].
                  insert(std::make_pair(
                      entity_ptr->get_entity_id().get_entity_id(),
                      entity_ptr)).second;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::removed_mem_owned(dbtype::Entity *entity_ptr)
    {
        bool success = entity_ptr;

        if (success)
        {
            boost::unique_lock<boost::shared_mutex> write_lock(
                entity_mem_map_mutex);

            success =
                owned_entity_mem_map[entity_ptr->get_entity_id().get_site_id()].
                  erase(entity_ptr->get_entity_id().get_entity_id()) > 0;

            // Even if the site is empty now, leave it as-is, because it's
            // likely to be used again at some point in the future.
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::is_mem_owned(const dbtype::Entity *entity_ptr)
    {
        bool success = entity_ptr;

        if (success)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                entity_mem_map_mutex);

            OwnedSiteMemMap::iterator site_iter = owned_entity_mem_map.
                find(entity_ptr->get_entity_id().get_site_id());

            if (site_iter == owned_entity_mem_map.end())
            {
                success = false;
            }
            else
            {
                success = site_iter->second.find(
                    entity_ptr->get_entity_id().get_entity_id()) !=
                        site_iter->second.end();
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::is_mem_owned(const dbtype::Id &id)
    {
        bool success = not id.is_default();

        if (success)
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                entity_mem_map_mutex);

            OwnedSiteMemMap::iterator site_iter =
                owned_entity_mem_map.find(id.get_site_id());

            if (site_iter == owned_entity_mem_map.end())
            {
                success = false;
            }
            else
            {
                success = site_iter->second.count(id.get_entity_id());
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity *DbBackend::get_entity_pointer(const dbtype::Id &id)
    {
        dbtype::Entity *result_ptr = 0;

        if (not id.is_default())
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                entity_mem_map_mutex);

            OwnedSiteMemMap::iterator site_iter =
                owned_entity_mem_map.find(id.get_site_id());

            if (site_iter != owned_entity_mem_map.end())
            {
                OwnedEntityMemMap::iterator iter =
                    site_iter->second.find(id.get_entity_id());

                if (iter != site_iter->second.end())
                {
                    result_ptr = iter->second;
                }
            }
        }

        return result_ptr;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::any_mem_owned(void)
    {
        bool owned = false;
        boost::shared_lock<boost::shared_mutex> read_lock(
            entity_mem_map_mutex);

        for (OwnedSiteMemMap::const_iterator site_iter =
                owned_entity_mem_map.begin();
            site_iter != owned_entity_mem_map.end();
            ++site_iter)
        {
            if (not site_iter->second.empty())
            {
                owned = true;
                break;
            }
        }

        return owned;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity* DbBackend::make_new_entity(
        const dbtype::EntityType type,
        const dbtype::Id &id,
        const dbtype::Id &owner,
        const std::string &name)
    {
        dbtype::Entity *entity_ptr = 0;

        switch (type)
        {
            case dbtype::ENTITYTYPE_group:
            {
                entity_ptr = new dbtype::Group(id);
                break;
            }

            case dbtype::ENTITYTYPE_capability:
            {
                entity_ptr = new dbtype::Capability(id);
                break;
            }

            case dbtype::ENTITYTYPE_container_property_entity:
            {
                entity_ptr = new dbtype::ContainerPropertyEntity(id);
                break;
            }

            case dbtype::ENTITYTYPE_region:
            {
                entity_ptr = new dbtype::Region(id);
                break;
            }

            case dbtype::ENTITYTYPE_room:
            {
                entity_ptr = new dbtype::Room(id);
                break;
            }

            case dbtype::ENTITYTYPE_player:
            {
                entity_ptr = new dbtype::Player(id);
                break;
            }

            case dbtype::ENTITYTYPE_guest:
            {
                entity_ptr = new dbtype::Guest(id);
                break;
            }

            case dbtype::ENTITYTYPE_thing:
            {
                entity_ptr = new dbtype::Thing(id);
                break;
            }

            case dbtype::ENTITYTYPE_puppet:
            {
                entity_ptr = new dbtype::Puppet(id);
                break;
            }

            case dbtype::ENTITYTYPE_vehicle:
            {
                entity_ptr = new dbtype::Vehicle(id);
                break;
            }

            case dbtype::ENTITYTYPE_program:
            {
                entity_ptr = new dbtype::Program(id);
                break;
            }

            case dbtype::ENTITYTYPE_exit:
            {
                entity_ptr = new dbtype::Exit(id);
                break;
            }

            case dbtype::ENTITYTYPE_command:
            {
                entity_ptr = new dbtype::Command(id);
                break;
            }

            default:
            {
            }
        }

        if (entity_ptr)
        {
            concurrency::WriterLockToken token(*entity_ptr);

            entity_ptr->set_entity_name(name, token);
            entity_ptr->set_entity_owner(owner, token);
        }

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity* DbBackend::make_deserialize_entity(
        const dbtype::EntityType type,
        utility::MemoryBuffer &buffer)
    {
        dbtype::Entity *entity_ptr = 0;
        boost::archive::binary_iarchive archive(buffer);

        switch (type)
        {
            case dbtype::ENTITYTYPE_group:
            {
                dbtype::Group *group_ptr = new dbtype::Group();
                archive >> *group_ptr;

                entity_ptr = group_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_capability:
            {
                dbtype::Capability *capability_ptr = new dbtype::Capability();
                archive >> *capability_ptr;

                entity_ptr = capability_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_container_property_entity:
            {
                dbtype::ContainerPropertyEntity *containerprop_ptr =
                    new dbtype::ContainerPropertyEntity();
                archive >> *containerprop_ptr;

                entity_ptr = containerprop_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_region:
            {
                dbtype::Region *region_ptr = new dbtype::Region();
                archive >> *region_ptr;

                entity_ptr = region_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_room:
            {
                dbtype::Room *room_ptr = new dbtype::Room();
                archive >> *room_ptr;

                entity_ptr = room_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_player:
            {
                dbtype::Player *player_ptr = new dbtype::Player();
                archive >> *player_ptr;

                entity_ptr = player_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_guest:
            {
                dbtype::Guest *guest_ptr = new dbtype::Guest();
                archive >> *guest_ptr;

                entity_ptr = guest_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_thing:
            {
                dbtype::Thing *thing_ptr = new dbtype::Thing();
                archive >> *thing_ptr;

                entity_ptr = thing_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_puppet:
            {
                dbtype::Puppet *puppet_ptr = new dbtype::Puppet();
                archive >> *puppet_ptr;

                entity_ptr = puppet_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_vehicle:
            {
                dbtype::Vehicle *vehicle_ptr = new dbtype::Vehicle();
                archive >> *vehicle_ptr;

                entity_ptr = vehicle_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_program:
            {
                dbtype::Program *program_ptr = new dbtype::Program();
                archive >> *program_ptr;

                entity_ptr = program_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_exit:
            {
                dbtype::Exit *exit_ptr = new dbtype::Exit();
                archive >> *exit_ptr;

                entity_ptr = exit_ptr;
                break;
            }

            case dbtype::ENTITYTYPE_command:
            {
                dbtype::Command *command_ptr = new dbtype::Command();
                archive >> *command_ptr;

                entity_ptr = command_ptr;
                break;
            }

            default:
            {
            }
        }

        if (entity_ptr)
        {
            entity_ptr->restore_complete();
        }

        return entity_ptr;
    }

    // ----------------------------------------------------------------------
    bool DbBackend::serialize_entity(
        dbtype::Entity *entity_ptr,
        utility::MemoryBuffer &buffer)
    {
        bool success = entity_ptr;

        if (success)
        {
            boost::archive::binary_oarchive archive(buffer);

            switch (entity_ptr->get_entity_type())
            {
                case dbtype::ENTITYTYPE_group:
                {
                    dbtype::Group *group_ptr =
                        dynamic_cast<dbtype::Group *>(entity_ptr);

                    if (group_ptr)
                    {
                        archive << (*group_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_capability:
                {
                    dbtype::Capability *capability_ptr =
                        dynamic_cast<dbtype::Capability *>(entity_ptr);

                    if (capability_ptr)
                    {
                        archive << (*capability_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_container_property_entity:
                {
                    dbtype::ContainerPropertyEntity *contprop_ptr =
                        dynamic_cast<dbtype::ContainerPropertyEntity *>(
                            entity_ptr);

                    if (contprop_ptr)
                    {
                        archive << (*contprop_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_region:
                {
                    dbtype::Region *region_ptr =
                        dynamic_cast<dbtype::Region *>(entity_ptr);

                    if (region_ptr)
                    {
                        archive << (*region_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_room:
                {
                    dbtype::Room *room_ptr =
                        dynamic_cast<dbtype::Room *>(entity_ptr);

                    if (room_ptr)
                    {
                        archive << (*room_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_player:
                {
                    dbtype::Player *player_ptr =
                        dynamic_cast<dbtype::Player *>(entity_ptr);

                    if (player_ptr)
                    {
                        archive << (*player_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_guest:
                {
                    dbtype::Guest *guest_ptr =
                        dynamic_cast<dbtype::Guest *>(entity_ptr);

                    if (guest_ptr)
                    {
                        archive << (*guest_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_thing:
                {
                    dbtype::Thing *thing_ptr =
                        dynamic_cast<dbtype::Thing *>(entity_ptr);

                    if (thing_ptr)
                    {
                        archive << (*thing_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_puppet:
                {
                    dbtype::Puppet *puppet_ptr =
                        dynamic_cast<dbtype::Puppet *>(entity_ptr);

                    if (puppet_ptr)
                    {
                        archive << (*puppet_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_vehicle:
                {
                    dbtype::Vehicle *vehicle_ptr =
                        dynamic_cast<dbtype::Vehicle *>(entity_ptr);

                    if (vehicle_ptr)
                    {
                        archive << (*vehicle_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_program:
                {
                    dbtype::Program *program_ptr =
                        dynamic_cast<dbtype::Program *>(entity_ptr);

                    if (program_ptr)
                    {
                        archive << (*program_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_exit:
                {
                    dbtype::Exit *exit_ptr =
                        dynamic_cast<dbtype::Exit *>(entity_ptr);

                    if (exit_ptr)
                    {
                        archive << (*exit_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                case dbtype::ENTITYTYPE_command:
                {
                    dbtype::Command *command_ptr =
                        dynamic_cast<dbtype::Command *>(entity_ptr);

                    if (command_ptr)
                    {
                        archive << (*command_ptr);
                    }
                    else
                    {
                        success = false;
                    }

                    break;
                }

                default:
                {
                    // Not a type we can serialize.
                    success = false;
                }
            }
        }

        return success;
    }
}
}
