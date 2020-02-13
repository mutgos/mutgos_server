/*
 * dbtype_EntityFields.cpp
 */

#include <string>

#include "dbtype_EntityField.h"

namespace
{
    const static std::string ENTITY_FIELD_AS_STRING[] =
    {
        "invalid",
        "invalid",
        "invalid",
        "type",
        "id",
        "name",
        "note",
        "security",
        "instance",
        "version",
        "created_timestamp",
        "updated_timestamp",
        "accessed_timestamp",
        "access_count",
        "owner",
        "flags",
        "references",
        "delete_batch_id",
        "deleted_flag",
        "invalid",
        "invalid",
        "group_ids",
        "group_disabled_ids",
        "invalid",
        "invalid",
        "application_properties",
        "invalid",
        "invalid",
        "contained_by",
        "linked_programs",
        "invalid",
        "invalid",
        "invalid",
        "invalid",
        "invalid",
        "invalid",
        "password",
        "player_display_name",
        "player_home",
        "player_last_connect",
        "invalid",
        "invalid",
        "thing_home",
        "thing_lock",
        "invalid",
        "invalid",
        "puppet_display_name",
        "invalid",
        "invalid",
        "vehicle_interior",
        "vehicle_controller",
        "invalid",
        "invalid",
        "program_runtime_sec",
        "program_source_code",
        "program_compiled_code",
        "program_language",
        "program_includes",
        "invalid",
        "invalid",
        "action_targets",
        "action_lock",
        "action_succ_msg",
        "action_succ_room_msg",
        "action_fail_msg",
        "action_fail_room_msg",
        "action_contained_by",
        "action_commands",
        "invalid",
        "invalid",
        "exit_arrive_msg",
        "exit_arrive_room_msg",
        "invalid",
        "invalid"
    };
}

namespace mutgos
{
namespace dbtype
{
    // -----------------------------------------------------------------------
    const std::string &entity_field_to_string(const EntityField field)
    {
        if ((field >= ENTITYFIELD_END) or (field <= ENTITYFIELD_invalid))
        {
            return ENTITY_FIELD_AS_STRING[0];
        }

        return ENTITY_FIELD_AS_STRING[field];
    }

    // -----------------------------------------------------------------------
    const EntityField string_to_entity_field(const std::string &str)
    {
        EntityField field = ENTITYFIELD_invalid;

        // Check each string for a match
        for (int index = 1; index < ENTITYFIELD_END; ++index)
        {
            if (ENTITY_FIELD_AS_STRING[index] == str)
            {
                field = (EntityField) index;
                break;
            }
        }

        return field;
    }
} /* namespace dbtype */
} /* namespace mutgos */
