/*
 * angelscript_AEntity.h
 */

#ifndef MUTGOS_ANGELSCRIPT_AENTITY_H
#define MUTGOS_ANGELSCRIPT_AENTITY_H

#include <angelscript.h>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"

#include "angelscript_SimpleGCObject.h"
#include "angelscript_AString.h"
#include "add_on/scriptarray.h"

#include "dbtypes/dbtype_EntityType.h"

namespace mutgos
{
namespace angelscript
{
    /**
     * Represents an 'Entity' and various subclasses in AngelScript.  This is,
     * for now, a test/prototype version and is subject to extensive changes.
     * It does not hold a EntityRef; rather it is just a fancy wrapper around
     * an Entity ID.
     *
     * Most operations you can do to or with an Entity will be found here.
     *
     * Note this class is meant exclusively to interface with AngelScript.
     * It is designed to be a wrapper and not reusable.  When instantiating,
     * always do it on the heap.
     *
     * The methods in thic class will throw exceptions when security or other
     * errors occur.
     */
    class AEntity : public SimpleGCObject
    {
    public:
        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * A factory used to create a new instance of an AEntity, equivalent
         * to the default constructor.  The created AEntity will be considered
         * invalid.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void entity_factory(asIScriptGeneric *gen_ptr);

        /**
         * A factory used to create a copy of an AEntity, equivalent to the
         * copy constructor.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void entity_factory_copy(asIScriptGeneric *gen_ptr);

        /**
         * Constructor that creates an invalid (default) instance.
         * @param engine[in] The pointer to the script engine.
         */
        AEntity(asIScriptEngine *engine);

        /**
         * Constructor that creates an instance with an ID.
         * @param engine[in] The pointer to the script engine.
         * @param id[in] The Entity ID represented by this AEntity.
         */
        AEntity(asIScriptEngine *engine, const dbtype::Id &id);

        /**
         * Destructor.
         */
        virtual ~AEntity();

        /**
         * @param rhs[in] The other Entity to compare.
         * @return True if both Entities are exactly equal.
         */
        bool operator==(const AEntity &rhs) const;

        /**
         * Assignment operator.
         * @param rhs[in] The AEntity to copy from.
         * @return This.
         */
        AEntity &operator=(const AEntity &rhs);

        /**
         * @return True if this Entity is valid (exists in the database).
         */
        bool is_valid(void);

        /**
         * @return The exact type of Entity this instance is.  Will return
         * invalid if Entity is not valid.
         */
        dbtype::EntityType get_type(void);

        // TODO The is_* type methods are for prototype only; something more elegant and reusable must be found

        /**
         * @return True if Entity is any kind of action.
         */
        bool is_action(void);

        /**
         * @return True if Entity is any kind of room.
         */
        bool is_room(void);

        /**
         * @return True if Entity is a Thing.
         */
        bool is_thing(void);

        /**
         * @return True if Entity is a Player.
         */
        bool is_player(void);

        /**
         * @return Detailed information about the Entity.  There are embedded
         * newlines.
         */
        AString *to_string(void);

        /**
         * @return The Entity containing this one, or an exception if
         * this Entity does not support being contained.
         */
        AEntity *get_location(void);

        /**
         * In AngelScript, this looks like: array<Entity> get_contents()
         * @return The contents (including actions) of this Entity.
         */
        CScriptArray *get_contents(void);

        /**
         * @param append_id[in] True to append the ID at the end of the Entity
         * name.
         * @return The name of this Entity with the ID optionally added.
         */
        AString *get_name(const bool append_id);

        /**
         * Deletes this Entity.
         */
        void delete_entity(void);

        /**
         * Sets the read and write flags on the 'other' security field.
         * @param readwrite_flag[in] True to set the read write flags,
         * false to remove them.
         */
        void set_security_other(const bool readwrite_flag);

        /**
         * Adds an admin to the entity's security settings.
         * @param admin_entity[in] The entity to add as an admin.
         */
        void set_security_add_admin(AEntity &admin_entity);

        /**
         * Removes an admin from the entity's security settings.
         * @param admin_entity[in] The entity to remove as an admin.
         */
        void set_security_remove_admin(AEntity &admin_entity);

        /**
         * Sets the write flag on the 'other' security field of an application.
         * @param application[in] The application (may be full path) to change
         * the security settings of.
         * @param write_flag[in] True to set the write flag, false to remove it.
         */
        void set_application_security_other(
            const AString &application,
            const bool write_flag);

        /**
         * Adds an admin to an application's security settings.
         * @param application[in] The application (may be full path) to change
         * the security settings of.
         * @param admin_entity[in] The entity to add as an admin.
         */
        void set_application_security_add_admin(
            const AString &application,
            AEntity &admin_entity);

        /**
         * Removes an admin from an application's security settings.
         * @param application[in] The application (may be full path) to change
         * the security settings of.
         * @param admin_entity[in] The entity to remove as an admin.
         */
        void set_application_security_remove_admin(
            const AString &application,
            AEntity &admin_entity);

        /**
         * Determines if a property currently exists.
         * @param property[in] The full path of the property to check.
         * @return True if the property exists, false if not.
         */
        bool prop_exists(const AString &property);

        /**
         * Determines if a property exists and is a Document.
         * @param property[in] The full path of the property to check.
         * @return True if the property exists and is a Document, false if not.
         */
        bool is_prop_document(const AString &property);

        /**
         * @param property[in] The full path of the int property to retrieve.
         * @return The value of the int property, or 0 if not set or type
         * mismatch.
         */
        MG_SignedInt get_int_prop(const AString &property);

        /**
         * @param property[in] The full path of the string property to retrieve.
         * @return The value of the string property, or an empty string if not
         * set or type mismatch.
         */
        AString *get_string_prop(const AString &property);

        /**
         * This will get and convert any property data type to a string.
         * For Documents, only the first line is returned.
         * @param property[in] The full path of the string property to retrieve.
         * @return The value of the property as a string, or an empty string
         * if not set.
         */
        AString *get_prop_as_string(const AString &property);

        /**
         * In AngelScript, this looks like:
         *    array<string> get_document_prop(const string &in property)
         * @param property[in] The full path of the document property to
         * retrieve.
         * @return The value of the document property as an array, or an empty
         * array if not set or type mismatch.
         */
        CScriptArray *get_document_prop(const AString &property);

        /**
         * Sets an integer prop, overwriting anything currently stored in the
         * property.
         * @param property[in] The full path of the property to set.
         * @param value[in] The new value of the property.
         */
        void set_prop(const AString &property, const MG_SignedInt value);

        /**
         * Sets a string prop, overwriting anything currently stored in the
         * property.
         * @param property[in] The full path of the property to set.
         * @param value[in] The new value of the property.
         */
        void set_prop(const AString &property, const AString &value);

        /**
         * @return The ID of the Entity represented by this AEntity.
         */
        const dbtype::Id &get_id(void) const
        { return entity_id; }

    protected:
        /**
         * Constructor that creates a copy of an existing AEntity.
         * This may go away if AEntity acquires more fields and becomes more
         * complicated to manage in memory.
         * @param rhs[in] The AEntity to copy from.
         */
        AEntity(const AEntity &rhs);


    private:
        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);

        dbtype::Id entity_id; ///< The ID of the Entity being represented by this instance.
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_AENTITY_H
