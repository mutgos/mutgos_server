#ifndef MUTGOS_DBDUMP_DUMPREADERINTERFACE_H
#define MUTGOS_DBDUMP_DUMPREADERINTERFACE_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_PropertyDirectory.h"
#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_Security.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

namespace mutgos
{
namespace dbdump
{
    /**
     * An interface to the database subsystem, used by parsers capable of
     * reading various dump formats.  It provides some basic state machine
     * functionality and simplifies setting fields and properties.
     *
     * It is not thread safe, however multiple instances could in theory be
     * used at once.
     *
     * The general flow is:
     *   Make/Set site
     *   For each Entity
     *      Make/set Entity
     *      Enter Entity security mode
     *          Set various security settings.
     *          End security
     *      Set fields
     *      (each application) Enter Application security settings
     *          Set various security settings.
     *          End security
     *      End entity
     *   End site
     */
    class DumpReaderInterface
    {
    public:
        /**
         * Indicates the correct method to call to set a particular field.
         */
        enum SetFieldMethod
        {
            METHOD_invalid = 0, ///< Not a valid field to be set.
            METHOD_string,  ///< Use the string set field method
            METHOD_string_multiple,  ///< Use the string set field method
            METHOD_id,  ///< Use the ID set field method
            METHOD_id_multiple,  ///< Use the ID set field method
            METHOD_document, ///< Use the Document set field method
            METHOD_lock  ///< Use the Lock set field method
        };

        /**
         * Standard constructor.
         */
        DumpReaderInterface(void);

        /**
         * Standard destructor.
         */
        ~DumpReaderInterface();

        /**
         * @return Pointer to the underlying DbInterface instance.
         */
        dbinterface::DatabaseAccess *get_dbinterface(void) const
          { return db; }

        /**
         * Indicates the underlying parser found an error.  Clear any
         * references to pointers that may be deleted to avoid coredumps.
         */
        void set_error(void);

        /**
         * Makes a new site to work on.  Must not have an existing site selected.
         * @param site_name[in] The name of the site.  The name must not
         * already be in use.
         * @param site_id[out] The newly created site ID, if success.
         * @return True if successfully created, false if error or site has
         * already been selected.
         */
        bool make_site(
            const std::string &site_name,
            dbtype::Id::SiteIdType &site_id);

        /**
         * Sets the description for the current site.  Optional.
         * @param description[in] The new description for the site.
         * @return True if successfully set, false if error or site has
         * not been selected.
         */
        bool set_site_description(const std::string &description);

        /**
         * Sets an existing site to work on.
         * Must not have an existing site selected.
         * @param id[in] The site ID to set.
         * @return True if successfully selected, false if in the wrong
         * site does not exist or a site has already been selected.
         */
        bool set_site(const dbtype::Id::SiteIdType id);

        /**
         * Indicates all work on a site has completed.
         * @return True if success, false if it detects an entity has been
         * selected.
         */
        bool end_site(void);

        /**
         * @return The currently selected site ID.  Assumes a site is valid
         * and has been selected.
         */
        dbtype::Id::SiteIdType get_current_site(void) const
          { return current_site_id; }

        /**
         * Creates a new Entity for the current site and sets it as the current
         * Entity.
         * @param type[in] The type of Entity to create.  Not all types
         * are valid.
         * @return The ID of the created Entity, or a default ID if error or
         * wrong mode.
         */
        dbtype::Id make_entity(const dbtype::EntityType type);

        /**
         * Sets an existing Entity in the current site as the current Entity.
         * @param entity_id[in] The ID of the existing Entity to use.
         * The site will be overridden with the currently selected site ID.
         * @return True if success, false if the wrong mode or the Entity
         * does not exist.
         */
        bool set_entity(const dbtype::Id &entity_id);

        /**
         * Outputs the full data of the currently set Entity to the log.
         */
        void log_entity(void);

        /**
         * Indicates all work on an Entity has been completed (can select a
         * new Entity).
         * @return True if success, false if in the wrong mode (no Entity
         * selected, or in a sub-mode like setting security).
         */
        bool end_entity(void);

        /**
         * Shortcut to setting the current Entity's name.
         * @param name[in] The new name of the Entity.
         * @return True if success, false if no Entity selected.
         */
        bool set_entity_name(const std::string &name);

        /**
         * Shortcut to setting the current Entity's owner.
         * @param name[in] The new owner of the Entity.
         * @return True if success, false if no Entity selected.
         */
        bool set_entity_owner(const dbtype::Id &owner);

        /**
         * Adds the given flag to the selected Entity.
         * @param value[in] The flag to add.
         * @return True if added, false if flag is not valid or no Entity
         * selected.
         */
        bool add_entity_flag(const dbtype::Entity::FlagType &value);

        /**
         * Helps more dynamic parsers figure out which set_entity_*() method
         * to call for a field.
         * @param field[in] The field to query.
         * @return An enumeration that indicates which method should be called.
         */
        SetFieldMethod which_set_field_method(const dbtype::EntityField field) const;

        /**
         * Sets a string field on an Entity.
         * If this field accepts multiple strings, this will append a string,
         * not replace anything existing.
         * @param field[in] The field to set.
         * @param value[in] The value of the field.
         * @return True if success, false if the wrong type for the field or
         * no Entity selected.
         */
        bool set_entity_field(
            const dbtype::EntityField field,
            const std::string &value);

        /**
         * Sets an ID field on an Entity.
         * If this field accepts multiple IDs, this will append an ID, not
         * replace anything existing.
         * @param field[in] The field to set.
         * @param value[in] The value of the field.
         * @return True if success, false if the wrong type for the field or
         * no Entity selected.
         */
        bool set_entity_field(
            const dbtype::EntityField field,
            const dbtype::Id &value);

        /**
         * Sets a Document field on an Entity.
         * @param field[in] The field to set.
         * @param value[in] The value of the field.
         * @return True if success, false if the wrong type for the field or
         * no Entity selected.
         */
        bool set_entity_field(
            const dbtype::EntityField field,
            const dbtype::DocumentProperty &value);

        /**
         * Sets a lock field on an Entity.  This is for the lock by ID.
         * @param field[in] The field to set.
         * @param lock_id[in] The ID to lock against.  The Entity represented
         * by the ID must exist.
         * @param is_lock[in] True if lock evaluation is to be 'not'ed.
         * @return True if success, false if the wrong type for the field or
         * no Entity selected, or the lock_id does not exist.
         */
        bool set_entity_lock_field(
            const dbtype::EntityField field,
            const dbtype::Id &lock_id,
            const bool is_not);

        /**
         * Sets a lock field on an Entity.  This is for the lock by property.
         * @param field[in] The field to set.
         * @param value[in] The value of the field.
         * @param is_lock[in] True if lock evaluation is to be 'not'ed.
         * @return True if success, false if the wrong type for the field or
         * no Entity selected.
         */
        bool set_entity_lock_field(
            const dbtype::EntityField field,
            const dbtype::PropertyDirectory::PathString &lock_path,
            const dbtype::PropertyData &lock_data,
            const bool is_not);

        /**
         * Adds an application to the Entity.  An Application must be added
         * before security or property data is set on it.
         * @param application[in] The application to add.
         * @param owner[in] The ID of the owner of the application data.
         * @return True if success, false if no Entity selected or the
         * Application was not added (perhaps it already exists?).
         */
        bool add_application(
            const std::string &application,
            const dbtype::Id &owner);

        /**
         * Place interface into Entity security setter mode.
         * @return True if success, false if no Entity selected or already
         * in another mode.
         */
        bool set_entity_security(void);

        /**
         * Place interface into Entity security setter mode for a specific
         * application.
         * @param application[in] The application to set security for.
         * @return True if success, false if no Entity selected or already
         * in another mode.
         */
        bool set_application_props_security(
            const dbtype::PropertyDirectory::PathString &application);

        /**
         * Take interface out of security setter mode (Entity or application).
         * @return True if success, false if not in security setter mode.
         */
        bool end_security(void);

        /**
         * When in security setter mode, adds/sets a security flag to the
         * 'list' security settings.
         * @param flag[in] The flag to add/set as true.
         * @return True if success, false if not in the proper mode.
         */
        bool add_security_flag_list(const dbtype::SecurityFlag flag);

        /**
         * When in security setter mode, adds/sets a security flag to the
         * 'other' security settings.
         * @param flag[in] The flag to add/set as true.
         * @return True if success, false if not in the proper mode.
         */
        bool add_security_flag_other(const dbtype::SecurityFlag flag);

        /**
         * When in security setter mode, adds an ID to the 'group' security
         * settings.
         * @param id[in] The ID to add.
         * @return True if success, false if not in the proper mode.
         */
        bool add_to_security_group(const dbtype::Id &id);

        /**
         * When in security setter mode, adds an ID to the 'admin' list.
         * @param id[in] The ID to add.
         * @return True if success, false if not in the proper mode.
         */
        bool add_to_security_admins(const dbtype::Id &id);

        /**
         * A shortcut to setting a string property.
         * @param path[in] The full path of the property to set,
         * including the application.
         * @param data[in] The data to set.
         * @return True if success, false if no Entity selected, not in
         * the proper mode, or an error from the property setter is returned.
         */
        bool set_string_prop(
            const dbtype::PropertyDirectory::PathString &path,
            const std::string &data);

        /**
         * A shortcut to setting an int property.
         * @param path[in] The full path of the property to set,
         * including the application.
         * @param data[in] The data to set.
         * @return True if success, false if no Entity selected, not in
         * the proper mode, or an error from the property setter is returned.
         */
        bool set_int_prop(
            const dbtype::PropertyDirectory::PathString &path,
            const MG_SignedInt data);

        /**
         * A shortcut to setting a bool property.
         * @param path[in] The full path of the property to set,
         * including the application.
         * @param data[in] The data to set.
         * @return True if success, false if no Entity selected, not in
         * the proper mode, or an error from the property setter is returned.
         */
        bool set_bool_prop(
            const dbtype::PropertyDirectory::PathString &path,
            const bool data);

        /**
         * Sets any property using a string.
         * @param path[in] The full path of the property to set,
         * including the application.
         * @param type[in] The type of the property data.
         * @param data[in] The data to set, as a string.
         * @return True if success, false if no Entity selected, not in
         * the proper mode, or an error from the property setter is returned.
         */
        bool set_prop(
            const dbtype::PropertyDirectory::PathString &path,
            const dbtype::PropertyDataType &type,
            const std::string &data);

        /**
         * Helper method to create native PropertyData instances out of
         * strings.  Only the simple types (string, integer, float, boolean, id)
         * are supported.
         * @param type[in] The type of property data to create.
         * @param data[in] The string data to parse and convert.
         * @return The property data instance of the data, or null if parsing
         * error or not supported.  The caller must manage the pointer.
         */
        dbtype::PropertyData *create_property_data(
            const dbtype::PropertyDataType &type,
            const std::string &data) const;

        /**
         * Sets property data.
         * @param path[in] The full path of the property to set,
         * including the application.
         * @param data[in] The data to set.  The data will be copied.
         * @return True if success, false if no Entity selected, not in
         * the proper mode, or an error from the property setter is returned.
         */
        bool set_prop(
            const dbtype::PropertyDirectory::PathString &path,
            const dbtype::PropertyData &data);

    private:
        /**
         * Indicates what mode the interface is in when setting stuff on an
         * Entity.
         */
        enum SetMode
        {
            NORMAL,  ///< No special set mode is active
            ENTITY_SECURITY, ///< In mode to set entity security
            APPLICATION_SECURITY  ///< In mode ot set application security
        };

        dbinterface::DatabaseAccess * const db; ///< Access to the database

        dbtype::Id::SiteIdType current_site_id; ///< Site being worked on
        bool site_valid; ///< True if current_site_id is valid
        dbinterface::EntityRef current_entity; ///< Entity being worked on, if any
        SetMode mode; ///< The current 'set mode' for the Entity.
        dbtype::PropertyDirectory::PathString current_application; ///< Application being worked on for security
        MG_LongUnsignedInt temp_ser_id_name; ///< Serial number that can be used for temp names, etc.
    };
}
}

#endif //MUTGOS_DBDUMP_DUMPREADERINTERFACE_H
