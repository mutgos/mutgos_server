/*
 * dbtype_PropertyEntity.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYENTITY_H_
#define MUTGOS_DBTYPE_PROPERTYENTITY_H_

#include <map>
#include <string>
#include <set>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/thread/recursive_mutex.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_ApplicationProperties.h"
#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"
#include "dbtypes/dbtype_PropertySecurity.h"
#include "dbtypes/dbtype_Id.h"

#include "osinterface/osinterface_OsTypes.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

// TODO Fix the 'owner' thing for applications.  Has no way to reference who owns what, or change owner even if authorized.

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a PropertyEntity database type, which is an Entity that
     * has application properties.
     *
     * The write token is used even while reading property data because
     * properties have a cache for last used entry, as it's common for
     * the same directory to be accessed repeatedly.  Because the cache
     * is updated, it's considered a write.  One day this might be
     * improved.
     *
     * The property path refers to a path as defined by PropertyDirectory.
     * An Application Property is the combination of the application name and
     * property path.  For example: /MyApp/dirA/dirB/prop
     * @see PropertyDirectory
     */
    class PropertyEntity : public Entity
    {
    public:
        /** First is the ID of the application owner, second is the security
            for the application */
        typedef std::pair<Id, PropertySecurity> ApplicationOwnerSecurity;

        /**
         * Constructor used for deserialization of a PropertyEntity.
         */
        PropertyEntity();

        /**
         * Constructs a PropertyEntity (final type).
         * @param id[in] The ID of the entity.
         */
        PropertyEntity(const Id &id);

        /**
         * Destructor.
         */
        virtual ~PropertyEntity();

        /**
         * Given a full path, return the application name portion.
         * @param full_path[in] The full application property path.
         * @return The application name contained within the path, or empty
         * string if invalid.
         */
        static std::string get_application_name_from_path(
            const std::string &full_path);

        /**
         * Creates a copy of this PropertyEntity.
         * @param id[in] The new ID of the cloned PropertyEntity.
         * @param version[in] The new version # of this PropertyEntity.
         * @param instance[in] The new instance # of this PropertyEntity.
         * @param token[in] The lock token.
         * @return The clone as a pointer.  Caller must manage the pointer!
         * Null is returned if there is an error, such as an incorrect
         * lock token.
         */
        virtual Entity *clone(
            const Id &id,
            const VersionType version,
            const InstanceType instance,
            concurrency::ReaderLockToken &token);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);


        /**
         * This method will automatically get a lock.
         * @param path[in] The application to check.  This may be a full path.
         * @return True if the application exists.
         */
        bool application_exists(const std::string &path);

        /**
         * @param path[in] The application to check.  This may be a full path.
         * @param token[in] The lock token.
         * @return True if the application exists.
         */
        bool application_exists(
            const std::string &path,
            concurrency::ReaderLockToken &token);


        /**
         * Adds the given application to this PropertyEntity (locking).
         * This method will automatically get a lock.
         * @param path[in] The application to add.  This may be a full path.
         * @param owner[in] The owner of the application properties.
         * @param security[in] The initial security settings.
         * @return True if successfully added, false if not (if it already
         * exists, for instance).
         */
        bool add_application(
            const std::string &path,
            const Id &owner,
            const PropertySecurity &security);

        /**
         * Adds the given application to this PropertyEntity.
         * @param path[in] The application to add.  This may be a full path.
         * @param owner[in] The owner of the application properties.
         * @param security[in] The initial security settings.
         * @param token[in] The lock token.
         * @return True if successfully added, false if not (if it already
         * exists, for instance).
         */
        bool add_application(
            const std::string &path,
            const Id &owner,
            const PropertySecurity &security,
            concurrency::WriterLockToken &token);


        /**
         * Erases the application from this PropertyEntity (locking).
         * This method will automatically get a lock.
         * @param path[in] The application to remove.  This may be a full path.
         */
        void remove_application(
            const std::string &path);

        /**
         * Erases the application from this PropertyEntity.
         * @param path[in] The application to remove.  This may be a full path.
         * @param token[in] The lock token.
         */
        void remove_application(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Get a copy of the security settings for the given application
         * (locking).
         * This method will automatically get a lock.
         * @param path[in] The application to get the security settings for.
         * This may be a full path.
         * @return A copy of the security settings, or a default if the
         * application no longer exists.
         */
        ApplicationOwnerSecurity get_application_security_settings(
            const std::string &path);

        /**
         * Get a copy of the security settings for the given application.
         * @param path[in] The application to get the security settings for.
         * This may be a full path.
         * @param token[in] The lock token.
         * @return A copy of the security settings, or a default if the
         * application no longer exists or error.
         */
        ApplicationOwnerSecurity get_application_security_settings(
            const std::string &path,
            concurrency::ReaderLockToken &token);


        /**
         * Sets the given security settings on the given application (locking).
         * This method will automatically get a lock.
         * @param path[in] The application to set the security settings for.
         * This may be a full path.
         * @param security[in] The security settings to set.
         * @return True if success (application exists).
         */
        bool set_application_security_settings(
            const std::string &path,
            const PropertySecurity &security);

        /**
         * Sets the given security settings on the given application.
         * @param path[in] The application to set the security settings for.
         * This may be a full path.
         * @param security[in] The security settings to set.
         * @param token[in] The lock token.
         * @return True if success (application exists).
         */
        bool set_application_security_settings(
            const std::string &path,
            const PropertySecurity &security,
            concurrency::WriterLockToken &token);


        /**
         * Gets the property, given the full path (including application name)
         * (locking).
         * The pointer returned is OWNED BY THE CALLER and is a COPY of the
         * data.  The caller MUST delete the pointer when it is done with the
         * data.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to get.
         * @return A copy of the propety data (caller owned!), or null if
         * property not found or has no data.
         */
        PropertyData *get_property(
            const std::string &path);

        /**
         * Gets the property, given the full path (including application name).
         * The pointer returned is OWNED BY THE CALLER and is a COPY of the
         * data.  The caller MUST delete the pointer when it is done with the
         * data.
         * @param path[in] The path of the application property data to get.
         * @param token[in] The lock token.
         * @return A copy of the property data (caller owned!), or null if
         * property not found or has no data.
         */
        PropertyData *get_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Gets the property as a string, given the full path (including
         * application name) (locking).  If the data type is not a string, it
         * will attempt to be converted to a string using the to_string()
         * call.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to get as
         * a string.
         * @return The application property as a string, or empty if not found
         * or unable to convert.
         */
        std::string get_string_property(
            const std::string &path);

        /**
         * Gets the property as a string, given the full path (including
         * application name).  If the data type is not a string, it
         * will attempt to be converted to a string using the to_string()
         * call.
         * @param path[in] The path of the application property data to get as
         * a string.
         * @param token[in] The lock token.
         * @return The application property as a string, or empty if not found
         * or unable to convert.
         */
        std::string get_string_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Gets the property as an int, given the full path (including
         * application name) (locking).  If the data type is another numerical
         * type, it will attempt to be converted to an int.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to get as
         * an int.
         * @return The application property as an int, or 0 if not found
         * or unable to convert.
         */
        MG_SignedInt get_int_property(
            const std::string &path);

        /**
         * Gets the property as an int, given the full path (including
         * application name).  If the data type is not an int, it
         * will attempt to be converted to an int.
         * @param path[in] The path of the application property data to get as
         * an int.
         * @param token[in] The lock token.
         * @return The application property as an int, or 0 if not found
         * or unable to convert.
         */
        MG_SignedInt get_int_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Gets the property as a bool, given the full path (including
         * application name) (locking).  If the data type is not a bool, it
         * will attempt to be converted to a bool.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to get as
         * a bool.
         * @return The application property as a bool, or false if not found
         * or unable to convert.
         */
        bool get_bool_property(
            const std::string &path);

        /**
         * Gets the property as a bool, given the full path (including
         * application name).  If the data type is not a bool, it
         * will attempt to be converted to a bool.
         * @param path[in] The path of the application property data to get as
         * a bool.
         * @param token[in] The lock token.
         * @return The application property as a bool, or false if not found
         * or unable to convert.
         */
        bool get_bool_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Makes a copy of the provided data and sets it on the given property.
         * As long as the application name exists, this should succeed -
         * directories are made as needed.  If the property exists, it will
         * be overwritten (locking).
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to set.
         * @param data[in] The data to set.  A copy will be made.
         * @return True if success.
         */
        bool set_property(
            const std::string &path,
            const PropertyData &data);

        /**
         * Makes a copy of the provided data and sets it on the given property.
         * As long as the application name exists, this should succeed -
         * directories are made as needed.  If the property exists, it will
         * be overwritten.
         * @param path[in] The path of the application property data to set.
         * @param data[in] The data to set.  A copy will be made.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_property(
            const std::string &path,
            const PropertyData &data,
            concurrency::WriterLockToken &token);


        /**
         * Returns the full path for the next entry in the deepest
         * directory (locking).  This allows "walking" a dirctory.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property to iterate.
         * @return The full application path to the next entry, or empty string
         * if not found or at the end.
         */
        std::string get_next_property(const std::string &path);

        /**
         * Returns the full path for the next entry in the deepest
         * directory.  This allows "walking" a dirctory.
         * @param path[in] The path of the application property to iterate.
         * @param token[in] The lock token.
         * @return The full application path to the next entry, or empty string
         * if not found or at the end.
         */
        std::string get_next_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Returns the full path for the previous entry in the deepest
         * directory (locking).  This allows "walking" a dirctory.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property to iterate.
         * @return The full application path to the previous entry, or null
         * if not found or at the beginning.
         */
        std::string get_previous_property(const std::string &path);

        /**
         * Returns the full path for the previous entry in the deepest
         * directory.  This allows "walking" a dirctory.
         * @param path[in] The path of the application property to iterate.
         * @param token[in] The lock token.
         * @return The full application path to the previous entry, or null
         * if not found or at the end.
         */
        std::string get_previous_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Returns the first property within the given directory (locking).
         * This method will automatically get a lock.
         * @param path[in] The path of the application property directory to get
         * the first subproperty of.
         * @return The full first application property inside of the
         * given application property directory, or empty if no subproperties
         * or not found.
         */
        std::string get_first_property(const std::string &path);

        /**
         * Returns the first property within the given directory.
         * @param path[in] The path of the application property directory to get
         * the first subproperty of.
         * @param token[in] The lock token.
         * @return The full first application property inside of the
         * given application property directory, or empty if no subproperties
         * or not found.
         */
        std::string get_first_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Returns the last property within the given directory (locking).
         * This method will automatically get a lock.
         * @param path[in] The path of the application property directory to get
         * the last subproperty of.
         * @return The full last application property inside of the
         * given application property directory, or empty if no subproperties
         * or not found.
         */
        std::string get_last_property(const std::string &path);

        /**
         * Returns the last property within the given directory.
         * @param path[in] The path of the application property directory to get
         * the last subproperty of.
         * @param token[in] The lock token.
         * @return The full last application property inside of the
         * given application property directory, or empty if no subproperties
         * or not found.
         */
        std::string get_last_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Deletes the application property data and associated entry (locking).
         * If the entry is a directory, all properties within it will also
         * be deleted.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property to delete.
         */
        void delete_property(const std::string &path);

        /**
         * Deletes the application property data and associated entry.
         * If the entry is a directory, all properties within it will also
         * be deleted.
         * @param path[in] The path of the application property to delete.
         * @param token[in] The lock token.
         */
        void delete_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Deletes the application property data (locking).  If the application
         * property is not a directory, the entry is also deleted.
         * If the entry is a directory, only the data will be deleted, and not
         * the entry.
         * This method will automatically get a lock.
         * @param path[in] The path of the application property data to delete.
         */
        void delete_property_data(const std::string &path);

        /**
         * Deletes the application property data.  If the application
         * property is not a directory, the entry is also deleted.
         * If the entry is a directory, only the data will be deleted, and not
         * the entry.
         * @param path[in] The path of the application property data to delete.
         * @param token[in] The lock token.
         */
        void delete_property_data(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * @param path[in] An application property path (locking).
         * This method will automatically get a lock.
         * @return True if the path has data.  False if no data or it doesn't
         * exist.  This is used primarily with directories.
         */
        bool property_has_data(const std::string &path);

        /**
         * @param path[in] An application property path.
         * @param token[in] The lock token.
         * @return True if the path has data.  False if no data or it doesn't
         * exist.  This is used primarily with directories.
         */
        bool property_has_data(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Determines if the given application path is a valid property or
         * property directory (locking).
         * This method will automatically get a lock.
         * @param path[in] The application property to check.
         * @return True if a valid application property.
         */
        bool is_property(const std::string &path);

        /**
         * Determines if the given application path is a valid property or
         * property directory.
         * @param path[in] The application property to check.
         * @param token[in] The lock token.
         * @return True if a valid application property.
         */
        bool is_property(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Determines the property type.
         * This method will automatically get a lock.
         * @param path[in] The property to get the type of.
         * @return The type of property, or 'invalid' if the property doesn't
         * exist.  If this is a directory with no data, it will return
         * invalid.
         */
        PropertyDataType get_property_type(const std::string &path);

        /**
         * Determines the property type.
         * This method will automatically get a lock.
         * @param path[in] The property to get the type of.
         * @param token[in] The lock token.
         * @return The type of property, or 'invalid' if the property doesn't
         * exist.  If this is a directory with no data, it will return
         * invalid.
         */
        PropertyDataType get_property_type(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Determines if the given application path is a valid property
         * directory (locking).
         * This method will automatically get a lock.
         * @param path[in] The application property to check.
         * @return True if a valid application property directory.
         */
        bool is_property_directory(const std::string &path);

        /**
         * Determines if the given application path is a valid property
         * directory.
         * @param path[in] The application property to check.
         * @param token[in] The lock token.
         * @return True if a valid application property directory.
         */
        bool is_property_directory(
            const std::string &path,
            concurrency::WriterLockToken &token);


        /**
         * Clears all properties from the given application (locking).
         * This method will automatically get a lock.
         * @param path[in] The application to clear.
         */
        void clear(const std::string &path);

        /**
         * Clears all properties from the given application.
         * @param path[in] The application to clear.
         * @param token[in] The lock token.
         */
        void clear(
            const std::string &path,
            concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The first application name contained by this
         * PropertyEntity.  Valid for use as a path.
         */
        std::string get_first_application_name(void);

        /**
         * @param token[in] The lock token.
         * @return The first application name contained by this
         * PropertyEntity. Valid for use as a path.
         */
        std::string get_first_application_name(
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param path[in] The application name to iterate.
         * @return The application name which comes after path, contained by
         * this PropertyEntity.  If the application name is not found or is at
         * the end, an empty string is returned.
         */
        std::string get_next_application_name(const std::string &path);

        /**
         * @param path[in] The application name to iterate.
         * @param token[in] The lock token.
         * @return The application name which comes after path, contained by
         * this PropertyEntity.  If the application name is not found or is at
         * the end, an empty string is returned.
         */
        std::string get_next_application_name(
            const std::string &path,
            concurrency::ReaderLockToken &token);

    protected:
        /**
         * Constructs an Entity with a provided type.  Used by subclasses.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        PropertyEntity(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error.
         */
        virtual size_t mem_used_fields(void);

        /**
         * Copies fields from this PropertyEntity to the provided Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:
        /**
         * Helper to get the application properties and a property path
         * suitable for use with it.
         * @param full_path[in] The full path of the properties to get,
         * including the application name.
         * @param properties[out] A pointer to the ApplicationProperties
         * contained within the path.
         * @param path[out] The path to the property within.  Does not
         * include the application name.
         * @return True if application found, false if not. If false, the
         * outgoing arguments will NOT be populated.
         */
        bool get_application_properties(
            const std::string &full_path,
            ApplicationProperties *&properties,
            std::string &property_path);

        /**
         * Given a full path, return the application data property, if any.
         * @param path[in] The full application property path.
         * @return The actual pointer to the application data, or null if not
         * found.  Do not delete this pointer!
         */
        PropertyData *get_property_data_ptr(const std::string &path);

        /** Maps application name to its properties. */
        typedef std::map<std::string, ApplicationProperties>
          ApplicationPropertiesMap;

        ApplicationPropertiesMap application_properties; ///< App properties

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<Entity>(*this);

            ar & application_properties;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<Entity>(*this);

            ar & application_properties;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_PROPERTYENTITY_H_ */
