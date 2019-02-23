/*
 * primitives_DatabasePrims.h
 */

#ifndef MUTGOS_PRIMITIVES_DATABASEPRIMS_H
#define MUTGOS_PRIMITIVES_DATABASEPRIMS_H

#include <string>
#include <map>
#include <vector>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Security.h"
#include "dbtypes/dbtype_PropertySecurity.h"
#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"
#include "dbtypes/dbtype_DocumentProperty.h"

#include "dbinterface/dbinterface_EntityRef.h"

#include "security/security_Context.h"

#include "primitives_Result.h"

namespace mutgos
{
namespace primitives
{
    /**
     * Primitives that deal with querying or manipulating the database or
     * related are here, except for anything relating to moving Entities
     * from one place to another.
     */
    class DatabasePrims
    {
    public:
        /** Used to control what contents to retrieve */
        enum ContentsEntityTypes
        {
            /** Only get actions and subclasses */
            CONTENTS_ACTIONS_ONLY,
            /** Only get non-actions (and subclasses), such as Things */
            CONTENTS_NON_ACTIONS_ONLY,
            /** Get both actions and entities */
            CONTENTS_ALL
        };


        /** Simple typedef to represent a Document Property's contents, one
            line per element */
        typedef std::vector<std::string> DocumentContents;

        /**
         * Constructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        DatabasePrims(void);

        /**
         * Destructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        ~DatabasePrims();

        /**
         * If the provided ID does not have a site ID, it will use the one
         * from the requester.
         * @param context[in] The execution context.
         * @param id[in] The ID to check and fix.
         * @return The fixed ID (will have a site ID).
         */
        dbtype::Id fix_site_on_id(
            const security::Context &context,
            const dbtype::Id &id) const
        {
            if (id.is_site_default())
            {
                return dbtype::Id(
                    context.get_requester().get_site_id(),
                    id.get_entity_id());
            }
            else
            {
                return id;
            }
        }

        /**
         * Determines if the provided Entity ID is valid (exists in the
         * database).
         * @param context[in] The execution context.
         * @param entity_id[in] The entity ID to check.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded (valid entity) or not
         * ('bad arguments' for an invalid Entity).
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result is_entity_valid(
            security::Context &context,
            const dbtype::Id &entity_id,
            const bool throw_on_violation = true);

        /**
         * Gets the entity type.
         * @param context[in] The execution context.
         * @param entity_id[in] The entity ID to check.
         * @param entity_type[out] The entity type.  Will be set to invalid
         * if entity is invalid.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded (valid entity) or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_entity_type(
            security::Context &context,
            const dbtype::Id &entity_id,
            dbtype::EntityType &entity_type,
            const bool throw_on_violation = true);

        /**
         * Calls to_string() on Entity.  This will be removed post-demo
         * and replaced with something more robust.
         * @param context[in] The execution context.
         * @param entity_id[in] The ID of the Entity to call to_string() on.
         * @param output[out] The to_string() output will be appended to
         * this, if successful.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result entity_to_string(
            security::Context &context,
            const dbtype::Id &entity_id,
            std::string &output,
            const bool throw_on_violation = true);

        /**
         * Returns who currently contains the Entity.
         * @param context[in] The execution context.
         * @param entity_id[in] The ID of the Entity to get the location of.
         * @param entity_location[out] The location of the Entity, if able to
         * find it.  If error or not applicable for the Entity type, this will
         * be left unchanged.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_entity_location(
            security::Context &context,
            const dbtype::Id &entity_id,
            dbtype::Id &entity_location,
            const bool throw_on_violation = true);

        /**
         * Gets the contents of the specified container, if security allows it.
         * @param context[in] The execution context.
         * @param container[in] The container to get the contents from.
         * @param types[in] Used to filter what type of entities should be
         * returned.  For instance, if only actions are desired, this can
         * filter out all non-actions.
         * @param contents[out] The contents of the container, with the filter
         * applied (types argument).  IDs will only ever be appended; nothing
         * will ever be erased.  Duplicate checks will not be performed.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_contents(
            security::Context &context,
            const dbtype::Id &container,
            const ContentsEntityTypes types,
            dbtype::Entity::IdVector &contents,
            const bool throw_on_violation = true);

        // TODO Post-demo, consider enhancing database to store normalized exits for searching without loading entity
        // TODO Post-demo, consider breaking apart to several methods, or simplifying inputs
        /**
         * Tries to find an Entity that matches the given search string.  As
         * needed, it will search up the environment.
         * Whether or not an Entity can be found also depends on the
         * capabilites granted.
         * This is not a general purpose find utility; this is why the types
         * to be filtered on are not complete.  It is used primarily to find
         * anything that matches in the current room (and up the Regions),
         * and matching players (for paging, etc).  In the future, the types
         * could be expanded upon as needed.
         *
         * This will also search all aliases of an ActionEntity.
         *
         * Currently, this method CANNOT throw a SecurityException or indicate
         * a security failure, due to how matching works (and the desire to not
         * give away too much information to the caller).  Instead,
         * found_entity will be set to default.
         * @param context[in] The execution context.
         * @param search_string[in] The name of the Entity to search for.
         * @param exact_match[in] True if search_string is to be an exact
         * match for the name, false if a partial match is acceptable.  Actions
         * are always an exact match (flag ignored).  This is case insensitive.
         * @param entity_type[in] Valid enum values are 'entity' (for all
         * types from current requester location), 'player' (for players and
         * guests only, no matter where they are located), 'action' (for exits
         * and commands starting from current requester location).
         * @param found_entity[out] If match is found, the ID will be set
         * here.  If no match found, the match is ambigious (multiple results),
         * or the security does not allow it to be found, an invalid ID will
         * be set.
         * @param ambiguous[out] If found_entity is default, this may be set
         * to true to indicate the search determined there were multiple
         * results and it could not decide which to pick.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result match_name_to_id(
            security::Context &context,
            const std::string &search_string,
            const bool exact_match,
            const dbtype::EntityType entity_type,
            dbtype::Id &found_entity,
            bool &ambiguous,
            const bool throw_on_violation = true);

        /**
         * Converts an ID in the form of #12-345 (or #1234) into an
         * dbtype::Id.
         * @param context[in] The security context.  Also used to fill in the
         * site if not specified.
         * @param id_as_string[in] The ID to convert, in string form.
         * @return The string ID as a dbtype::Id, or default if invalid
         * format.
         */
        dbtype::Id convert_string_to_id(
            security::Context &context,
            const std::string &id_as_string);

        /**
         * Parses a provided string into three parts: The Entity holding the
         * property, the property itself, and optionally a new property value.
         * The Entity can be given either as an ID or a name (will search the
         * environment for it).  This does not check to see if context has
         * permission to access the entity or property, except whatever
         * permissions are used in the course of searching by name.
         * The format is:  entityname=/property/path:prop_value
         * Excess spaces are allowed.
         * @param context[in] The security context.  Also used to fill in the
         * site if not specified.
         * @param input_string[in] The string to parse.
         * @param entity_id[out] If successful, contains the Entity the
         * property is on.
         * @param property[out] If successful, contains the full property path.
         * @param has_property_value[out] If successful, will be true if
         * a property value is provided (which could be empty).  The syntax
         * of the path is not checked for validity.
         * @param property_value[out] If successful, contains the new property
         * value as a string.
         * @param ambiguous[out] If failure due to the entity name being
         * ambiguous, this will be set to true, otherwise false.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result convert_string_to_prop_params(
            security::Context &context,
            const std::string &input_string,
            dbtype::Id &entity_id,
            std::string &property,
            bool &has_property_value,
            std::string &property_value,
            bool &ambiguous,
            const bool throw_on_violation = true);

        /**
         * Converts an ID to its name, if allowed.
         * @param context[in] The security context.
         * @param id[in] The ID to convert into a name.
         * @param append_id[in] True to append the ID at the end of the name.
         * Normally you do not want this if sending to the client; you will
         * want to manually format as an External ID type.
         * @param name[out] The name of the Entity will be set here.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result convert_id_to_name(
            security::Context &context,
            const dbtype::Id &id,
            const bool append_id,
            std::string &name,
            const bool throw_on_violation = true);

        /**
         * Creates an Entity of the given type.  It will be created in the
         * requester's site, and the requester will be the owner.  Defaults
         * will be provided for what contains it.
         * @param context[in] The security context.
         * @param type[in] The type of Entity to create.
         * @param name[in] The name of the entity.  Cannot be empty.
         * @param created_entity_id[out] The ID of the Entity that was created,
         * if successful.  If not successful, this will be default.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result create_entity(
            security::Context &context,
            const dbtype::EntityType type,
            const std::string &name,
            dbtype::Id &created_entity_id,
            const bool throw_on_violation = true);

        /**
         * Deletes the given entity from the database, if allowed.
         * @param context[in] The security context.
         * @param entity_id[in] The Entity to delete.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result delete_entity(
            security::Context &context,
            const dbtype::Id &entity_id,
            const bool throw_on_violation = true);

        /**
         * Gets the security object for an Entity.
         * @param context[in] The security context.
         * @param entity_id[in] The Entity to get the Security object from.
         * @param entity_security[out] A copy of the Security object, or a
         * default version if it could not be retrieved.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_entity_security(
            security::Context &context,
            const dbtype::Id &entity_id,
            dbtype::Security &entity_security,
            const bool throw_on_violation = true);

        // TODO These demo-level security settings will need to be redone later

        /**
         * Sets the read/write flags on the 'other' security field.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to change the read/write flags on
         * the 'other' security field.
         * @param allow_read_write[in] True to set the read/write flags on
         * 'other', false to unset both flags.
         * @param throw_on_violation
         * @return
         */
        Result set_entity_security_other(
            security::Context &context,
            const dbtype::Id &entity_id,
            const bool allow_read_write,
            const bool throw_on_violation = true);

        /**
         * Adds an admin to an Entity's security.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to add the admin to.
         * @param admin_id[in] The admin to add.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_entity_security_add_admin(
            security::Context &context,
            const dbtype::Id &entity_id,
            const dbtype::Id &admin_id,
            const bool throw_on_violation = true);

        /**
         * Removes an admin from an Entity's security.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to remove the admin from.
         * @param admin_id[in] The admin to remove.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_entity_security_remove_admin(
            security::Context &context,
            const dbtype::Id &entity_id,
            const dbtype::Id &admin_id,
            const bool throw_on_violation = true);

        /**
         * Gets the application security object for an Entity's properties.
         * @param context[in] The security context.
         * @param entity_id[in] The Entity to get the Security object from.
         * @param application[in] The application (full path OK) whose
         * security is to be retrieved.
         * @param application_security[out] A copy of the PropertySecurity
         * object, or a default version if it could not be retrieved.
         * @param owner[out] The owner of the application, or invalid if
         * it could not be retrieved.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_application_security(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &application,
            dbtype::PropertySecurity &application_security,
            dbtype::Id &owner,
            const bool throw_on_violation = true);

        /**
         * Sets the write flag on the 'other' security field of an application.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to change the write flag on
         * the 'other' security field of an application.
         * @param application[in] The application (full path OK) whose
         * security is to be modified.
         * @param allow_write[in] True to set the write flag on
         * 'other', false to unset flag.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
          */
        Result set_application_security_other(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &application,
            const bool allow_write,
            const bool throw_on_violation = true);

        /**
         * Adds an admin to an applications's security.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to add an admin of an
         * application.
         * @param application[in] The application (full path OK) whose
         * security is to be modified.
         * @param admin_id[in] The admin to add.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_application_security_add_admin(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &application,
            const dbtype::Id &admin_id,
            const bool throw_on_violation = true);

        /**
         * Removes an admin from an applications's security.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to remove an admin of an
         * application.
         * @param application[in] The application (full path OK) whose
         * security is to be modified.
         * @param admin_id[in] The admin to remove.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_application_security_remove_admin(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &application,
            const dbtype::Id &admin_id,
            const bool throw_on_violation = true);

        /**
         * Determines if an action can be used without actually using it.
         * @param context[in] The security context.
         * @param action_id[in] The ID of the action to check.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result can_use_action(
            security::Context &context,
            const dbtype::Id &action_id,
            const bool throw_on_violation = true);

        /**
         * Gets the type of a property.  This can also be used to determine
         * if a property exists.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to get the property from.
         * @param property_path[in] The full property path (including the
         * application) to retrieve.
         * @param type[out] The type of the property, or 'invalid' if it
         * doesn't exist or there is some other error.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_application_property_type(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            dbtype::PropertyDataType &type,
            const bool throw_on_violation = true);

        /**
         * Gets a string property.  It can also convert non-string properties
         * to string form.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to get the property from.
         * @param property_path[in] The full property path (including the
         * application) to retrieve.
         * @param property_value[out] A copy of the property value. If error
         * (doesn't exist, wrong property type, etc), this will be left
         * unchanged to allow for a default value.
         * @param convert_nonstring[in] If true, if the property is not of a
         * string type, convert it to a string and set to property_value.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            std::string &property_value,
            const bool convert_nonstring,
            const bool throw_on_violation = true);

        /**
         * Gets a signed int property.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to get the property from.
         * @param property_path[in] The full property path (including the
         * application) to retrieve.
         * @param property_value[out] A copy of the property value. If error
         * (doesn't exist, wrong property type, etc), this will be left
         * unchanged to allow for a default value.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            MG_SignedInt &property_value,
            const bool throw_on_violation = true);

        /**
         * Gets a document property.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to get the property from.
         * @param property_path[in] The full property path (including the
         * application) to retrieve.
         * @param property_value[out] A copy of the property value. If error
         * (doesn't exist, wrong property type, etc), this will be left
         * unchanged to allow for a default value.  Each line of a document
         * is an element.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            DocumentContents &property_value,
            const bool throw_on_violation = true);

        /**
         * Sets a signed int property, creating the application as needed.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to set the property on.
         * @param property_path[in] The full property path (including the
         * application) to set.
         * @param property_value[in] The property value to set.  A copy will
         * be made.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            const MG_SignedInt property_value,
            const bool throw_on_violation = true);

        /**
         * Sets a string property, creating the application as needed.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to set the property on.
         * @param property_path[in] The full property path (including the
         * application) to set.
         * @param property_value[in] The property value to set.  A copy will
         * be made.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            const std::string &property_value,
            const bool throw_on_violation = true);

        /**
         * Sets a Document property, creating the application as needed.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to set the property on.
         * @param property_path[in] The full property path (including the
         * application) to set.
         * @param property_value[in] The property value to set.  A copy will
         * be made.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_application_property(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            const dbtype::DocumentProperty &property_value,
            const bool throw_on_violation = true);

    private:
        typedef std::pair<dbtype::Id, std::string> IdNamePair;
        typedef std::vector<IdNamePair> IdNamesVector;

        /**
         * Finds a matching player.
         * @param context[in] The security context.
         * @param search_string_lower[in] The name of the character to search
         * for.  Must be all lowercase.
         * @param exact_match[in] True if name must be an exact match to the
         * search string.  If false, it will check online players for a
         * partial match, then check all players for an exact match.
         * @param result[out] If this call succeeded or not.  It may succeed
         * yet not find any matching player.
         * @param found_entity[out] If found, this will be populated with the
         * ID of the found player.  If not found, this will be set to default.
         * @param ambiguous[out] Set to true if it could not determine an exact
         * match for the search string (multiple matches returned).
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        void match_character(
            security::Context &context,
            const std::string &search_string_lower,
            const bool exact_match,
            Result &result,
            dbtype::Id &found_entity,
            bool &ambiguous,
            const bool throw_on_violation);

        /**
         * Tries to find an Entity whose name of command aliases match
         * the given search string, starting from the requester.
         * This will check the requester's contents, then the room, then
         * all regions above.
         * @param context[in] The security context.
         * @param search_string_lower[in] The name or command alias to search
         * for.
         * @param exact_match[in] True if the name must be an exact match.
         * Action aliases are always an exact match (no partials allowed).  If
         * false, a partial match on a name is considered a match.
         * @param entity_types[in] Indicates what types of entities are
         * to be considered for matching.
         * @param result[out] If this call succeeded or not.  It may succeed
         * yet not find any matching player.
         * @param found_entity[out] If found, this will be populated with the
         * ID of the found Entity.  If not found, this will be set to default.
         * @param ambiguous[out] Set to true if it could not determine an exact
         * match for the search string (multiple matches returned).
         */
        void match_in_environment(
            security::Context &context,
            const std::string &search_string_lower,
            const bool exact_match,
            const ContentsEntityTypes entity_types,
            Result &result,
            dbtype::Id &found_entity,
            bool &ambiguous);

        /**
         * This is used as a sort of post-processor after calling
         * get_contents().
         * What it does is find the actions contained on entities (if desired)
         * in the contents list, and puts them in the contents list as well.
         * It can also remove non-actions if desired.  The output can then be
         * fed into match_name_in_contents() to match anything in a container
         * that would actually match, including elegible actions inside of
         * other Entities.
         * @param context[in] The security context.
         * @param contents[in] The IDs directly listed as an Entity's
         * contents.
         * @param entity_types[in] If actions only, the output will only have
         * actions and any actions contained by entities (if passes security).
         * If non-actions only, the output will only have non-action Entities.
         * If all, then it will be everything above.
         * @param effective_contents[out] This is a combination of 'contents'
         * with the actions filter applied, plus any actions contained in
         * any entities in the contents.  IDs will only ever be appended;
         * nothing will ever be erased.  Duplicate checks will not be
         * performed.
         */
        void filter_enhance_contents(
            security::Context &context,
            const dbtype::Entity::IdVector &contents,
            const ContentsEntityTypes entity_types,
            dbtype::Entity::IdVector &effective_contents);

        /**
         * Given IDs to check (usually the direct contents of an Entity),
         * search through the contents to see if the given search_string
         * matches any of the names (or command alias in the case of actions,
         * which are always exact).
         * This method will not recursively check for matches on anything
         * contained inside the entities provided.
         * @param context[in] The security context.  Must have valid requester.
         * @param contents[in] The IDs of the Entities to search, usually from
         * another Entity's contents.  May be actions, non-actions, or a
         * combination.
         * @param search_string[in] The string to search for (always exact
         * on actions, and will check command aliases).  Should be lower case.
         * @param exact_match[in] True for exact match on name, or false for
         * partial match of a name being acceptable.
         * @param found_entity[out] If a match is found, this will contain
         * the best/unambiguous match.  If a match is not found, this
         * will be set to default.
         * @param found_exact_match[out] If true, the match found is an exact
         * match, false if it was a partial match.
         * @param ambiguous[out] If true, indicates multiple matches found
         * but that it cannot determine which one is the match.  When true,
         * found_entity will be set to default.
         * @return True if any type of match is found (ambiguous or not),
         * false if nothing at all matches.
         */
        bool match_name_in_contents(
            security::Context &context,
            const dbtype::Entity::IdVector &contents,
            const std::string &search_string,
            const bool exact_match,
            dbtype::Id &found_entity,
            bool &found_exact_match,
            bool &ambiguous);

        /**
         * Attempts to match an Entity name against a given search string.
         * @param name[in] The name to try matching.  Not case sensitive.
         * @param search_string[in] The search string to find inside the name.
         * Must be in lowercase.
         * @param exact_match[in] True if search_string must exactly equal
         * the name, false if search_string can be considered a match if
         * found anywhere in name.
         * @param found_exact_match[out] True if what actually matched is'
         * exact, even if the original search was partial.
         * @return True if a match was found with the given parameters, false
         * if not.
         */
        bool match_name(
            const std::string &name,
            const std::string &search_string,
            const bool exact_match,
            bool &found_exact_match);

        /**
         * Given a newly created entity, sets any defaults appropriate
         * for the entity type.
         * It is assumed the method is called with a context of the owner;
         * security checks are currently not done.
         * @param context[in] The security context.
         * @param entity_ref[in] The newly created Entity whose defaults need
         * to be set.
         */
        void set_new_entity_defaults(
            security::Context &context,
            dbinterface::EntityRef &entity_ref);

        /**
         * Gets the raw property data.  This is a helper method to do all the
         * repetitive parts for getting a property.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to get the property from.
         * @param property_path[in] The full property path (including the
         * application) to retrieve.
         * @param property_value_ptr[out] The property value.  If success,
         * this will be set to a pointer with the raw property data.  The data
         * will be a copy and the caller must manage the pointer.  If failure,
         * this will be set to null.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result get_property_raw(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            dbtype::PropertyData *&property_value_ptr,
            const bool throw_on_violation);

        /**
         * Sets the raw property data, creating the application and setting
         * security defaults in the process.  This is a helper method to do
         * all the repetitive parts for setting a property.
         * @param context[in] The security context.
         * @param entity_id[in] The entity to set the property on.
         * @param property_path[in] The full property path (including the
         * application) to set.
         * @param property_value_ptr[in] The property data.  It will be
         * copied.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws SecurityException If conditions are met
         * (see throw_on_violation).
         */
        Result set_property_raw(
            security::Context &context,
            const dbtype::Id &entity_id,
            const std::string &property_path,
            const dbtype::PropertyData * const property_value_ptr,
            const bool throw_on_violation);
    };
}
}

#endif //MUTGOS_SERVER_PRIMITIVES_DATABASEPRIMS_H
