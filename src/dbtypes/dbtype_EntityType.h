#ifndef MUTGOS_DBTYPE_ENTITY_TYPE_H_
#define MUTGOS_DBTYPE_ENTITY_TYPE_H_

/*
 * dbtype_EntityType.h
 */

#include <string>

namespace mutgos
{
namespace dbtype
{
    /**
     * An enum which lists all the valid database entity types.  Only add types
     * to the list, never delete
     */
    enum EntityType
    {
        ENTITYTYPE_invalid = 0,               ///< invalid
        ENTITYTYPE_entity,                    ///< entity
        ENTITYTYPE_property_entity,           ///< property_entity
        ENTITYTYPE_container_property_entity, ///< container_property_entity
        ENTITYTYPE_region,                    ///< region
        ENTITYTYPE_room,                      ///< room
        ENTITYTYPE_player,                    ///< player
        ENTITYTYPE_guest,                     ///< guest
        ENTITYTYPE_thing,                     ///< thing
        ENTITYTYPE_puppet,                    ///< puppet
        ENTITYTYPE_vehicle,                   ///< vehicle
        ENTITYTYPE_group,                     ///< group
        ENTITYTYPE_capability,                ///< capability
        ENTITYTYPE_program,                   ///< program
        ENTITYTYPE_action,                    ///< action, abstract only!
        ENTITYTYPE_exit,                      ///< exit
        ENTITYTYPE_command,                   ///< command
        ENTITYTYPE_END                        ///< Always at the end
    };

    /**
     * Given an EntityType, return it as a string.
     * @param[in] type The type to convert.
     * @return type as a string.
     */
    const std::string &entity_type_to_string(const EntityType type);

    /**
     * Given an EntityType, return it as a string that could be shown to a
     * user or translated.
     * @param[in] type The type to convert.
     * @return type as a 'friendly' string suitable for display or translation.
     */
    const std::string &entity_type_to_friendly_string(const EntityType type);

    /**
     * Given a string representing an EntityType (friendly or normal), return
     * the representative enum.
     * @param[in] str The string to convert.  Must not have excess whitespace
     * and be an exact match.
     * @return The equivalent EntityType enum or invalid.
     * @see EntityType
     */
    EntityType string_to_entity_type(const std::string &str);
}  /* namespace dbtype */
} /* namespace mutgos */

#endif  /* DBTYPE_ENTITY_TYPE_H_ */
