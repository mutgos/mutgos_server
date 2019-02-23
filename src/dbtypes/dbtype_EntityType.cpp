/*
 * dbtype_EntityType.cpp
 */

#include <string>

#include "dbtypes/dbtype_EntityType.h"

namespace
{
    const static std::string ENTITY_TYPE_AS_STRING[] =
    {
        "invalid",
        "entity",
        "property_entity",
        "container_property_entity",
        "region",
        "room",
        "player",
        "guest",
        "thing",
        "puppet",
        "vehicle",
        "group",
        "capability",
        "program",
        "action",
        "exit",
        "command"
    };

    const static std::string ENTITY_TYPE_AS_FRIENDLY_STRING[] =
    {
        "INVALID",
        "Entity",
        "PropertyEntity",
        "ContainerPropertyEntity",
        "Region",
        "Room",
        "Player",
        "Guest",
        "Thing",
        "Puppet",
        "Vehicle",
        "Group",
        "Capability",
        "Program",
        "Action",
        "Exit",
        "Command"
    };
}

namespace mutgos
{
namespace dbtype
{
    // -----------------------------------------------------------------------
    const std::string &entity_type_to_string(const EntityType type)
    {
        if ((type >= ENTITYTYPE_END) or (type <= ENTITYTYPE_invalid))
        {
            return ENTITY_TYPE_AS_STRING[0];
        }

        return ENTITY_TYPE_AS_STRING[type];
    }

    // -----------------------------------------------------------------------
    const std::string &entity_type_to_friendly_string(const EntityType type)
    {
        if ((type >= ENTITYTYPE_END) or (type <= ENTITYTYPE_invalid))
        {
            return ENTITY_TYPE_AS_FRIENDLY_STRING[0];
        }

        return ENTITY_TYPE_AS_FRIENDLY_STRING[type];
    }

    // -----------------------------------------------------------------------
    EntityType string_to_entity_type(const std::string &str)
    {
        EntityType type = ENTITYTYPE_invalid;

        // Check each string for a match.
        for (int index = 1; index < ENTITYTYPE_END; ++index)
        {
            if (ENTITY_TYPE_AS_STRING[index] == str)
            {
                type = (EntityType) index;
                break;
            }

            if (ENTITY_TYPE_AS_FRIENDLY_STRING[index] == str)
            {
                type = (EntityType) index;
                break;
            }
        }

        return type;
    }
}
} /* namespace mutgos */
