/*
 * dbtype_PropertyEntity.cpp
 */

#include <string>
#include <ostream>

#include <boost/algorithm/string/trim.hpp>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_PropertyDataType.h"
#include "dbtypes/dbtype_PropertySecurity.h"

#include "dbtypes/dbtype_BooleanProperty.h"
#include "dbtypes/dbtype_FloatProperty.h"
#include "dbtypes/dbtype_IntegerProperty.h"
#include "dbtypes/dbtype_StringProperty.h"

#include "dbtypes/dbtype_Id.h"

namespace
{
    // @see PropertyDirectory
    /** Currently this can only be one character */
    static const std::string PATH_SEPARATOR = "/";
}

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    PropertyEntity::PropertyEntity()
      : Entity()
    {
    }

    // ----------------------------------------------------------------------
    PropertyEntity::PropertyEntity(const Id &id)
      : Entity(
          id,
          ENTITYTYPE_property_entity,
          0,
          0)
    {
    }

    // ----------------------------------------------------------------------
    PropertyEntity::~PropertyEntity()
    {
    }

    // -----------------------------------------------------------------------
    Entity *PropertyEntity::clone(
        const Id &id,
        const Entity::VersionType version,
        const Entity::InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new PropertyEntity(
                id,
                ENTITYTYPE_property_entity,
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
    size_t PropertyEntity::mem_used_fields(void)
    {
        // Get parent class size
        size_t total_size = Entity::mem_used_fields();

        // Add up the application properties
        total_size += sizeof(application_properties);

        for (ApplicationPropertiesMap::iterator app_iter =
                application_properties.begin();
            app_iter != application_properties.end();
            ++app_iter)
        {
            total_size += sizeof(app_iter->first) + app_iter->first.size();
            total_size += app_iter->second.mem_used();
        }

        return total_size;
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << Entity::to_string();

        for (ApplicationPropertiesMap::iterator app_iter =
                application_properties.begin();
            app_iter != application_properties.end();
            ++app_iter)
        {
            strstream << "----Application Property----" << std::endl;
            strstream << app_iter->second.to_string();
            strstream << "----End Application Property----" << std::endl;
        }

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::application_exists(const std::string &path)
    {
        concurrency::ReaderLockToken token(*this);

        return application_exists(path, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::application_exists(
        const std::string &path,
        concurrency::ReaderLockToken &token)
    {
        bool exists = false;

        if (token.has_lock(*this))
        {
            const std::string application =
                get_application_name_from_path(path);

            exists = application_properties.find(application) !=
                       application_properties.end();
        }
        else
        {
            LOG(error, "dbtype", "application_exists",
                "Using the wrong lock token!");
        }

        return exists;
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::add_application(
        const std::string &path,
        const Id &owner,
        const PropertySecurity &security)
    {
        concurrency::WriterLockToken token(*this);

        return add_application(path, owner, security, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::add_application(
        const std::string &path,
        const Id &owner,
        const PropertySecurity &security,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            const std::string application =
                get_application_name_from_path(path);

            if (not application.empty() and
                  (application_properties.find(application) ==
                  application_properties.end()))
            {
                // Insert the application properties (empty) and set the
                // security all at once.
                application_properties.insert(std::make_pair(
                    application, ApplicationProperties(application, owner))).
                        first->second.get_security() = security;

                success = true;
                notify_field_changed(ENTITYFIELD_application_properties);
            }
        }
        else
        {
            LOG(error, "dbtype", "add_application",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::remove_application(
        const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        remove_application(path, token);
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::remove_application(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            const std::string application =
                get_application_name_from_path(path);

            application_properties.erase(application);
            notify_field_changed(ENTITYFIELD_application_properties);
        }
        else
        {
            LOG(error, "dbtype", "remove_application",
                "Using the wrong lock token!");
        }
    }

    // ----------------------------------------------------------------------
    PropertyEntity::ApplicationOwnerSecurity
    PropertyEntity::get_application_security_settings(
        const std::string &path)
    {
        concurrency::ReaderLockToken token(*this);

        return get_application_security_settings(path, token);
    }

    // ----------------------------------------------------------------------
    PropertyEntity::ApplicationOwnerSecurity
    PropertyEntity::get_application_security_settings(
        const std::string &path,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            const std::string application =
                get_application_name_from_path(path);

            ApplicationPropertiesMap::iterator app_iter =
                application_properties.find(application);

            if (app_iter != application_properties.end())
            {
                return std::make_pair(
                    app_iter->second.get_application_owner(),
                    app_iter->second.get_security());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_application_security_settings",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::make_pair(Id(), PropertySecurity());
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::set_application_security_settings(
        const std::string &path,
        const PropertySecurity &security)
    {
        concurrency::WriterLockToken token(*this);

        return set_application_security_settings(path, security, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::set_application_security_settings(
        const std::string &path,
        const PropertySecurity &security,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            const std::string application =
                get_application_name_from_path(path);

            ApplicationPropertiesMap::iterator app_iter =
                application_properties.find(application);

            if (app_iter != application_properties.end())
            {
                app_iter->second.get_security() = security;
                notify_field_changed(ENTITYFIELD_application_properties);
                return true;
            }
        }
        else
        {
            LOG(error, "dbtype", "set_application_security_settings",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return false;
    }

    // ----------------------------------------------------------------------
    PropertyData *PropertyEntity::get_property(
        const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_property(path, token);
    }

    // ----------------------------------------------------------------------
    PropertyData *PropertyEntity::get_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            PropertyData *data_ptr = get_property_data_ptr(path);

            if (data_ptr)
            {
                return data_ptr->clone();
            }
        }
        else
        {
            LOG(error, "dbtype", "get_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return 0;
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_string_property(
        const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_string_property(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_string_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            PropertyData *data_ptr = get_property_data_ptr(path);

            if (data_ptr)
            {
                return data_ptr->get_as_string();
            }
        }
        else
        {
            LOG(error, "dbtype", "get_string_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::string();
    }

    // ----------------------------------------------------------------------
    MG_SignedInt PropertyEntity::get_int_property(
        const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_int_property(path, token);
    }

    // ----------------------------------------------------------------------
    MG_SignedInt PropertyEntity::get_int_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            PropertyData *data_ptr = get_property_data_ptr(path);

            if (data_ptr)
            {
                switch (data_ptr->get_data_type())
                {
                    case PROPERTYDATATYPE_integer:
                    {
                        return ((IntegerProperty *) data_ptr)->get();
                        break;
                    }

                    case PROPERTYDATATYPE_float:
                    {
                        return (MG_SignedInt)
                            ((FloatProperty *) data_ptr)->get();
                        break;
                    }

                    default:
                    {
                        return 0;
                    }
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_int_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return 0;
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::get_bool_property(
        const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_bool_property(path, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::get_bool_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            PropertyData *data_ptr = get_property_data_ptr(path);

            if (data_ptr)
            {
                switch (data_ptr->get_data_type())
                {
                    case PROPERTYDATATYPE_boolean:
                    {
                        return ((BooleanProperty *) data_ptr)->get();
                        break;
                    }

                    case PROPERTYDATATYPE_integer:
                    {
                        return ((IntegerProperty *) data_ptr)->get();
                        break;
                    }

                    case PROPERTYDATATYPE_float:
                    {
                        return (MG_SignedInt)
                            ((FloatProperty *) data_ptr)->get();
                        break;
                    }

                    case PROPERTYDATATYPE_string:
                    {
                        // Use the conversion facility of BooleanProperties
                        // to convert a string into a boolean.
                        //
                        BooleanProperty conversion;
                        conversion.set_from_string(data_ptr->get_as_string());

                        return conversion.get();
                        break;
                    }

                    default:
                    {
                        return false;
                    }
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_bool_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return false;
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::set_property(
        const std::string &path,
        const PropertyData &data)
    {
        concurrency::WriterLockToken token(*this);

        return set_property(path, data, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::set_property(
        const std::string &path,
        const PropertyData &data,
        concurrency::WriterLockToken &token)
    {
        bool success = false;

        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and set the property data.
                success = properties_ptr->get_properties().set_property(
                    property_path,
                    data);

                notify_field_changed(ENTITYFIELD_application_properties);
            }
        }
        else
        {
            LOG(error, "dbtype", "set_property",
                "Using the wrong lock token!");
        }

        return success;
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_next_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_next_property(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_next_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and get the next property.
                return properties_ptr->get_properties().get_next_property(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::string();
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_previous_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_previous_property(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_previous_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and get the prev property.
                return properties_ptr->get_properties().get_previous_property(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "get_previous_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::string();
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_first_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_first_property(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_first_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and get the first property.
                return properties_ptr->get_properties().get_first_property(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::string();
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_last_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_last_property(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_last_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and get the last property.
                return properties_ptr->get_properties().get_last_property(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_property",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return std::string();
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::delete_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        delete_property(path, token);
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::delete_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and delete the property.
                properties_ptr->get_properties().delete_property(property_path);
                notify_field_changed(ENTITYFIELD_application_properties);
            }
        }
        else
        {
            LOG(error, "dbtype", "delete_property",
                "Using the wrong lock token!");
        }
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::delete_property_data(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        delete_property_data(path, token);
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::delete_property_data(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now try and delete the property.
                properties_ptr->get_properties().delete_property_data(
                    property_path);
                notify_field_changed(ENTITYFIELD_application_properties);
            }
        }
        else
        {
            LOG(error, "dbtype", "delete_property_data",
                "Using the wrong lock token!");
        }
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::property_has_data(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return property_has_data(path, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::property_has_data(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return get_property_data_ptr(path);
        }
        else
        {
            LOG(error, "dbtype", "property_has_data",
                "Using the wrong lock token!");
        }

        // Could not locate or bad lock
        return false;
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::is_property(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return is_property(path, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::is_property(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now see if property exists.
                return properties_ptr->get_properties().does_property_exist(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "is_property",
                "Using the wrong lock token!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    PropertyDataType PropertyEntity::get_property_type(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return get_property_type(path, token);
    }

    // ----------------------------------------------------------------------
    PropertyDataType PropertyEntity::get_property_type(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        PropertyDataType result = PROPERTYDATATYPE_invalid;

        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now see get the property type.
                PropertyData * const data_ptr =
                    properties_ptr->get_properties().get_property_data(
                        property_path);

                if (data_ptr)
                {
                    result = data_ptr->get_data_type();
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "is_property",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::is_property_directory(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        return is_property_directory(path, token);
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::is_property_directory(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Now see if property is a directory.
                return properties_ptr->get_properties().is_property_directory(
                    property_path);
            }
        }
        else
        {
            LOG(error, "dbtype", "is_property_directory",
                "Using the wrong lock token!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::clear(const std::string &path)
    {
        concurrency::WriterLockToken token(*this);

        clear(path, token);
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::clear(
        const std::string &path,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationProperties *properties_ptr = 0;
            std::string property_path;

            if (get_application_properties(path, properties_ptr, property_path))
            {
                // Found the application.  Clear all properties.
                properties_ptr->get_properties().clear();
                notify_field_changed(ENTITYFIELD_application_properties);
            }
        }
        else
        {
            LOG(error, "dbtype", "clear",
                "Using the wrong lock token!");
        }
    }


    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_first_application_name(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_application_name(token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_first_application_name(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (not application_properties.empty())
            {
                return application_properties.begin()->first;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_application_name",
                "Using the wrong lock token!");
        }

        // Lock error or empty
        return std::string();
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_next_application_name(
        const std::string &path)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_application_name(path, token);
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_next_application_name(
        const std::string &path,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            ApplicationPropertiesMap::const_iterator app_iter =
                application_properties.find(
                    get_application_name_from_path(path));

            if (app_iter != application_properties.end())
            {
                return app_iter->first;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_application_name",
                "Using the wrong lock token!");
        }

        // Lock error or empty
        return std::string();
    }

    // ----------------------------------------------------------------------
    PropertyEntity::PropertyEntity(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : Entity(id, type, version, instance, restoring)
    {
    }

    // ----------------------------------------------------------------------
    void PropertyEntity::copy_fields(Entity *entity_ptr)
    {
        Entity::copy_fields(entity_ptr);

        PropertyEntity *cast_ptr = 0;

        // Only copy if this is also a PropertyEntity.
        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<PropertyEntity *>(entity_ptr)))
             != 0))
        {
            cast_ptr->application_properties = application_properties;
            cast_ptr->notify_field_changed(ENTITYFIELD_application_properties);
        }
    }

    // ----------------------------------------------------------------------
    bool PropertyEntity::get_application_properties(
        const std::string &full_path,
        ApplicationProperties *&properties,
        std::string &property_path)
    {
        std::string trimmed_path = boost::trim_copy(full_path);

        if (trimmed_path.empty())
        {
            // Empty paths are not valid.
            return false;
        }

        // Remove any prefixed separators, since they are not needed.
        //
        size_t trim_index =
            trimmed_path.find_first_not_of(PATH_SEPARATOR);

        if (trim_index != std::string::npos)
        {
            // Get rid of prefix separator since it's not needed.
            trimmed_path = trimmed_path.substr(trim_index);
        }

        // Find the next separator, if any, and then extract our
        // application name.
        //
        trim_index = trimmed_path.find_first_of(PATH_SEPARATOR);

        if (trim_index == std::string::npos)
        {
            // Not valid since there is no prop path after it.
            return false;
        }

        const std::string application_name =
            trimmed_path.substr(0, trim_index);
        const std::string prop_path =
            (trimmed_path.size() == (trim_index + 1)) ?
                "" : trimmed_path.substr(trim_index + 1);

        if (application_name.empty() or prop_path.empty())
        {
            // Application name or prop path are empty, which is invalid.
            return false;
        }

        ApplicationPropertiesMap::iterator app_iter =
            application_properties.find(application_name);

        if (app_iter == application_properties.end())
        {
            // Could not find the application.
            return false;
        }

        // Looks like the application is valid, and we have a valid path.
        // Return everything to the caller.
        properties = &app_iter->second;
        property_path = prop_path;
        return true;
    }

    // ----------------------------------------------------------------------
    std::string PropertyEntity::get_application_name_from_path(
        const std::string &full_path)
    {
        std::string trimmed_path = boost::trim_copy(full_path);

        if (trimmed_path.empty())
        {
            // Empty paths are not valid.
            return "";
        }

        // Remove any prefixed separators, since they are not needed.
        //
        size_t trim_index =
            trimmed_path.find_first_not_of(PATH_SEPARATOR);

        if (trim_index != std::string::npos)
        {
            // Get rid of prefix separator since it's not needed.
            trimmed_path = trimmed_path.substr(trim_index);
        }

        // Find the next separator, if any, and then extract our
        // application name.
        //
        trim_index = trimmed_path.find_first_of(PATH_SEPARATOR);

        return ((trim_index == std::string::npos) ?
            trimmed_path :
            trimmed_path.substr(0, trim_index));
    }

    // ----------------------------------------------------------------------
    PropertyData *PropertyEntity::get_property_data_ptr(const std::string &path)
    {
        ApplicationProperties *properties_ptr = 0;
        std::string property_path;

        if (get_application_properties(path, properties_ptr, property_path))
        {
            // Found the application.  Now try and get the property data.
            return properties_ptr->get_properties().get_property_data(
                property_path);
        }

        return 0;
    }
} /* namespace dbtype */
} /* namespace mutgos */
