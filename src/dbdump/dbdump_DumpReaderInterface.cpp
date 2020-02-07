#include <string>
#include <algorithm>

#include "osinterface/osinterface_OsTypes.h"
#include "text/text_StringConversion.h"

#include "dbdump_DumpReaderInterface.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Lock.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Thing.h"
#include "dbtypes/dbtype_Puppet.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_ActionEntity.h"
#include "dbtypes/dbtype_Exit.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Vehicle.h"

#include "dbtypes/dbtype_PropertyDataType.h"

#include "logging/log_Logger.h"

namespace
{
    const std::string TEMP_NAME_PREFIX = "TEMPNAME_DUMP_READER_";
}

namespace mutgos
{
namespace dbdump
{
    // ----------------------------------------------------------------------
    DumpReaderInterface::DumpReaderInterface()
      : db(dbinterface::DatabaseAccess::make_singleton()),
        current_site_id(0),
        site_valid(false),
        mode(NORMAL),
        temp_ser_id_name(0)
    {
        db->startup();
    }

    // ----------------------------------------------------------------------
    DumpReaderInterface::~DumpReaderInterface()
    {
        if (site_valid)
        {
            LOG(warning, "dbdump", "~DumpReaderInterface",
                "Site was not ended before destructing interface!");
        }

        db->destroy_singleton();
    }

    // ----------------------------------------------------------------------
    void DumpReaderInterface::set_error(void)
    {
        current_entity.clear();
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::make_site(
        const std::string &site_name,
        dbtype::Id::SiteIdType &site_id)
    {
        bool result = false;

        if (site_valid)
        {
            LOG(error, "dbdump", "make_site",
                "Tried to make a new site when a site is already active!");
        }
        else
        {
            result = db->new_site(current_site_id)
                 == dbinterface::DBRESULTCODE_OK;

            site_valid = result;

            if (result)
            {
                site_id = current_site_id;

                result = db->set_site_name(current_site_id, site_name)
                    == dbinterface::DBRESULTCODE_OK;
            }

            LOG(debug, "dbdump", "make_site",
                "Made new site, ID " + text::to_string(site_id)
                + "  name " + site_name
                + ",  result: " + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_site_description(
        const std::string &description)
    {
        bool result = false;

        if (not site_valid)
        {
            LOG(error, "dbdump", "set_site_description",
                "Tried to set a site description when no site is active!");
        }
        else
        {
            result = db->set_site_description(current_site_id, description)
                 == dbinterface::DBRESULTCODE_OK;

            LOG(debug, "dbdump", "set_site_description",
                "Set site description, ID "
                + text::to_string(current_site_id)
                + "  description " + description
                + ",  result: " + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_site(const dbtype::Id::SiteIdType id)
    {
        bool result = false;

        if (site_valid)
        {
            LOG(error, "dbdump", "set_site",
                "Tried to set a site ID " + text::to_string(id)
                + " when another site is already active!");
        }
        else
        {
            dbtype::Id::SiteIdVector known_sites = db->get_all_site_ids();

            if (std::find(known_sites.begin(), known_sites.end(), id) !=
                known_sites.end())
            {
                current_site_id = id;
                result = true;
                site_valid = true;
            }

            LOG(debug, "dbdump", "set_site",
                "Set existing site, ID " + text::to_string(id)
                + "  result: " + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::end_site(void)
    {
        bool result = false;

        if (current_entity.valid())
        {
            LOG(error, "dbdump", "end_site",
                "Tried to end a site when an entity is selected!  Site ID "
                + text::to_string(current_site_id));
        }
        else
        {
            LOG(debug, "dbdump", "end_site", "Ending site "
               + text::to_string(current_site_id));

            site_valid = false;
            current_site_id = 0;
            result = true;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Id DumpReaderInterface::make_entity(const dbtype::EntityType type)
    {
        dbtype::Id result;

        if ((not site_valid) || current_entity.valid())
        {
            LOG(error, "dbdump", "make_entity",
                "Tried to make an entity when a site ID has not been set or"
                "an Entity has already been selected.");
        }
        else
        {
            // Make a temporary, possibly invalid owner ID to satisfy database.
            // It will be switched to the ID of the newly created entity
            // immediately after.  Later on in the dump, the actual owner
            // will be set.
            //
            const dbtype::Id temp_owner(current_site_id, 1);
            const std::string temp_name =
                TEMP_NAME_PREFIX + text::to_string(temp_ser_id_name);
            const dbinterface::DbResultCode rc = db->new_entity(
                  type, current_site_id, temp_owner, temp_name, current_entity);

            ++temp_ser_id_name;

            if (rc == dbinterface::DBRESULTCODE_OK)
            {
                // All set!
                result = current_entity.id();

                LOG(debug, "dbdump", "make_entity", "Created Entity "
                    + result.to_string(true) + " of type "
                    + dbtype::entity_type_to_string(type));

                // Players always own themselves by default, other
                // Entities only temporarily own themself until the real
                // owner is known.
                if (not set_entity_owner(result))
                {
                    LOG(error, "dbdump", "make_entity",
                        "Unable to set owner of Entity to itself!");
                }
            }
            else
            {
                // Bad type, likely.
                LOG(error, "dbdump", "make_entity", "Unable to make new Entity "
                    "of type " + temp_name + " due to error "
                    + dbinterface::db_result_code_to_string(rc));
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity(const dbtype::Id &entity_id)
    {
        bool result = false;

        if ((not site_valid) || current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity",
                "Tried to set an entity when a site ID has not been set or"
                    "an Entity has already been selected.");
        }
        else
        {
            const dbtype::Id id(current_site_id, entity_id.get_entity_id());

            current_entity = db->get_entity(id);
            result = current_entity.valid();

            if (result)
            {
                LOG(debug, "dbdump", "set_entity", "Set entity "
                    + id.to_string(true));
            }
            else
            {
                LOG(error, "dbdump", "set_entity", "Failed to set entity "
                    + id.to_string(true));
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void DumpReaderInterface::log_entity(void)
    {
        if (current_entity.valid())
        {
            LOG(info, "dbdump", "log_entity",
                "\n" + current_entity.get()->to_string());
        }
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::end_entity(void)
    {
        bool result = false;

        if (mode != NORMAL)
        {
            LOG(error, "dbdump", "end_entity",
                "Tried to end an entity " + current_entity.id().to_string(true)
                + " when it was in a mode.");
        }
        else if (not site_valid)
        {
            LOG(error, "dbdump", "end_entity",
                "Tried to end an entity when no site was selected!");
        }
        else
        {
            LOG(debug, "dbdump", "end_entity", "Ended entity "
                + current_entity.id().to_string(true));

            current_entity.clear();
            result = true;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_name(const std::string &name)
    {
        bool result = false;

        if (current_entity.valid())
        {
            result = current_entity.get()->set_entity_name(name);

            LOG(result ? debug : error, "dbdump", "set_entity_name",
                "Set name of Entity "
                + current_entity.id().to_string(true)
                + " to " + name + ",  result:"
                + text::to_string(result));
        }
        else
        {
            LOG(error, "dbdump", "set_entity_name", "Could not set name of Entity "
                "because no Entity has been selected.");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_owner(const dbtype::Id &owner)
    {
        bool result = false;

        if (current_entity.valid())
        {
            result = current_entity.get()->set_entity_owner(owner);

            LOG(result ? debug : error, "dbdump", "set_entity_owner",
                "Set owner of Entity "
                + current_entity.id().to_string(true)
                + " to " + owner.to_string(true) + ",  result:"
                + text::to_string(result));
        }
        else
        {
            LOG(error, "dbdump", "set_entity_owner", "Could not set owner of Entity "
                "because no Entity has been selected.");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_entity_flag(
        const dbtype::Entity::FlagType &value)
    {
        bool result = false;

        if (current_entity.valid())
        {
            result = (current_entity.get()->add_entity_flag(value) ==
                dbtype::Entity::FLAGRC_success);

            LOG(result ? debug : error, "dbdump", "add_entity_flag",
                "Addding flag to Entity "
                 + current_entity.id().to_string(true)
                 + " of " + value + ",  result:"
                 + text::to_string(result));
        }
        else
        {
            LOG(error, "dbdump", "add_entity_flag", "Could add flag to Entity "
                 "because no Entity has been selected.");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    DumpReaderInterface::SetFieldMethod DumpReaderInterface::which_set_field_method(
        const dbtype::EntityField field) const
    {
        SetFieldMethod method = METHOD_invalid;

        switch (field)
        {
            // String
            case dbtype::ENTITYFIELD_name:
            case dbtype::ENTITYFIELD_note:
            case dbtype::ENTITYFIELD_reg_name:
            case dbtype::ENTITYFIELD_reg_category:
            case dbtype::ENTITYFIELD_password:
            case dbtype::ENTITYFIELD_player_display_name:
            case dbtype::ENTITYFIELD_puppet_display_name:
            case dbtype::ENTITYFIELD_program_language:
            case dbtype::ENTITYFIELD_action_succ_msg:
            case dbtype::ENTITYFIELD_action_succ_room_msg:
            case dbtype::ENTITYFIELD_action_fail_msg:
            case dbtype::ENTITYFIELD_action_fail_room_msg:
            case dbtype::ENTITYFIELD_exit_arrive_msg:
            case dbtype::ENTITYFIELD_exit_arrive_room_msg:
            {
                method = METHOD_string;
                break;
            }

            // String (multiple)
            case dbtype::ENTITYFIELD_action_commands:
            {
                method = METHOD_string_multiple;
                break;
            }

            // ID
            case dbtype::ENTITYFIELD_owner:
            case dbtype::ENTITYFIELD_contained_by:
            case dbtype::ENTITYFIELD_player_home:
            case dbtype::ENTITYFIELD_thing_home:
            case dbtype::ENTITYFIELD_vehicle_interior:
            case dbtype::ENTITYFIELD_vehicle_controller:
            case dbtype::ENTITYFIELD_action_contained_by:
            {
                method = METHOD_id;
                break;

            }

            // ID (multiple)
            case dbtype::ENTITYFIELD_group_ids:
            case dbtype::ENTITYFIELD_linked_programs:
            case dbtype::ENTITYFIELD_action_targets:
            {
                method = METHOD_id_multiple;
                break;
            }

            // Document
            case dbtype::ENTITYFIELD_program_source_code:
            {
                method = METHOD_document;
                break;
            }

            // Locks
            case dbtype::ENTITYFIELD_thing_lock:
            case dbtype::ENTITYFIELD_action_lock:
            {
                method = METHOD_lock;
                break;
            }

            // Anything else is not valid to be set
            default:
            {
                break;
            }
        }

        return method;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_field(
        const dbtype::EntityField field,
        const std::string &value)
    {
        bool result = false;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_field(string)",
                "Could not set string on Entity "
                "because no Entity has been selected.");
        }
        else
        {
            const SetFieldMethod method = which_set_field_method(field);

            if (method == METHOD_string)
            {
                switch (field)
                {
                    case dbtype::ENTITYFIELD_name:
                    {
                        result = current_entity.get()->set_entity_name(value);
                        break;
                    }

                    case dbtype::ENTITYFIELD_note:
                    {
                        result = current_entity.get()->set_entity_note(value);
                        break;
                    }

                    case dbtype::ENTITYFIELD_reg_name:
                    {
                        result = current_entity.get()->
                            set_entity_registration_name(value);
                        break;
                    }

                    case dbtype::ENTITYFIELD_reg_category:
                    {
                        result = current_entity.get()->
                            set_entity_registration_category(value);
                        break;
                    }

                    case dbtype::ENTITYFIELD_password:
                    case dbtype::ENTITYFIELD_player_display_name:
                    {
                        dbtype::Player *player_ptr =
                            dynamic_cast<dbtype::Player *>(current_entity.get());

                        if (not player_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            switch (field)
                            {
                                case dbtype::ENTITYFIELD_password:
                                {
                                    result = player_ptr->set_password(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_player_display_name:
                                {
                                    result = player_ptr->set_display_name(value);
                                    break;
                                }

                                default:
                                {
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_puppet_display_name:
                    {
                        dbtype::Puppet *puppet_ptr =
                            dynamic_cast<dbtype::Puppet *>(current_entity.get());

                        if (not puppet_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = puppet_ptr->set_puppet_display_name(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_program_language:
                    {
                        dbtype::Program *program_ptr =
                            dynamic_cast<dbtype::Program *>(current_entity.get());

                        if (not program_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = program_ptr->set_program_language(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_action_succ_msg:
                    case dbtype::ENTITYFIELD_action_succ_room_msg:
                    case dbtype::ENTITYFIELD_action_fail_msg:
                    case dbtype::ENTITYFIELD_action_fail_room_msg:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            switch (field)
                            {
                                case dbtype::ENTITYFIELD_action_succ_msg:
                                {
                                    result = action_ptr->
                                        set_action_success_message(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_action_succ_room_msg:
                                {
                                    result = action_ptr->
                                        set_action_success_room_message(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_action_fail_msg:
                                {
                                    result = action_ptr->
                                        set_action_fail_message(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_action_fail_room_msg:
                                {
                                    result = action_ptr->
                                        set_action_fail_room_message(value);
                                    break;
                                }

                                default:
                                {
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_exit_arrive_msg:
                    case dbtype::ENTITYFIELD_exit_arrive_room_msg:
                    {
                        dbtype::Exit *exit_ptr =
                            dynamic_cast<dbtype::Exit *>(current_entity.get());

                        if (not exit_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            switch (field)
                            {
                                case dbtype::ENTITYFIELD_exit_arrive_msg:
                                {
                                    result = exit_ptr->
                                        set_exit_arrive_message(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_exit_arrive_room_msg:
                                {
                                    result = exit_ptr->
                                        set_exit_arrive_room_message(value);
                                    break;
                                }

                                default:
                                {
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    default:
                    {
                        LOG(error, "dbdump", "set_entity_field(string)",
                            "Could not set string on Entity "
                            + current_entity.id().to_string(true)
                            + ", field "
                            + dbtype::entity_field_to_string(field)
                            + " because the field is not known.");
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(string)",
                    "Set string \""
                    + value
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else if (method == METHOD_string_multiple)
            {
                switch (field)
                {
                    case dbtype::ENTITYFIELD_action_commands:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(string)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            dbtype::ActionEntity::CommandList current_commands =
                                action_ptr->get_action_commands();

                            current_commands.push_back(value);

                            result = action_ptr->set_action_commands(
                                current_commands);
                        }

                        break;
                    }

                    default:
                    {
                        LOG(error, "dbdump", "set_entity_field(string)",
                            "Could not set string list on Entity "
                            + current_entity.id().to_string(true)
                            + ", field "
                            + dbtype::entity_field_to_string(field)
                            + " because the field is not known.");
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(string)",
                    "Added string \""
                    + value
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else
            {
                // Not valid for this method
                LOG(error, "dbdump", "set_entity_field(string)",
                    "Could not set string on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + " because the field is not a string.");
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_field(
        const dbtype::EntityField field,
        const dbtype::Id &value)
    {
        bool result = false;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_field(id)",
                "Could not set ID on Entity "
                    "because no Entity has been selected.");
        }
        else
        {
            const SetFieldMethod method = which_set_field_method(field);

            if (method == METHOD_id)
            {
                switch (field)
                {
                    case dbtype::ENTITYFIELD_owner:
                    {
                        result = current_entity.get()->set_entity_owner(value);
                        break;
                    }

                    case dbtype::ENTITYFIELD_contained_by:
                    {
                        dbtype::ContainerPropertyEntity *container_ptr =
                            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                                current_entity.get());

                        if (not container_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = container_ptr->set_contained_by(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_player_home:
                    {
                        dbtype::Player *player_ptr =
                            dynamic_cast<dbtype::Player *>(current_entity.get());

                        if (not player_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = player_ptr->set_player_home(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_thing_home:
                    {
                        dbtype::Thing *thing_ptr =
                            dynamic_cast<dbtype::Thing *>(current_entity.get());

                        if (not thing_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = thing_ptr->set_thing_home(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_vehicle_interior:
                    case dbtype::ENTITYFIELD_vehicle_controller:
                    {
                        dbtype::Vehicle *vehicle_ptr =
                            dynamic_cast<dbtype::Vehicle *>(current_entity.get());

                        if (not vehicle_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            switch (field)
                            {
                                case dbtype::ENTITYFIELD_vehicle_interior:
                                {
                                    result =
                                        vehicle_ptr->set_vehicle_interior(value);
                                    break;
                                }

                                case dbtype::ENTITYFIELD_vehicle_controller:
                                {
                                    result =
                                        vehicle_ptr->set_vehicle_controller(value);
                                    break;
                                }

                                default:
                                {
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_action_contained_by:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = action_ptr->set_action_contained_by(value);
                        }

                        break;
                    }

                    default:
                    {
                        LOG(error, "dbdump", "set_entity_field(id)",
                            "Could not set ID on Entity "
                            + current_entity.id().to_string(true)
                            + ", field "
                            + dbtype::entity_field_to_string(field)
                            + " because the field is not known.");
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(id)",
                    "Set id \""
                    + value.to_string(true)
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else if (method == METHOD_id_multiple)
            {
                switch (field)
                {
                    case dbtype::ENTITYFIELD_group_ids:
                    {
                        dbtype::Group *group_ptr =
                            dynamic_cast<dbtype::Group *>(current_entity.get());

                        if (not group_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = group_ptr->add_to_group(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_linked_programs:
                    {
                        dbtype::ContainerPropertyEntity *container_ptr =
                            dynamic_cast<dbtype::ContainerPropertyEntity *>(
                                current_entity.get());

                        if (not container_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = container_ptr->add_program(value);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_action_targets:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(id)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = action_ptr->add_action_target(value);
                        }

                        break;
                    }

                    default:
                    {
                        LOG(error, "dbdump", "set_entity_field(id)",
                            "Could not add ID to Entity "
                            + current_entity.id().to_string(true)
                            + ", field "
                            + dbtype::entity_field_to_string(field)
                            + " because the field is not known.");
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(id)",
                    "Added id \""
                    + value.to_string(true)
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else
            {
                // Not valid for this method
                LOG(error, "dbdump", "set_entity_field(id)",
                    "Could not set id on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + " because the field is not an ID.");
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_field(
        const dbtype::EntityField field,
        const dbtype::DocumentProperty &value)
    {
        bool result = false;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_field(DocumentProperty)",
                "Could not set DocumentProperty on Entity "
                    "because no Entity has been selected.");
        }
        else
        {
            const SetFieldMethod method = which_set_field_method(field);

            if (method == METHOD_document)
            {
                switch (field)
                {
                    case dbtype::ENTITYFIELD_program_source_code:
                    {
                        dbtype::Program *program_ptr =
                            dynamic_cast<dbtype::Program *>(
                                current_entity.get());

                        if (not program_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(DocumentProperty)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = program_ptr->set_source_code(value);
                        }

                        break;
                    }

                    default:
                    {
                        LOG(error, "dbdump", "set_entity_field(DocumentProperty)",
                            "Could set DocumentProperty on Entity "
                            + current_entity.id().to_string(true)
                            + ", field "
                            + dbtype::entity_field_to_string(field)
                            + " because the field is not known.");
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(DocumentProperty)",
                    "Set DocumentProperty (lines: "
                    + text::to_string(value.get_number_lines())
                    + ") on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else
            {
                // Not valid for this method
                LOG(error, "dbdump", "set_entity_field(DocumentProperty)",
                    "Could not set DocumentProperty on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + " because the field is not a DocumentProperty.");
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_lock_field(
        const dbtype::EntityField field,
        const dbtype::Id &lock_id,
        const bool is_not)
    {
        bool result = false;
        bool lock_result = false;

        dbinterface::EntityRef lock_id_ref = db->get_entity(lock_id);
        dbtype::Lock lock;

        if (lock_id_ref.valid())
        {
            concurrency::ReaderLockToken token(*lock_id_ref.get());
            lock_result = lock.lock_by_entity(lock_id_ref.get(), token, is_not);
        }

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_field(lock by ID)",
                "Could not set lock on Entity "
                    "because no Entity has been selected.");
        }
        else if (not lock_id_ref.valid())
        {
            LOG(error, "dbdump", "set_entity_field(lock by ID)",
                "Could not set lock on Entity "
                    "because lock ID " + lock_id.to_string(true)
                + " does not exist.");
        }
        else if (not lock_result)
        {
            LOG(error, "dbdump", "set_entity_field(lock by ID)",
                "Could not set lock on Entity "
                    "because lock ID " + lock_id.to_string(true)
                + " could not be locked against.");
        }
        else
        {
            const SetFieldMethod method = which_set_field_method(field);

            if (method == METHOD_lock)
            {
                dbtype::Lock lock;

                switch (field)
                {
                    case dbtype::ENTITYFIELD_thing_lock:
                    {
                        dbtype::Thing *thing_ptr =
                            dynamic_cast<dbtype::Thing *>(current_entity.get());

                        if (not thing_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(lock by ID)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = thing_ptr->set_thing_lock(lock);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_action_lock:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(lock by ID)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = action_ptr->set_action_lock(lock);
                        }

                        break;
                    }

                    default:
                    {
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(lock by ID)",
                    "Set lock by ID \""
                    + lock.to_string()
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else
            {
                // Not valid for this method
                LOG(error, "dbdump", "set_entity_field(lock by ID)",
                    "Could not set lock (by ID) on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + " because the field is not a lock.");
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_lock_field(
        const dbtype::EntityField field,
        const dbtype::PropertyDirectory::PathString &lock_path,
        const dbtype::PropertyData &lock_data,
        const bool is_not)
    {
        bool result = false;
        bool lock_result = false;

        dbtype::Lock lock;

        lock_result = lock.lock_by_property(lock_path, lock_data, is_not);

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_field(lock by property)",
                "Could not set lock on Entity "
                    "because no Entity has been selected.");
        }
        else if (not lock_result)
        {
            LOG(error, "dbdump", "set_entity_field(lock by property)",
                "Could not set lock on Entity "
                    "because the lock was not valid.");
        }
        else
        {
            const SetFieldMethod method = which_set_field_method(field);

            if (method == METHOD_lock)
            {
                dbtype::Lock lock;

                switch (field)
                {
                    case dbtype::ENTITYFIELD_thing_lock:
                    {
                        dbtype::Thing *thing_ptr =
                            dynamic_cast<dbtype::Thing *>(current_entity.get());

                        if (not thing_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(lock by property)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = thing_ptr->set_thing_lock(lock);
                        }

                        break;
                    }

                    case dbtype::ENTITYFIELD_action_lock:
                    {
                        dbtype::ActionEntity *action_ptr =
                            dynamic_cast<dbtype::ActionEntity *>(
                                current_entity.get());

                        if (not action_ptr)
                        {
                            LOG(error, "dbdump", "set_entity_field(lock by property)",
                                "Entity is not the right type for field.  ID "
                                + current_entity.id().to_string(true)
                                + ", field "
                                + dbtype::entity_field_to_string(field)
                                + ", actual type "
                                + dbtype::entity_type_to_string(
                                    current_entity.type()));
                        }
                        else
                        {
                            result = action_ptr->set_action_lock(lock);
                        }

                        break;
                    }

                    default:
                    {
                        break;
                    }
                }

                LOG(result ? debug : error, "dbdump", "set_entity_field(lock by property)",
                    "Set lock by property \""
                    + lock.to_string()
                    + "\" on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + ".  Result: "
                    + text::to_string(result));
            }
            else
            {
                // Not valid for this method
                LOG(error, "dbdump", "set_entity_field(lock by property)",
                    "Could not set lock (by property) on Entity "
                    + current_entity.id().to_string(true)
                    + ", field "
                    + dbtype::entity_field_to_string(field)
                    + " because the field is not a lock.");
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_application(
        const std::string &application,
        const dbtype::Id &owner)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "add_application",
                "Tried to add an Application when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "add_application",
                "Tried to add an Application when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for adding the application.
            //
            result = property_entity_ptr->add_application(
                application,
                owner,
                dbtype::PropertySecurity());

            LOG(result ? debug : error, "dbdump", "add_application",
             "Added application " + application + " with owner "
             + owner.to_string(true) + "   Result: "
             + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_entity_security(void)
    {
        bool result = false;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_entity_security",
                "Tried to set Entity security mode when "
                    "no Entity has been selected!");
        }
        else if (mode != NORMAL)
        {
            LOG(error, "dbdump", "set_entity_security",
                "Tried to set Entity security mode when "
                    "already in another mode!");
        }
        else
        {
            mode = ENTITY_SECURITY;
            result = true;

            LOG(debug, "dbdump", "set_entity_security",
                "Entered Entity security mode.");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_application_props_security(
        const dbtype::PropertyDirectory::PathString &application)
    {
        bool result = false;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_application_props_security",
                "Tried to set application security mode when "
                    "no Entity has been selected!");
        }
        else if (mode != NORMAL)
        {
            LOG(error, "dbdump", "set_application_props_security",
                "Tried to set application security mode when "
                    "already in another mode!");
        }
        else if (application.empty())
        {
            LOG(error, "dbdump", "set_application_props_security",
                "Tried to set application security mode when "
                    "no application has been specified!");
        }
        else if (not dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get()))
        {
            LOG(error, "dbdump", "set_application_props_security",
                "Tried to set application security mode when "
                    "Entity " + current_entity.id().to_string(true)
                    + " does not support application properties!");
        }
        else
        {
            mode = APPLICATION_SECURITY;
            current_application = application;
            result = true;

            LOG(debug, "dbdump", "set_application_props_security",
                "Entered application security mode for application "
                + current_application);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::end_security(void)
    {
        bool result = false;

        if (mode == NORMAL)
        {
            LOG(error, "dbdump", "end_security",
                "Tried to end security mode when not in the mode!");
        }
        else
        {
            mode = NORMAL;
            current_application.clear();
            result = true;

            LOG(debug, "dbdump", "end_security", "Ended security mode.");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_security_flag_list(
        const dbtype::SecurityFlag flag)
    {
        bool result = false;

        if (mode == NORMAL)
        {
            LOG(error, "dbdump", "add_security_flag_list",
                "In the wrong mode to set security flags!");
        }
        else
        {
            concurrency::WriterLockToken token(*current_entity.get());

            if (mode == ENTITY_SECURITY)
            {
                dbtype::Security security =
                    current_entity.get()->get_entity_security(token);

                result = security.set_list_security_flag(flag, true) and
                    current_entity.get()->set_entity_security(
                        security,
                        token);
            }
            else if (mode == APPLICATION_SECURITY)
            {
                // Assumed already validated to be this type
                dbtype::PropertyEntity *property_entity_ptr =
                    dynamic_cast<dbtype::PropertyEntity *>(current_entity.get());
                dbtype::PropertySecurity security =
                    property_entity_ptr->get_application_security_settings(
                        current_application,
                        token).second;

                result = security.set_list_security_flag(flag, true) and
                         property_entity_ptr->set_application_security_settings(
                             current_application,
                             security,
                             token);
            }
            else
            {
                LOG(error, "dbdump", "add_security_flag_list", "Unknown mode!");
            }

            LOG(result ? debug : error, "dbdump", "add_security_flag_list",
                "Set security flag " + text::to_string(flag)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_security_flag_other(
        const dbtype::SecurityFlag flag)
    {
        bool result = false;

        if (mode == NORMAL)
        {
            LOG(error, "dbdump", "add_security_flag_other",
                "In the wrong mode to set security flags!");
        }
        else
        {
            concurrency::WriterLockToken token(*current_entity.get());

            if (mode == ENTITY_SECURITY)
            {
                dbtype::Security security =
                    current_entity.get()->get_entity_security(token);

                result = security.set_other_security_flag(flag, true) and
                         current_entity.get()->set_entity_security(
                             security,
                             token);
            }
            else if (mode == APPLICATION_SECURITY)
            {
                // Assumed already validated to be this type
                dbtype::PropertyEntity *property_entity_ptr =
                    dynamic_cast<dbtype::PropertyEntity *>(current_entity.get());
                dbtype::PropertySecurity security =
                    property_entity_ptr->get_application_security_settings(
                        current_application,
                        token).second;

                result = security.set_other_security_flag(flag, true) and
                         property_entity_ptr->set_application_security_settings(
                             current_application,
                             security,
                             token);
            }
            else
            {
                LOG(error, "dbdump", "add_security_flag_other", "Unknown mode!");
            }

            LOG(result ? debug : error, "dbdump", "add_security_flag_other",
                "Set security flag " + text::to_string(flag)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_to_security_group(const dbtype::Id &id)
    {
        bool result = false;

        if (mode == NORMAL)
        {
            LOG(error, "dbdump", "add_to_security_group",
                "In the wrong mode to set security flags!");
        }
        else
        {
            concurrency::WriterLockToken token(*current_entity.get());

            if (mode == ENTITY_SECURITY)
            {
                dbtype::Security security =
                    current_entity.get()->get_entity_security(token);

                result = security.add_to_list(id) and
                         current_entity.get()->set_entity_security(
                             security,
                             token);
            }
            else if (mode == APPLICATION_SECURITY)
            {
                // Assumed already validated to be this type
                dbtype::PropertyEntity *property_entity_ptr =
                    dynamic_cast<dbtype::PropertyEntity *>(current_entity.get());
                dbtype::PropertySecurity security =
                    property_entity_ptr->get_application_security_settings(
                        current_application,
                        token).second;

                result = security.add_to_list(id) and
                         property_entity_ptr->set_application_security_settings(
                             current_application,
                             security,
                             token);
            }
            else
            {
                LOG(error, "dbdump", "add_to_security_group", "Unknown mode!");
            }

            LOG(result ? debug : error, "dbdump", "add_to_security_group",
                "Add ID " + id.to_string(true)
                + " to group.  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::add_to_security_admins(const dbtype::Id &id)
    {
        bool result = false;

        if (mode == NORMAL)
        {
            LOG(error, "dbdump", "add_to_security_admins",
                "In the wrong mode to set security flags!");
        }
        else
        {
            concurrency::WriterLockToken token(*current_entity.get());

            if (mode == ENTITY_SECURITY)
            {
                dbtype::Security security =
                    current_entity.get()->get_entity_security(token);

                result = security.add_admin(id) and
                         current_entity.get()->set_entity_security(
                             security,
                             token);
            }
            else if (mode == APPLICATION_SECURITY)
            {
                // Assumed already validated to be this type
                dbtype::PropertyEntity *property_entity_ptr =
                    dynamic_cast<dbtype::PropertyEntity *>(current_entity.get());
                dbtype::PropertySecurity security =
                    property_entity_ptr->get_application_security_settings(
                        current_application,
                        token).second;

                result = security.add_admin(id) and
                         property_entity_ptr->set_application_security_settings(
                             current_application,
                             security,
                             token);
            }
            else
            {
                LOG(error, "dbdump", "add_to_security_admins", "Unknown mode!");
            }

            LOG(result ? debug : error, "dbdump", "add_to_security_admins",
                "Add ID " + id.to_string(true)
                + " to admin group.  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_string_prop(
        const dbtype::PropertyDirectory::PathString &path,
        const std::string &data)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_string_prop",
                "Tried to set string prop when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "set_string_prop",
                "Tried to set string property when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for setting the property.
            //
            dbtype::StringProperty property_data;

            if (not property_data.set(data))
            {
                result = false;
            }
            else
            {
                result = property_entity_ptr->set_property(path, property_data);
            }

            LOG(result ? debug : error, "dbdump", "set_string_prop",
                "Set " + path + " : " + property_data.get_as_short_string()
                + "  on Entity "
                + current_entity.id().to_string(true)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_int_prop(
        const dbtype::PropertyDirectory::PathString &path,
        const MG_SignedInt data)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_int_prop",
                "Tried to set int prop when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "set_int_prop",
                "Tried to set int property when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for setting the property.
            //
            const dbtype::IntegerProperty property_data(data);
            result = property_entity_ptr->set_property(path, property_data);

            LOG(result ? debug : error, "dbdump", "set_int_prop",
                "Set " + path + " : " + property_data.get_as_short_string()
                + "  on Entity "
                + current_entity.id().to_string(true)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_bool_prop(
        const dbtype::PropertyDirectory::PathString &path,
        const bool data)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_bool_prop",
                "Tried to set bool prop when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "set_bool_prop",
                "Tried to set bool property when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for setting the property.
            //
            const dbtype::BooleanProperty property_data(data);
            result = property_entity_ptr->set_property(path, property_data);

            LOG(result ? debug : error, "dbdump", "set_bool_prop",
                "Set " + path + " : " + property_data.get_as_short_string()
                + "  on Entity "
                + current_entity.id().to_string(true)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_prop(
        const dbtype::PropertyDirectory::PathString &path,
        const dbtype::PropertyDataType &type,
        const std::string &data)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_prop(set any from string)",
                "Tried to set prop from string when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "set_prop(set any from string)",
                "Tried to set prop from string when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for setting the property.
            //
            dbtype::PropertyData *property_data_ptr =
                create_property_data(type, data);

            if (property_data_ptr)
            {
                result = property_entity_ptr->set_property(
                    path,
                    *property_data_ptr);

                delete property_data_ptr;
                property_data_ptr = 0;
            }

            LOG(result ? debug : error, "dbdump", "set_prop(set any from string)",
                "Set " + path + " : " + dbtype::property_data_type_to_string(type)
                + " : "
                + property_data_ptr->get_as_short_string()
                + "  on Entity "
                + current_entity.id().to_string(true)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::PropertyData* DumpReaderInterface::create_property_data(
        const dbtype::PropertyDataType &type,
        const std::string &data) const
    {
        // Valid for setting the property.
        //
        dbtype::PropertyData *property_data_ptr = 0;

        switch (type)
        {
            case dbtype::PROPERTYDATATYPE_string:
            {
                property_data_ptr = new dbtype::StringProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_integer:
            {
                property_data_ptr = new dbtype::IntegerProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_float:
            {
                property_data_ptr = new dbtype::FloatProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_boolean:
            {
                property_data_ptr = new dbtype::BooleanProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_id:
            {
                property_data_ptr = new dbtype::IdProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_document:
            {
                property_data_ptr = new dbtype::DocumentProperty();
                break;
            }

            case dbtype::PROPERTYDATATYPE_set:
            {
                property_data_ptr = new dbtype::SetProperty();
                break;
            }

            default:
            {
                LOG(error, "dbdump", "create_property_data",
                    "Unknown type "
                    + dbtype::property_data_type_to_string(type));

                break;
            }
        }

        if (property_data_ptr)
        {
            if (not property_data_ptr->set_from_string(data))
            {
                LOG(error, "dbdump", "create_property_data",
                    "Could not set type "
                    + dbtype::property_data_type_to_string(type)
                    + " from string " + data);

                delete property_data_ptr;
                property_data_ptr = 0;
            }
        }

        return property_data_ptr;
    }

    // ----------------------------------------------------------------------
    bool DumpReaderInterface::set_prop(
        const dbtype::PropertyDirectory::PathString &path,
        const dbtype::PropertyData &data)
    {
        bool result = false;
        dbtype::PropertyEntity *property_entity_ptr = 0;

        if (not current_entity.valid())
        {
            LOG(error, "dbdump", "set_prop(PropertyData)",
                "Tried to set prop when no Entity has been selected!");
        }
        else if (not (property_entity_ptr = dynamic_cast<dbtype::PropertyEntity *>(
            current_entity.get())))
        {
            LOG(error, "dbdump", "set_prop(PropertyData)",
                "Tried to set property when Entity "
                + current_entity.id().to_string(true)
                + " does not support application properties!");
        }
        else
        {
            // Valid for setting the property.
            //
            result = property_entity_ptr->set_property(path, data);

            LOG(result ? debug : error, "dbdump", "set_prop(PropertyData)",
                "Set " + path + " : " + data.get_as_short_string()
                + "  on Entity "
                + current_entity.id().to_string(true)
                + "  Result: "
                + text::to_string(result));
        }

        return result;
    }
}
}
