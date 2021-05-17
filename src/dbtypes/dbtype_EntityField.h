/*
 * dbtype_EntityFields.h
 */

#ifndef MUTGOS_DBTYPE_ENTITYFIELDS_H_
#define MUTGOS_DBTYPE_ENTITYFIELDS_H_

#include <string>

namespace mutgos
{
namespace dbtype
{
    /**
     * A list of all fields available on an Entity and ALL subclasses.
     * This is used primarily for things like knowing which fields are dirty.
     * Yes, this will make for some long switch/case chains, but there are
     * many different types for fields.  This will become useful for event
     * processing.
     *
     * This enum should never be saved/restored; runtime use only.
     */
    enum EntityField
    {
        ENTITYFIELD_BEGIN,  ///< Always at the beginning.  Internal use only.
        ENTITYFIELD_BEGIN_ENTITY,  ///< Marker for beginning of Entity
        ENTITYFIELD_invalid, ///< Invalid field.  Error condition.
        ENTITYFIELD_type,    ///< Type of entity.
        ENTITYFIELD_id,      ///< ID of entity.
        ENTITYFIELD_name,    ///< Name of entity.
        ENTITYFIELD_note,    ///< Note concerning the Entity
        ENTITYFIELD_security,///< Security flags
        ENTITYFIELD_instance,///< Instance # of Entity
        ENTITYFIELD_version, ///< Version # of Entity
        ENTITYFIELD_created_timestamp, ///< When Entity was created
        ENTITYFIELD_updated_timestamp, ///< When Entity was last updated
        ENTITYFIELD_accessed_timestamp, ///< When Entity was last accessed
        ENTITYFIELD_access_count,       ///< How many times Entity accessed
        ENTITYFIELD_owner,         ///< Owner ID of Entity
        ENTITYFIELD_flags,         ///< Flags of Entity
        ENTITYFIELD_references,  ///< Who references Entity
        ENTITYFIELD_delete_batch_id, ///< Delete batch ID
        ENTITYFIELD_deleted_flag, ///< Delete flag
        ENTITYFIELD_END_ENTITY, ///< Marker for end of Entity
        ENTITYFIELD_BEGIN_GROUP, ///< Marker for beginning of Group
        ENTITYFIELD_group_ids,  ///< Group members
        ENTITYFIELD_group_disabled_ids,  ///< Group members, temporarily disabled
        ENTITYFIELD_END_GROUP, ///< Marker for end of Group
        ENTITYFIELD_BEGIN_PROPERTY_ENTITY, ///< Marker for beginning of Property
        ENTITYFIELD_application_properties, ///< Properties of PropertyEntity
        ENTITYFIELD_END_PROPERTY_ENTITY, ///< Marker for end of Property
        ENTITYFIELD_BEGIN_CONTAINER_PROPERTY_ENTITY, ///< Marker for begin of C P E
        ENTITYFIELD_contained_by, ///< Used only by DB dump, generic contained by - runtime uses specific ones below // TODO remove??
        ENTITYFIELD_linked_programs, ///< Linked Programs
        ENTITYFIELD_registrations, ///< Registration entries
        ENTITYFIELD_END_CONTAINER_PROPERTY_ENTITY, ///< Marker for end of C P E
        ENTITYFIELD_BEGIN_REGION, ///< Marker for beginning of Region
        ENTITYFIELD_END_REGION,  ///< Marker for ending of Region
        ENTITYFIELD_BEGIN_ROOM,  ///< Marker for beginning of Room
        ENTITYFIELD_END_ROOM,  ///< Marker for ending of Room
        ENTITYFIELD_BEGIN_PLAYER,  ///< Marker for beginning of Player
        ENTITYFIELD_password, ///< Player's encrypted password
        ENTITYFIELD_player_display_name, ///< Player's display name
        ENTITYFIELD_player_home, ///< Player's home Room
        ENTITYFIELD_player_last_connect, ///< When player last connected
        ENTITYFIELD_END_PLAYER,  ///< Marker for ending of Player
        ENTITYFIELD_BEGIN_THING, ///< Marker for beginning of Thing
        ENTITYFIELD_thing_home, ///< Thing's home
        ENTITYFIELD_thing_lock, ///< Thing's lock
        ENTITYFIELD_END_THING, ///< Marker for ending of Thing
        ENTITYFIELD_BEGIN_PUPPET,  ///< Marker for beginning of Puppet
        ENTITYFIELD_puppet_display_name, ///< Puppet display name
        ENTITYFIELD_END_PUPPET, ///< Marker for ending of Puppet
        ENTITYFIELD_BEGIN_VEHICLE,  ///< Marker for beginning of Vehicle
        ENTITYFIELD_vehicle_interior, ///< Vehicle's interior dbref
        ENTITYFIELD_vehicle_controller, ///< Vehicle's authorized controllers
        ENTITYFIELD_END_VEHICLE,  ///< Marker for ending of Vehicle
        ENTITYFIELD_BEGIN_PROGRAM, ///< Marker for beginning of Program
        ENTITYFIELD_program_runtime_sec, ///< Cumulative runtime of program
        ENTITYFIELD_program_reg_name, ///< Global registration name of program library
        ENTITYFIELD_program_source_code, ///< Raw source code of program
        ENTITYFIELD_program_compiled_code, ///< Opaque compiled version
        ENTITYFIELD_program_language, ///< What code language it is written in
        ENTITYFIELD_program_includes, ///< What other programs required to run
        ENTITYFIELD_END_PROGRAM, ///< Marker for ending of Program
        ENTITYFIELD_BEGIN_ACTION, ///< Marker for beginning of action
        ENTITYFIELD_action_targets, ///< What the action acts upon
        ENTITYFIELD_action_lock, ///< Lock to be evaluted before using action
        ENTITYFIELD_action_succ_msg, ///< Success message shown to user
        ENTITYFIELD_action_succ_room_msg, ///< Success message shown to room
        ENTITYFIELD_action_fail_msg, ///< Failure message shown to user
        ENTITYFIELD_action_fail_room_msg, ///< Failure message shown to room
        ENTITYFIELD_action_contained_by, ///< Where this action is
        ENTITYFIELD_action_commands, ///< Commands (aliases) that activate action
        ENTITYFIELD_END_ACTION, ///< Marker for ending of action
        ENTITYFIELD_BEGIN_EXIT, ///< Marker for beginning of exit
        ENTITYFIELD_exit_arrive_msg, ///< Message seen when entering room via exit
        ENTITYFIELD_exit_arrive_room_msg, ///< Message to room entering room via exit
        ENTITYFIELD_END_EXIT, ///< Marker for ending of exit
        ENTITYFIELD_END      ///< Always at the end.  Internal use only.
    };

    /**
     * Given an EntityField, return it as an equivalent string.
     * @param field[in] The field enum to convert.
     * @return The field enum as a string.
     */
    const std::string &entity_field_to_string(const EntityField field);

    /**
     * Given a string representing an EntityField, return the representative
     * enum.
     * @param str[in] The string to convert.  Must not have excess whitespace.
     * @return The equivalent EntityField or invalid.
     * @see EntityField
     */
    const EntityField string_to_entity_field(const std::string &str);
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_ENTITYFIELDS_H_ */
