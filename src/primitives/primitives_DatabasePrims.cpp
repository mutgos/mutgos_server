/*
 * primitives_DatabasePrims.cpp
 */

#include <string>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "concurrency/concurrency_WriterLockToken.h"

#include "primitives_DatabasePrims.h"
#include "primitives_Result.h"

#include "comminterface/comm_CommAccess.h"

#include "security/security_Context.h"
#include "security/security_SecurityException.h"
#include "security/security_SecurityAccess.h"

#include "concurrency/concurrency_ReaderLockToken.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_ActionEntity.h"
#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_PropertySecurity.h"
#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_StringProperty.h"
#include "dbtypes/dbtype_IntegerProperty.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Region.h"
#include "dbtypes/dbtype_Room.h"
#include "dbtypes/dbtype_Security.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

namespace
{
    const std::string ME_SEARCH = "me";
    const std::string HERE_SEARCH = "here";
    const std::string ID_PREFIX = "#";
    const std::string ID_SITE_SEPARATOR = "-";
    const std::string ID_PRINT_OPEN = "(";
    const std::string ID_PRINT_CLOSE = ")";
}

namespace mutgos
{
namespace primitives
{
    // ----------------------------------------------------------------------
    DatabasePrims::DatabasePrims(void)
    {
    }

    // ----------------------------------------------------------------------
    DatabasePrims::~DatabasePrims()
    {
    }

    // -----------------------------------------------------------------------
    Result DatabasePrims::is_entity_valid(
        security::Context &context,
        const dbtype::Id &entity_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result DatabasePrims::get_entity_type(
        security::Context &context,
        const dbtype::Id &entity_id,
        dbtype::EntityType &entity_type,
        const bool throw_on_violation)
    {
        Result result;
        entity_type = dbtype::ENTITYTYPE_invalid;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Have a valid Entity, now do the security check.
        //
        const bool security_success =
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_GET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_type,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            entity_type = entity_ref.type();
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result DatabasePrims::entity_to_string(
        security::Context &context,
        const dbtype::Id &entity_id,
        std::string &output,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to do a to_string()
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_ENTITY_TOSTRING,
            context,
            entity_ref,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // We are allowed to do the to_string().
            output += "-------------\n";
            output += entity_ref->to_string();
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result DatabasePrims::get_entity_location(
        security::Context &context,
        const dbtype::Id &entity_id,
        dbtype::Id &entity_location,
        const bool throw_on_violation)
    {
        Result result;

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else
        {
            // Determine the type of entity, and then look up who contains it.
            //
            dbtype::ActionEntity * const action_ptr =
                dynamic_cast<dbtype::ActionEntity *>(entity_ref.get());

            if (action_ptr)
            {
                // This is an action, which uses a slightly different field.
                //
                if (not security::SecurityAccess::instance()->security_check(
                    security::OPERATION_GET_ENTITY_FIELD,
                    context,
                    entity_ref,
                    dbtype::ENTITYFIELD_action_contained_by,
                    throw_on_violation))
                {
                    result.set_status(Result::STATUS_SECURITY_VIOLATION);
                }
                else
                {
                    entity_location = action_ptr->get_action_contained_by();
                }
            }
            else
            {
                dbtype::ContainerPropertyEntity * const container_ptr =
                    dynamic_cast<dbtype::ContainerPropertyEntity *>(
                        entity_ref.get());

                if (container_ptr)
                {
                    // This is a standard container.
                    //
                    if (not security::SecurityAccess::instance()->security_check(
                        security::OPERATION_GET_ENTITY_FIELD,
                        context,
                        entity_ref,
                        dbtype::ENTITYFIELD_contained_by,
                        throw_on_violation))
                    {
                        result.set_status(Result::STATUS_SECURITY_VIOLATION);
                    }
                    else
                    {
                        entity_location = container_ptr->get_contained_by();
                    }
                }
                else
                {
                    // This is not an Entity that can have a location.
                    result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
                }
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Result DatabasePrims::get_contents(
        security::Context &context,
        const dbtype::Id &container,
        const DatabasePrims::ContentsEntityTypes types,
        dbtype::Entity::IdVector &contents,
        const bool throw_on_violation)
    {
        Result result;
        bool security_success = false;
        dbinterface::EntityRef container_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(container);

        if (not container_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            // Return early to avoid elaborate/nested if checks
            return result;
        }

        switch (types)
        {
            case CONTENTS_NON_ACTIONS_ONLY:
            {
                security_success = security::SecurityAccess::instance()->
                    security_check(
                        security::OPERATION_GET_CONTAINS,
                        context,
                        container_ref,
                        throw_on_violation);
                break;
            }

            case CONTENTS_ACTIONS_ONLY:
            {
                security_success = security::SecurityAccess::instance()->
                    security_check(
                        security::OPERATION_GET_ACTIONS,
                        context,
                        container_ref,
                        throw_on_violation);
                break;
            }

            case CONTENTS_ALL:
            {
                security_success = security::SecurityAccess::instance()->
                    security_check(
                        security::OPERATION_GET_ACTIONS,
                        context,
                        container_ref,
                        throw_on_violation) and
                    security::SecurityAccess::instance()->
                        security_check(
                            security::OPERATION_GET_CONTAINS,
                            context,
                            container_ref,
                            throw_on_violation);
                break;
            }

            default:
            {
                LOG(error, "primitives", "get_contents",
                    "Unknown contents type specified.");

                result.set_status(Result::STATUS_BAD_ARGUMENTS);
                // Return early to avoid elaborate/nested if checks
                return result;

                break;
            }
        }

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            if (not dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    container_ref.get()))
            {
                // Not a container, so we can't get anything it contains
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                // Get the desired contents and return.
                //
                const bool get_all = (types == CONTENTS_ALL);
                concurrency::ReaderLockToken token(*container_ref.get());

                if (get_all or (types == CONTENTS_NON_ACTIONS_ONLY))
                {
                    container_ref->get_reference_ids_append(
                        dbtype::ENTITYFIELD_contained_by,
                        contents,
                        token);
                }

                if (get_all or (types == CONTENTS_ACTIONS_ONLY))
                {
                    container_ref->get_reference_ids_append(
                        dbtype::ENTITYFIELD_action_contained_by,
                        contents,
                        token);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // TODO This will likely need to be significantly expanded post-demo to support more types, action priorities, puppets, etc
    // TODO Will need to implement action 'priorities' so certain top level actions cannot be overridden
    Result DatabasePrims::match_name_to_id(
        security::Context &context,
        const std::string &search_string,
        const bool exact_match,
        const dbtype::EntityType entity_type,
        dbtype::Id &found_entity,
        bool &ambiguous,
        const bool throw_on_violation)
    {
        Result result;

        result.set_status(Result::STATUS_BAD_ARGUMENTS);

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();
        const std::string search_string_lower =
            text::to_lower_copy(text::trim_copy(search_string));

        found_entity = dbtype::Id();
        ambiguous = false;

        // Sanity checks
        //
        if (search_string_lower.empty() or
            (db_access->get_entity_type(context.get_requester()) ==
                dbtype::ENTITYTYPE_entity))
        {
            return result;
        }

        switch (entity_type)
        {
            case dbtype::ENTITYTYPE_entity:
            case dbtype::ENTITYTYPE_player:
            case dbtype::ENTITYTYPE_action:
            {
                // Valid
                break;
            }

            default:
            {
                // All other types invalid.
                return result;
                break;
            }
        }

        // Inputs look valid, do the search
        //
        if ((entity_type != dbtype::ENTITYTYPE_action) and
            (search_string_lower == ME_SEARCH))
        {
            // Looking for requester (basically, themself).
            //
            dbinterface::EntityRef entity_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(
                    context.get_requester());

            if (not entity_ref.valid())
            {
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }
            else
            {
                found_entity = context.get_requester();
                result.set_status(Result::STATUS_OK);
            }
        }
        else if ((entity_type == dbtype::ENTITYTYPE_entity) and
            (search_string_lower == HERE_SEARCH))
        {
            // Looking for requester's location.
            //
            dbinterface::EntityRef entity_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(
                    context.get_requester());

            if (not entity_ref.valid())
            {
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }
            else
            {
                dbtype::ContainerPropertyEntity * const cpe_ptr =
                    dynamic_cast<dbtype::ContainerPropertyEntity *>(
                        entity_ref.get());

                if (not cpe_ptr)
                {
                    result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
                }
                else
                {
                    found_entity = cpe_ptr->get_contained_by();
                    result.set_status(Result::STATUS_OK);
                }
            }
        }
        else if (entity_type == dbtype::ENTITYTYPE_player)
        {
            // Finding a particular player/puppet, not nessecarily in the room
            //
            match_character(
                context,
                search_string_lower,
                exact_match,
                result,
                found_entity,
                ambiguous,
                throw_on_violation);
        }
        else
        {
            // Do search along the environment
            //
            const ContentsEntityTypes types =
                ((entity_type == dbtype::ENTITYTYPE_action) ?
                    CONTENTS_ACTIONS_ONLY : CONTENTS_ALL);

            match_in_environment(
                context,
                search_string_lower,
                exact_match,
                types,
                result,
                found_entity,
                ambiguous);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Id DatabasePrims::convert_string_to_id(
        security::Context &context,
        const std::string &id_as_string)
    {
        dbtype::Id result;

        const std::string id_trimmed = text::trim_copy(id_as_string);
        const bool intermediate_spaces =
            (id_trimmed.find(' ') != std::string::npos);

        if ((not intermediate_spaces) and
            (id_trimmed.find(ID_PREFIX) == 0) and
            (id_trimmed.size() > ID_PREFIX.size()))
        {
            const size_t separator_index = id_trimmed.find(ID_SITE_SEPARATOR);
            std::string site_str;
            std::string entity_str;

            if (separator_index != std::string::npos)
            {
                // Has a site ID specified.
                //
                site_str = id_trimmed.substr(
                    ID_PREFIX.size(),
                    separator_index - ID_PREFIX.size());

                if (id_trimmed.size() > separator_index + 1)
                {
                    entity_str = id_trimmed.substr(separator_index + 1);
                }
            }
            else
            {
                // No site ID; use requester's site ID.
                //
                site_str =
                    text::to_string(context.get_requester().get_site_id());
                entity_str = id_trimmed.substr(ID_PREFIX.size());
            }

            // Have ID strings, now try and convert.
            //
            // TODO May need to be more rigorous in checking for all digits first
            dbtype::Id::SiteIdType site_id = 0;
            dbtype::Id::EntityIdType entity_id = 0;

            if ((not site_str.empty()) and (not entity_str.empty()) and
                (site_str.find('-') == std::string::npos) and
                (entity_str.find('-') == std::string::npos) and
                text::from_string(site_str, site_id) and
                text::from_string(entity_str, entity_id))
            {
                // Valid.
                result = dbtype::Id(site_id, entity_id);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // TODO Might need to make parser more robust
    Result DatabasePrims::convert_string_to_prop_params(
        security::Context &context,
        const std::string &input_string,
        dbtype::Id &entity_id,
        std::string &property,
        bool &has_property_value,
        std::string &property_value,
        bool &ambiguous,
        const bool throw_on_violation)
    {
        Result result;
        const std::string input = text::trim_copy(input_string);
        has_property_value = false;
        ambiguous = false;

        if (input.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        const size_t name_sep = input.find('=');

        if (name_sep == std::string::npos)
        {
            // Missing entity separator.
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Got the entity name as a string.  Search for it.
        //
        const std::string entity_name_str =
            text::trim_copy(input.substr(0, name_sep));

        // Try as ID first
        //
        entity_id = convert_string_to_id(context, entity_name_str);

        if (entity_id.is_default())
        {
            // Not an ID.  Search for it.
            //
            const Result search_result = match_name_to_id(
                context,
                entity_name_str,
                false,
                dbtype::ENTITYTYPE_entity,
                entity_id,
                ambiguous,
                throw_on_violation);

            if (not search_result.is_success())
            {
                // Failed to find entity.  Stop here.
                result = search_result;
                return result;
            }
        }

        if (not (input.size() > (name_sep + 1)))
        {
            // Missing property separator.
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        const size_t prop_sep = input.find(':', name_sep + 1);

        if (prop_sep == std::string::npos)
        {
            // Entire remainder is the property
            property = text::trim_copy(input.substr(name_sep + 1));
        }
        else
        {
            // Also has a value.
            //
            has_property_value = true;
            property = text::trim_copy(
                input.substr(name_sep + 1, prop_sep - name_sep + 1));

            if (input.size() > (prop_sep + 1))
            {
                // Value is not empty
                property_value = text::trim_copy(input.substr(prop_sep + 1));
            }
            else
            {
                // Value is empty
                property_value.clear();
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::convert_id_to_name(
        security::Context &context,
        const dbtype::Id &id,
        const bool append_id,
        std::string &name,
        const bool throw_on_violation)
    {
        Result result;

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            context,
            entity_ref,
            dbtype::ENTITYFIELD_name,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // Entity is valid and we are allowed to get the name.
            //
            name = entity_ref->get_entity_name();

            if (append_id)
            {
                // Add the ID at the end as requested.  Only show the site
                // if it's a different site from the requester's.
                //
                name += ID_PRINT_OPEN;
                name += id.to_string(
                    id.get_site_id() != context.get_requester().get_site_id());
                name += ID_PRINT_CLOSE;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::create_entity(
        security::Context &context,
        const dbtype::EntityType type,
        const std::string &name,
        dbtype::Id &created_entity_id,
        const bool throw_on_violation)
    {
        Result result;
        const std::string name_trimmed = text::trim_copy(name);
        created_entity_id = dbtype::Id();

        // Check inputs for validity.
        // Capabilities can only be created by MUTGOS itself.
        //
        if (name.empty() or (type == dbtype::ENTITYTYPE_capability))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check permission
        //
        if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_CREATE_ENTITY,
            context,
            type,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            dbinterface::EntityRef entity_ref;

            // Security says it is OK, so create it.  Check the return code.
            //
            const dbinterface::DbResultCode rc =
                dbinterface::DatabaseAccess::instance()->new_entity(
                    type,
                    context.get_requester().get_site_id(),
                    context.get_requester(),
                    name,
                    entity_ref);

            switch (rc)
            {
                case dbinterface::DBRESULTCODE_OK:
                {
                    // Good creation.  Set the ID and we're done.
                    //
                    if (entity_ref.valid())
                    {
                        created_entity_id = entity_ref.id();
                        set_new_entity_defaults(context, entity_ref);

                        LOG(info, "primitives", "create_entity",
                            "Requester "
                              + context.get_requester().to_string(true)
                              + " created Entity of type "
                              + dbtype::entity_type_to_string(type)
                              + ", ID " + created_entity_id.to_string(true));
                    }
                    else
                    {
                        result.set_status(Result::STATUS_BAD_ARGUMENTS);
                    }

                    break;
                }

                case dbinterface::DBRESULTCODE_BAD_ENTITY_TYPE:
                case dbinterface::DBRESULTCODE_BAD_NAME:
                {
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                    break;
                }

                default:
                {
                    LOG(error, "primitives", "create_entity",
                        "DB interface returned unexpected error: " +
                        dbinterface::db_result_code_to_string(rc));

                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                    break;
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // TODO This will require enhancements to send anything 'home' that should
    // not be deleted, if a room is deleted that contains stuff, etc.
    Result DatabasePrims::delete_entity(
        security::Context &context,
        const dbtype::Id &entity_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or
            context.get_requester().is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to delete it.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_DELETE_ENTITY,
            context,
            entity_ref,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            bool can_delete = true;

            // Check to make sure it is not something we do not allow to be
            // deleted, such as the initial Region (#1), 'system' user (#2),
            // Room (#3), and admin (#4).
            // user, along with any Capabilities (those are created by MUTGOS
            // and cannot be deleted).
            // Also, cannot delete a room you are currently in.
            //
            if (entity_id.get_entity_id() <= 4)
            {
                // These are special Entities that can never be deleted.
                can_delete = false;
            }
            else if (entity_ref.type() == dbtype::ENTITYTYPE_capability)
            {
                // Capabilities can never be deleted.
                can_delete = false;
            }
            else if ((entity_id == context.get_requester()) or
                (entity_id == context.get_program()))
            {
                // Can't delete yourself or the running program.
                can_delete = false;
            }
            else
            {
                dbinterface::EntityRef requester_ref =
                    dbinterface::DatabaseAccess::instance()->get_entity(
                        context.get_requester());

                if (not requester_ref.valid())
                {
                    LOG(error, "primitives", "delete_entity",
                        "Requester is not valid!");
                    can_delete = false;
                }
                else
                {
                    dbtype::ContainerPropertyEntity * const cpe_ptr =
                        dynamic_cast<dbtype::ContainerPropertyEntity *>(
                            requester_ref.get());

                    if (cpe_ptr and (cpe_ptr->get_contained_by() == entity_id))
                    {
                        // Trying to delete something we're inside.
                        // Currently not supported.
                        can_delete = false;
                    }
                }
            }


            if (not can_delete)
            {
                result.set_status(Result::STATUS_IMPOSSIBLE);
            }
            else
            {
                // Everything is OK; do the delete.
                //
                const dbinterface::DbResultCode rc =
                    dbinterface::DatabaseAccess::instance()->delete_entity(
                        entity_id);

                switch (rc)
                {
                    case dbinterface::DBRESULTCODE_OK:
                    case dbinterface::DBRESULTCODE_OK_DELAYED:
                    {
                        LOG(info, "primitives", "delete_entity",
                            "Requester "
                            + context.get_requester().to_string(true)
                            + " deleted Entity of type "
                            + dbtype::entity_type_to_string(entity_ref.type())
                            + ", ID " + entity_id.to_string(true));
                        break;
                    }

                    case dbinterface::DBRESULTCODE_BAD_ENTITY_ID:
                    {
                        // Maybe it was just deleted.  Not an error for now.
                        LOG(warning, "primitives", "delete_entity",
                            "Requester "
                            + context.get_requester().to_string(true)
                            + " tried to delete Entity of type "
                            + dbtype::entity_type_to_string(entity_ref.type())
                            + ", ID " + entity_id.to_string(true)
                            + ", but it did not exist.");
                        break;
                    }

                    default:
                    {
                        result.set_status(Result::STATUS_BAD_ARGUMENTS);
                        break;
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_entity_security(
        security::Context &context,
        const dbtype::Id &entity_id,
        dbtype::Security &entity_security,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        entity_security = dbtype::Security();

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            context,
            entity_ref,
            dbtype::ENTITYFIELD_security,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            entity_security = entity_ref->get_entity_security();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_entity_security_other(
        security::Context &context,
        const dbtype::Id &entity_id,
        const bool allow_read_write,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            context,
            entity_ref,
            dbtype::ENTITYFIELD_security,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_security,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // We can get AND set the security, so toggle the base.
            //
            concurrency::WriterLockToken token(*entity_ref.get());

            dbtype::Security entity_security =
                entity_ref->get_entity_security(token);

            entity_security.set_other_security_flag(
                dbtype::SECURITYFLAG_read,
                allow_read_write);
            entity_security.set_other_security_flag(
                dbtype::SECURITYFLAG_write,
                allow_read_write);

            if (not entity_ref->set_entity_security(entity_security, token))
            {
                LOG(error, "primitives", "set_entity_security_other",
                    "Requester "
                    + context.get_requester().to_string(true)
                    + " unable to set Security for Entity of type "
                    + dbtype::entity_type_to_string(entity_ref.type())
                    + ", ID " + entity_id.to_string(true));
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_entity_security_add_admin(
        security::Context &context,
        const dbtype::Id &entity_id,
        const dbtype::Id &admin_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or admin_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);
        // We get the admin to make sure it exists and to prevent deletion.
        dbinterface::EntityRef admin_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(admin_id);

        if ((not entity_ref.valid()) or (not admin_ref.valid()))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            context,
            entity_ref,
            dbtype::ENTITYFIELD_security,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_security,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // We can get AND set the security, so add the admin.
            //
            concurrency::WriterLockToken token(*entity_ref.get());
            dbtype::Security entity_security =
                entity_ref->get_entity_security(token);

            if (entity_security.add_admin(admin_id))
            {
                if (not entity_ref->set_entity_security(entity_security, token))
                {
                    LOG(error, "primitives", "set_entity_security_add_admin",
                        "Requester "
                        + context.get_requester().to_string(true)
                        + " unable to set Security for Entity of type "
                        + dbtype::entity_type_to_string(entity_ref.type())
                        + ", ID " + entity_id.to_string(true));
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_entity_security_remove_admin(
        security::Context &context,
        const dbtype::Id &entity_id,
        const dbtype::Id &admin_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or admin_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);
        // We get the admin to make sure it exists and to prevent deletion.
        dbinterface::EntityRef admin_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(admin_id);

        if ((not entity_ref.valid()) or (not admin_ref.valid()))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_ENTITY_FIELD,
            context,
            entity_ref,
            dbtype::ENTITYFIELD_security,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_security,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // We can get AND set the security, so remove the admin.
            //
            concurrency::WriterLockToken token(*entity_ref.get());
            dbtype::Security entity_security =
                entity_ref->get_entity_security(token);

            if (entity_security.remove_admin(admin_id))
            {
                if (not entity_ref->set_entity_security(entity_security, token))
                {
                    LOG(error, "primitives", "set_entity_security_remove_admin",
                        "Requester "
                        + context.get_requester().to_string(true)
                        + " unable to set Security for Entity of type "
                        + dbtype::entity_type_to_string(entity_ref.type())
                        + ", ID " + entity_id.to_string(true));
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_application_security(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &application,
        dbtype::PropertySecurity &application_security,
        dbtype::Id &owner,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or application.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        application_security = dbtype::PropertySecurity();
        owner = dbtype::Id();

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_APPLICATION_SECURITY,
            context,
            entity_ref,
            application,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            dbtype::PropertyEntity * const pe_ptr =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not pe_ptr)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                dbtype::PropertyEntity::ApplicationOwnerSecurity
                    security = pe_ptr->get_application_security_settings(
                        application);

                if (security.first.is_default())
                {
                    // Application does not exist.
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
                else
                {
                    owner = security.first;
                    application_security = security.second;
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_security_other(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &application,
        const bool allow_write,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or application.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_APPLICATION_SECURITY,
            context,
            entity_ref,
            application,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_APPLICATION_SECURITY,
                context,
                entity_ref,
                application,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            dbtype::PropertyEntity * const pe_ptr =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not pe_ptr)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                concurrency::WriterLockToken token(*entity_ref.get());
                dbtype::PropertyEntity::ApplicationOwnerSecurity
                    security = pe_ptr->get_application_security_settings(
                    application,
                    token);

                if (security.first.is_default())
                {
                    // Application does not exist.
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
                else
                {
                    // Update security and set.
                    security.second.set_other_security_flag(
                        dbtype::SECURITYFLAG_write,
                        allow_write);

                    if (not pe_ptr->set_application_security_settings(
                        application,
                        security.second,
                        token))
                    {
                        LOG(error, "primitives",
                            "set_application_security_other",
                            "Requester "
                            + context.get_requester().to_string(true)
                            + " unable to set Security for application of type "
                            + dbtype::entity_type_to_string(entity_ref.type())
                            + ", ID " + entity_id.to_string(true)
                            + ", application " + application);
                        result.set_status(Result::STATUS_BAD_ARGUMENTS);
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_security_add_admin(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &application,
        const dbtype::Id &admin_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or application.empty() or
            admin_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);
        // We get the admin to make sure it exists and to prevent deletion.
        dbinterface::EntityRef admin_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(admin_id);

        if ((not entity_ref.valid()) or (not admin_ref.valid()))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_APPLICATION_SECURITY,
            context,
            entity_ref,
            application,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_APPLICATION_SECURITY,
                context,
                entity_ref,
                application,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            dbtype::PropertyEntity * const pe_ptr =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not pe_ptr)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                concurrency::WriterLockToken token(*entity_ref.get());
                dbtype::PropertyEntity::ApplicationOwnerSecurity
                    security = pe_ptr->get_application_security_settings(
                    application,
                    token);

                if (security.first.is_default())
                {
                    // Application does not exist.
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
                else
                {
                    // Update security and set.
                    //
                    if (security.second.add_admin(admin_id))
                    {
                        if (not pe_ptr->set_application_security_settings(
                            application,
                            security.second,
                            token))
                        {
                            LOG(error, "primitives",
                                "set_application_security_add_admin",
                                "Requester "
                                + context.get_requester().to_string(true)
                                + " unable to set Security for application of "
                                  "type "
                                + dbtype::entity_type_to_string(
                                    entity_ref.type())
                                + ", ID " + entity_id.to_string(true)
                                + ", application " + application);
                            result.set_status(Result::STATUS_BAD_ARGUMENTS);
                        }
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_security_remove_admin(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &application,
        const dbtype::Id &admin_id,
        const bool throw_on_violation)
    {
        Result result;

        if (entity_id.is_default() or application.empty() or
            admin_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Check security to see if we might be able to access the security
        // field.
        //
        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);
        // We get the admin to make sure it exists and to prevent deletion.
        dbinterface::EntityRef admin_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(admin_id);

        if ((not entity_ref.valid()) or (not admin_ref.valid()))
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else if (not (security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_APPLICATION_SECURITY,
            context,
            entity_ref,
            application,
            throw_on_violation) and
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_SET_APPLICATION_SECURITY,
                context,
                entity_ref,
                application,
                throw_on_violation)))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            dbtype::PropertyEntity * const pe_ptr =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not pe_ptr)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                concurrency::WriterLockToken token(*entity_ref.get());
                dbtype::PropertyEntity::ApplicationOwnerSecurity
                    security = pe_ptr->get_application_security_settings(
                    application,
                    token);

                if (security.first.is_default())
                {
                    // Application does not exist.
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
                else
                {
                    // Update security and set.
                    //
                    if (security.second.remove_admin(admin_id))
                    {
                        if (not pe_ptr->set_application_security_settings(
                            application,
                            security.second,
                            token))
                        {
                            LOG(error, "primitives",
                                "set_application_security_remove_admin",
                                "Requester "
                                + context.get_requester().to_string(true)
                                + " unable to set Security for application of "
                                  "type "
                                + dbtype::entity_type_to_string(
                                    entity_ref.type())
                                + ", ID " + entity_id.to_string(true)
                                + ", application " + application);
                            result.set_status(Result::STATUS_BAD_ARGUMENTS);
                        }
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::can_use_action(
        security::Context &context,
        const dbtype::Id &action_id,
        const bool throw_on_violation)
    {
        Result result;

        if (action_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef action_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(action_id);

        if (not action_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
        else
        {
            dbtype::ActionEntity * const action_ptr =
                dynamic_cast<dbtype::ActionEntity *>(action_ref.get());

            if (not action_ptr)
            {
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            }
            else
            {
                const bool security_success =
                    security::SecurityAccess::instance()->
                        security_check(
                            security::OPERATION_USE_ACTION,
                            context,
                            action_ref,
                            throw_on_violation);

                if (not security_success)
                {
                    result.set_status(Result::STATUS_SECURITY_VIOLATION);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_application_property_type(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        dbtype::PropertyDataType &type,
        const bool throw_on_violation)
    {
        Result result;
        type = dbtype::PROPERTYDATATYPE_invalid;

        // Basic error checking, and retrieve the entity.
        //
        if (entity_id.is_default() or property_path.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Have a valid Entity, now do the security check.
        //
        const bool security_success =
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_GET_APPLICATION_PROPERTY,
                context,
                entity_ref,
                property_path,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // See if this is a type that can support properties.
            //
            dbtype::PropertyEntity * const property_entity =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not property_entity)
            {
                // Properties not supported
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
                return result;
            }

            // Now try and get the property type.
            //
            type = property_entity->get_property_type(property_path);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        std::string &property_value,
        const bool convert_nonstring,
        const bool throw_on_violation)
    {
        dbtype::PropertyData *property_ptr = 0;
        Result result = get_property_raw(
            context,
            entity_id,
            property_path,
            property_ptr,
            throw_on_violation);

        if (property_ptr)
        {
            // Was able to get a property.  Convert as needed and return.
            //
            dbtype::StringProperty * const string_property =
                dynamic_cast<dbtype::StringProperty *>(property_ptr);

            if (string_property)
            {
                property_value = string_property->get();
            }
            else if (convert_nonstring)
            {
                // Not a string, but we can convert it.
                property_value = property_ptr->get_as_string();
            }
            else
            {
                // Not a string and no conversion desired.  Error out.
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }

            delete property_ptr;
            property_ptr = 0;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        MG_SignedInt &property_value,
        const bool throw_on_violation)
    {
        dbtype::PropertyData *property_ptr = 0;
        Result result = get_property_raw(
            context,
            entity_id,
            property_path,
            property_ptr,
            throw_on_violation);

        if (property_ptr)
        {
            // Was able to get a property.  Convert as needed and return.
            //
            dbtype::IntegerProperty * const int_property =
                dynamic_cast<dbtype::IntegerProperty *>(property_ptr);

            if (int_property)
            {
                property_value = int_property->get();
            }
            else
            {
                // Not an int.  Error out.
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }

            delete property_ptr;
            property_ptr = 0;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        DocumentContents &property_value,
        const bool throw_on_violation)
    {
        dbtype::PropertyData *property_ptr = 0;
        Result result = get_property_raw(
            context,
            entity_id,
            property_path,
            property_ptr,
            throw_on_violation);

        if (property_ptr)
        {
            // Was able to get a property.  Convert as needed and return.
            //
            dbtype::DocumentProperty * const document_property =
                dynamic_cast<dbtype::DocumentProperty *>(property_ptr);

            if (document_property)
            {
                property_value.clear();
                const dbtype::DocumentProperty::DocumentData &data_raw =
                    document_property->get();

                for (dbtype::DocumentProperty::DocumentData::const_iterator
                        line_iter = data_raw.begin();
                    line_iter != data_raw.end();
                    ++line_iter)
                {
                    if (*line_iter)
                    {
                        property_value.push_back(**line_iter);
                    }
                }
            }
            else
            {
                // Not an int.  Error out.
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }

            delete property_ptr;
            property_ptr = 0;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        const MG_SignedInt property_value,
        const bool throw_on_violation)
    {
        const dbtype::IntegerProperty property_obj =
            dbtype::IntegerProperty(property_value);
        Result result = set_property_raw(
            context,
            entity_id,
            property_path,
            &property_obj,
            throw_on_violation);

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        const std::string &property_value,
        const bool throw_on_violation)
    {
        const dbtype::StringProperty property_obj =
            dbtype::StringProperty(property_value);
        Result result = set_property_raw(
            context,
            entity_id,
            property_path,
            &property_obj,
            throw_on_violation);

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_application_property(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        const dbtype::DocumentProperty &property_value,
        const bool throw_on_violation)
    {
        Result result = set_property_raw(
            context,
            entity_id,
            property_path,
            &property_value,
            throw_on_violation);

        return result;
    }

    // ----------------------------------------------------------------------
    // TODO This does not support puppets yet.
    void DatabasePrims::match_character(
        security::Context &context,
        const std::string &search_string_lower,
        const bool exact_match,
        Result &result,
        dbtype::Id &found_entity,
        bool &ambiguous,
        const bool throw_on_violation)
    {
        result.set_status(Result::STATUS_OK);

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();

        if (not security::SecurityAccess::instance()->security_check(
            security::OPERATION_FIND_CHARACTER_BY_NAME,
            context,
            throw_on_violation))
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else if (exact_match)
        {
            const dbtype::Entity::IdVector search_results =
                dbinterface::DatabaseAccess::instance()->find(
                    context.get_requester().get_site_id(),
                    dbtype::ENTITYTYPE_player,
                    search_string_lower,
                    true);

            if (search_results.size() == 1)
            {
                // Found exact match
                found_entity = search_results.front();
            }
        }
        else
        {
            // Not exact match, so check online players first, and if no
            // unambiguous match, save result.  Then, try search with exact
            // match and return only if a single result or exact (prefer over
            // partial online check)
            //
            const dbtype::Entity::IdVector online_ids =
                comm::CommAccess::instance()->get_online_ids(
                    context.get_requester().get_site_id());
            dbtype::Id matched_online_id;
            bool matched_id_exact  = false;
            bool online_ambiguous = false;
            dbinterface::EntityRef entity_ref;

            for (dbtype::Entity::IdVector::const_iterator online_iter =
                online_ids.begin();
                 online_iter != online_ids.end();
                 ++online_iter)
            {
                bool found_online_exact = false;
                entity_ref = db_access->get_entity(*online_iter);

                if (entity_ref.valid())
                {
                    if (match_name(
                        entity_ref->get_entity_name(),
                        search_string_lower,
                        false,
                        found_online_exact))
                    {
                        // Matched.  See if a better match or ambiguous
                        //
                        if (matched_online_id.is_default())
                        {
                            // First match.
                            // If exact, we can stop immediately since there
                            // cannot be duplicates.
                            //
                            matched_online_id = entity_ref.id();
                            matched_id_exact = found_online_exact;
                        }
                        else
                        {
                            // Found another match.
                            // Players cannot have duplicate names, making
                            // this easier.  Either it is a better match
                            // (more exact) than what we have, or it is no
                            // better.  A match that is no better is therefore
                            // ambiguous.
                            //
                            if (found_online_exact)
                            {
                                // Found exact.  We can stop here.
                                //
                                matched_online_id = entity_ref.id();
                                matched_id_exact = true;
                            }
                            else
                            {
                                // Found another partial.  We can stop here
                                // since it is ambiguous.
                                online_ambiguous = true;
                                matched_online_id = dbtype::Id();
                            }
                        }

                        if (matched_id_exact or online_ambiguous)
                        {
                            // Either we found exactly what we wanted, or we
                            // found too many.  Either way, we're done.
                            break;
                        }
                    }
                }
            }

            entity_ref.clear();

            // Examine result and determine if we need to try exact database
            // match.
            //
            if (online_ambiguous or matched_online_id.is_default())
            {
                // Ambiguous or couldn't find. Try exact.
                //
                const dbtype::Entity::IdVector search_results =
                    dbinterface::DatabaseAccess::instance()->find(
                        context.get_requester().get_site_id(),
                        dbtype::ENTITYTYPE_player,
                        search_string_lower,
                        true);

                if (search_results.size() == 1)
                {
                    // Found exact match
                    found_entity = search_results.front();
                }
            }
            else
            {
                // Matched good enough online, so use that result.
                found_entity = matched_online_id;
            }
        }

        if (result.is_success() and found_entity.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
    }

    // ----------------------------------------------------------------------
    void DatabasePrims::match_in_environment(
        security::Context &context,
        const std::string &search_string_lower,
        const bool exact_match,
        const DatabasePrims::ContentsEntityTypes entity_types,
        Result &result,
        dbtype::Id &found_entity,
        bool &ambiguous)
    {
        found_entity = dbtype::Id();
        ambiguous = false;

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();
        Result current_result;
        dbtype::Id current_found_entity;
        bool current_exact_match = false;
        bool current_ambiguous = false;
        dbinterface::EntityRef entity_ref;
        dbtype::ContainerPropertyEntity *cpe_ptr = 0;
        dbtype::Entity::IdVector current_contents;
        dbtype::Entity::IdVector current_effective_contents;

        result.set_status(Result::STATUS_OK);

        // First, check requester's inventory.
        //
        current_result = get_contents(
            context,
            context.get_requester(),
            CONTENTS_ALL,
            current_contents,
            false);

        if (current_result.is_success())
        {
            filter_enhance_contents(
                context,
                current_contents,
                entity_types,
                current_effective_contents);
        }

        if (current_result.is_success() and match_name_in_contents(
                context,
                current_effective_contents,
                search_string_lower,
                exact_match,
                current_found_entity,
                current_exact_match,
                current_ambiguous))
        {
            // See if we found a good match.
            //
            if (current_ambiguous)
            {
                // Set ambiguous now since it is, but this could be set
                // to false later if we find an exact match.
                ambiguous = true;
            }
            else
            {
                // Found a potential match.  If it's exact, we won't search
                // any further after this.
                //
                found_entity = current_found_entity;
                ambiguous = false;
            }
        }

        current_contents.clear();
        current_effective_contents.clear();

        if (not current_exact_match)
        {
            // Not an exact match; check the room for an exact match.
            //
            entity_ref = db_access->get_entity(context.get_requester());
            cpe_ptr = dynamic_cast<dbtype::ContainerPropertyEntity *>(
                entity_ref.get());

            if (cpe_ptr)
            {
                entity_ref = db_access->get_entity(cpe_ptr->get_contained_by());
                cpe_ptr = 0;

                if (entity_ref.valid())
                {
                    cpe_ptr = dynamic_cast<dbtype::ContainerPropertyEntity *>(
                        entity_ref.get());
                }
            }

            if (cpe_ptr)
            {
                current_result = get_contents(
                    context,
                    cpe_ptr->get_entity_id(),
                    CONTENTS_ALL,
                    current_contents,
                    false);

                if (current_result.is_success())
                {
                    filter_enhance_contents(
                        context,
                        current_contents,
                        entity_types,
                        current_effective_contents);

                    if (match_name_in_contents(
                        context,
                        current_effective_contents,
                        search_string_lower,
                        exact_match,
                        current_found_entity,
                        current_exact_match,
                        current_ambiguous))
                    {
                        if (current_ambiguous)
                        {
                            // Set ambiguous now since it is, but this could be
                            // set to false later if we find an exact match.
                            ambiguous = true;
                        }
                        else if (found_entity.is_default() or
                            current_exact_match)
                        {
                            // Found a potential match.  If it's exact, we
                            // won't search any further after this.
                            //
                            found_entity = current_found_entity;
                            ambiguous = false;
                        }
                        else
                        {
                            // Set ambiguous now since we already had a
                            // partial match from requester's inventory and
                            // it found another partial match.  This could be
                            // set to false later if we find an exact match.
                            ambiguous = true;
                        }
                    }
                }
            }
        }

        current_contents.clear();
        current_effective_contents.clear();

        if ((not current_exact_match) and (entity_ref.valid() and cpe_ptr))
        {
            // Still no match.  Starting at the Region above the Room,
            // check for any actions that match.  Keep going to the root until
            // a match is found or no more Regions left.
            //
            // entity_ref and cpe_ptr are currently set to the room; we can
            // use that to start our checks.
            //
            entity_ref = db_access->get_entity(cpe_ptr->get_contained_by());
            cpe_ptr = 0;
            dbtype::Region *region_ptr = 0;

            if (entity_ref.valid())
            {
                region_ptr = dynamic_cast<dbtype::Region *>(entity_ref.get());
            }

            while (region_ptr and (not current_exact_match))
            {
                current_contents.clear();
                current_result = get_contents(
                    context,
                    region_ptr->get_entity_id(),
                    CONTENTS_ACTIONS_ONLY,
                    current_contents,
                    false);

                if (current_result.is_success())
                {
                    if (match_name_in_contents(
                        context,
                        current_contents,
                        search_string_lower,
                        exact_match,
                        current_found_entity,
                        current_exact_match,
                        current_ambiguous))
                    {
                        if (current_ambiguous)
                        {
                            // Set ambiguous now since it is, but this could be
                            // set to false later if we find an exact match.
                            ambiguous = true;
                        }
                        else if (found_entity.is_default() or
                                 current_exact_match)
                        {
                            // Found a potential match.  If it's exact, we
                            // won't search any further after this.
                            //
                            found_entity = current_found_entity;
                            ambiguous = false;
                        }
                        else
                        {
                            // Set ambiguous now since we already had a
                            // partial match from requester's inventory and
                            // it found another partial match.  This could be
                            // set to false later if we find an exact match.
                            ambiguous = true;
                        }
                    }
                }

                // This is important to note:  Security is bypassed here, to
                // make sure the parent region can always be retrieved.
                // Otherwise someone could accidentally block getting the
                // region above, effectively turning off all important
                // commands with no recovery.
                //
                if (not current_exact_match)
                {
                    entity_ref = db_access->get_entity(
                        region_ptr->get_contained_by());
                    region_ptr = 0;

                    if (entity_ref.valid())
                    {
                        region_ptr = dynamic_cast<dbtype::Region *>(
                            entity_ref.get());
                    }
                }
            }

            region_ptr = 0;
        }

        if (ambiguous)
        {
            found_entity = dbtype::Id();
        }

        if (found_entity.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
        }
    }

    // ----------------------------------------------------------------------
    void DatabasePrims::filter_enhance_contents(
        security::Context &context,
        const dbtype::Entity::IdVector &contents,
        const ContentsEntityTypes entity_types,
        dbtype::Entity::IdVector &effective_contents)
    {
        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();
        const bool want_actions = (entity_types == CONTENTS_ACTIONS_ONLY) or
            (entity_types == CONTENTS_ALL);
        const bool want_non_actions = (entity_types == CONTENTS_NON_ACTIONS_ONLY) or
            (entity_types == CONTENTS_ALL);
        dbinterface::EntityRef entity_ref;

        effective_contents.reserve(contents.size());

        for (dbtype::Entity::IdVector::const_iterator id_iter =
                contents.begin();
            id_iter != contents.end();
            ++id_iter)
        {
            entity_ref = db_access->get_entity(*id_iter);

            if (entity_ref.valid())
            {
                if (want_actions and
                    dynamic_cast<dbtype::ActionEntity *>(entity_ref.get()))
                {
                    // This is an action.  We can just add it as-is.
                    effective_contents.push_back(*id_iter);
                }
                else if (dynamic_cast<dbtype::ContainerPropertyEntity *>(
                    entity_ref.get()))
                {
                    // Found a container.  Add container itself if not
                    // actions only, then add all actions contained in it
                    // if pass security.
                    //
                    if (want_non_actions)
                    {
                        effective_contents.push_back(*id_iter);
                    }

                    if (want_actions)
                    {
                        get_contents(
                            context,
                            *id_iter,
                            CONTENTS_ACTIONS_ONLY,
                            effective_contents,
                            false);
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    bool DatabasePrims::match_name_in_contents(
        security::Context &context,
        const dbtype::Entity::IdVector &contents,
        const std::string &search_string,
        const bool exact_match,
        dbtype::Id &found_entity,
        bool &found_exact_match,
        bool &ambiguous)
    {
        bool matched_something = false;

        found_exact_match = false;
        ambiguous = false;

        dbinterface::DatabaseAccess * const db_access =
            dbinterface::DatabaseAccess::instance();

        for (dbtype::Entity::IdVector::const_iterator entity_iter =
            contents.begin();
             entity_iter != contents.end();
             ++entity_iter)
        {
            dbinterface::EntityRef entity_ref =
                db_access->get_entity(*entity_iter);

            if (entity_ref.valid())
            {
                // TODO Disabled to prevent deadlock
                // concurrency::ReaderLockToken token(*entity_ref.get());

                // Determine type of Entity.  That will determine how we check
                // for a match.
                //
                dbtype::ActionEntity * const action_ptr =
                    dynamic_cast<dbtype::ActionEntity *>(entity_ref.get());

                if (action_ptr)
                {
                    // An action.  Do exact match of commands, and partial
                    // match of name itself (if parameter indicates).
                    //
                    if (security::SecurityAccess::instance()->security_check(
                            security::OPERATION_GET_ENTITY_FIELD,
                            context,
                            entity_ref,
                            dbtype::ENTITYFIELD_action_commands,
                            false) and
                        action_ptr->has_action_command(search_string))
                    {
                        // Exact match for alias.
                        //
                        if (found_exact_match)
                        {
                            // Already had an exact match; ambiguous.  Exit now.
                            //
                            ambiguous = true;
                            found_entity = dbtype::Id();
                            break;
                        }
                        else
                        {
                            // Found a better, exact match.
                            //
                            found_entity = *entity_iter;
                            found_exact_match = true;
                            ambiguous = false;
                            matched_something = true;
                        }
                    }
                }

                // Now check actual name, whether or not this is an action.
                //
                bool temp_found_exact = false;

                if (security::SecurityAccess::instance()->security_check(
                        security::OPERATION_GET_ENTITY_FIELD,
                        context,
                        entity_ref,
                        dbtype::ENTITYFIELD_name,
                        false) and
                    match_name(
                        entity_ref->get_entity_name(),
                        search_string,
                        exact_match,
                        temp_found_exact))
                {
                    if (matched_something)
                    {
                        // Determine if this is a better match or
                        // ambiguous
                        //
                        if (found_exact_match and (not temp_found_exact))
                        {
                            // This match can be discarded, since we
                            // already have a better one.
                        }
                        else if ((not found_exact_match) and temp_found_exact)
                        {
                            // Found a better (exact) match.
                            //
                            found_exact_match = true;
                            found_entity = *entity_iter;
                        }
                        else if ((found_exact_match and temp_found_exact) and
                            (found_entity == *entity_iter))
                        {
                            // Special situation where action name is the same
                            // as one of the aliases, of which both are an
                            // exact match.
                            // For now we don't have to do anything, just
                            // let it get past this special circumstance.
                        }
                        else
                        {
                            // Another partial or exact match.
                            // Either way, it is ambiguous.  Exit now.
                            //
                            ambiguous = true;
                            found_entity = dbtype::Id();
                            break;
                        }
                    }
                    else
                    {
                        // No other matches right now except for this one.
                        //
                        matched_something = true;
                        found_exact_match = temp_found_exact;
                        found_entity = *entity_iter;
                    }
                }
            }
        }

        if (not matched_something)
        {
            found_entity = dbtype::Id();
        }

        return matched_something;
    }

    // ----------------------------------------------------------------------
    bool DatabasePrims::match_name(
        const std::string &name,
        const std::string &search_string,
        const bool exact_match,
        bool &found_exact_match)
    {
        bool found = false;
        found_exact_match = false;
        const std::string name_lower = text::to_lower_copy(name);
        const size_t search_result = name_lower.find(search_string);

        if (search_result != std::string::npos)
        {
            found_exact_match = (name.size() == search_string.size());

            if (exact_match)
            {
                found = found_exact_match;
            }
            else
            {
                found = true;
            }
        }

        return found;
    }

    // ----------------------------------------------------------------------
    // TODO Need significant enhancement
    void DatabasePrims::set_new_entity_defaults(
        security::Context &context,
        dbinterface::EntityRef &entity_ref)
    {
        if (not entity_ref.valid())
        {
            LOG(error, "primitives", "set_new_entity_defaults",
                "Passed an invalid entity ref!");
            return;
        }

        dbtype::ContainerPropertyEntity * const cpe_ptr =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(entity_ref.get());
        dbtype::Room * const room_ptr =
            dynamic_cast<dbtype::Room *>(entity_ref.get());
        dbtype::ActionEntity * const action_ptr =
            dynamic_cast<dbtype::ActionEntity *>(entity_ref.get());
        dbtype::Player * const player_ptr =
            dynamic_cast<dbtype::Player *>(entity_ref.get());

        // Set basic security.
        //
        dbtype::Security security;
        security.set_other_security_flag(dbtype::SECURITYFLAG_basic, true);
        entity_ref->set_entity_security(security);

        if (room_ptr)
        {
            // Set contains by to first/default Region, #1
            //
            room_ptr->set_contained_by(
                dbtype::Id(context.get_requester().get_site_id(), 1));
        }
        else if (action_ptr)
        {
            // Set contains on owner
            //
            action_ptr->set_action_contained_by(context.get_requester());
        }
        else if (player_ptr)
        {
            // Set to first/default room, #3
            //
            player_ptr->set_contained_by(
                dbtype::Id(context.get_requester().get_site_id(), 3));

            // Set default password
            //
            player_ptr->set_password("1234");
        }
        else if (cpe_ptr)
        {
            // Generic container, put in owner's inventory.
            //
            cpe_ptr->set_contained_by(context.get_requester());
        }
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::get_property_raw(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        dbtype::PropertyData *&property_value_ptr,
        const bool throw_on_violation)
    {
        Result result;

        property_value_ptr = 0;

        // Basic error checking, and retrieve the entity.
        //
        if (entity_id.is_default() or property_path.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Have a valid Entity, now do the security check.
        //
        const bool security_success =
            security::SecurityAccess::instance()->security_check(
                security::OPERATION_GET_APPLICATION_PROPERTY,
                context,
                entity_ref,
                property_path,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // See if this is a type that can support properties.
            //
            dbtype::PropertyEntity * const property_entity =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not property_entity)
            {
                // Properties not supported
                result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
                return result;
            }

            // Now try and get the property.
            //
            property_value_ptr = property_entity->get_property(property_path);

            if (not property_value_ptr)
            {
                result.set_status(Result::STATUS_BAD_ARGUMENTS);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result DatabasePrims::set_property_raw(
        security::Context &context,
        const dbtype::Id &entity_id,
        const std::string &property_path,
        const dbtype::PropertyData * const property_value_ptr,
        const bool throw_on_violation)
    {
        Result result;

        // Basic error checking, and retrieve the entity.
        //
        if ((not property_value_ptr) or entity_id.is_default() or
            property_path.empty())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbtype::PropertyEntity * const property_entity =
            dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

        if (not property_entity)
        {
            // Properties not supported
            result.set_status(Result::STATUS_BAD_ENTITY_TYPE);
            return result;
        }

        concurrency::WriterLockToken token(*property_entity);
        bool security_success = true;

        // If application does NOT exist, do check on set field, then add
        // application and security defaults.
        // TODO Need to figure out how to determine owner of application, post-demo.
        //
        if (not property_entity->application_exists(property_path, token))
        {
            security_success =
                security::SecurityAccess::instance()->security_check(
                    security::OPERATION_SET_ENTITY_FIELD,
                    context,
                    entity_ref,
                    dbtype::ENTITYFIELD_application_properties,
                    throw_on_violation);

            if (not security_success)
            {
                result.set_status(Result::STATUS_SECURITY_VIOLATION);
            }
            else
            {
                // Add application and set a security default of world
                // readable.
                //
                dbtype::PropertySecurity security;

                security.set_other_security_flag(
                    dbtype::SECURITYFLAG_read,
                    true);

                if (not property_entity->add_application(
                    property_path,
                    property_entity->get_entity_owner(token),
                    security,
                    token))
                {
                    security_success = false;
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
            }
        }

        // Have a valid Entity and the application exists, now do the security
        // check and set the property.
        //
        if (security_success)
        {
            security_success =
                security::SecurityAccess::instance()->security_check(
                    security::OPERATION_SET_APPLICATION_PROPERTY,
                    context,
                    entity_ref,
                    property_path,
                    throw_on_violation);

            if (not security_success)
            {
                result.set_status(Result::STATUS_SECURITY_VIOLATION);
            }
            else
            {
                if (not property_entity->set_property(
                    property_path,
                    *property_value_ptr,
                    token))
                {
                    result.set_status(Result::STATUS_BAD_ARGUMENTS);
                }
            }
        }

        return result;
    }
}
}
