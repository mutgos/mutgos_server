#ifndef MUTGOS_DBINTERFACE_ENTITYREF_H
#define MUTGOS_DBINTERFACE_ENTITYREF_H

#include "dbtypes/dbtype_Entity.h"

#include "dbinterface_EntityRefCounter.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * ### ALL DEVELOPERS MUST READ ###
     *
     * Instance of this class are returned by the database interface in response
     * to requesting an Entity.  The instance contains a reference to the
     * Entity requested.
     *
     * This class is needed because the database needs to know how many active
     * references to an Entity exist in memory.  When nothing is referencing
     * the Entity, the database manager could then decide to deallocate it from
     * memory to save space.
     *
     * It is EXTREMELY IMPORTANT that you never 'lose control'
     * of the Entity pointer contained in this class.  Never attempt to delete
     * the Entity pointer!  If other classes, threads, or methods require
     * the Entity for processing, it is safer to pass them this EntityRef
     * instead of the Entity pointer.  It is highly discouraged to pass
     * around the Entity pointer for any reason.
     *
     * If you 'lose control' of the Entity pointer and the reference count
     * becomes 0, the Entity may be deleted out from under you, causing a
     * crash that will be difficult to reproduce or track down.
     *
     * It is safe to delete an instance of an EntityRef - it will not
     * delete the Entity.  You should avoid allocating this on the heap because
     * then you'd have to manage it.  A mistake in doing so would cause an
     * Entity to stay in memory forever.  Rather, it is preferred to leave it
     * on the stack (local variables) only.  That way, when you're done using it,
     * it will automatically be released.
     *
     * In short:  This is a reference counter for Entity instances.
     */
    class EntityRef
    {
    public:
        /**
         * Makes an invalid EntityRef.
         */
        EntityRef(void);

        /**
         * Makes a valid EntityRef with the given Entity.
         * @param entity[in] The Entity instance contained by this reference.
         * @param ref_counter[in] The reference counter associated with
         * this Entity.
         */
        EntityRef(dbtype::Entity *entity, EntityRefCounter *ref_counter);

        /**
         * Copy constructor.  Correctly increases the reference count.
         * @param rhs[in] The EntityRef to copy from.
         */
        EntityRef(const EntityRef &rhs);

        /**
         * Destructor.  Releases a reference count.
         */
        ~EntityRef();

        /**
         * Used by the database subsystem to change what this EntityRef points
         * to.  This is not used by anyone outside the dbinterface namespace.
         * @param entity[in] The new Entity pointer.
         * @param ref_counter[in] The reference counter callback.
         */
        void set_reference(
            dbtype::Entity *entity,
            EntityRefCounter *ref_counter);

        /**
         * @return True if this EntityRef has a valid Entity pointer.  If
         * this returns false, do not use any other method.
         */
        bool valid(void) const
            { return entity_ptr; }

        /**
         * Nulls the Entity reference and releases the reference count.
         * After this call completes, valid() will return false.
         */
        void clear(void);

        /**
         * @return A pointer to the Entity.  See class documentation for details
         * on how to use this.
         */
        dbtype::Entity *get(void) const
            { return entity_ptr; }

        /**
         * @return A pointer to the Entity.  See class documentation for details
         * on how to use this.
         */
        dbtype::Entity *operator->(void) const
            { return entity_ptr; }

        /**
         * @return The Entity type.
         */
        const dbtype::EntityType type(void) const
            { return entity_ptr->get_entity_type(); }

        /**
         * @return The Entity ID.
         */
        const dbtype::Id &id(void) const
            { return entity_ptr->get_entity_id(); }

        /**
         * @return True if Entity is scheduled to be deleted.
         */
        bool is_delete_pending(void)
            { return entity_ptr->get_deleted_flag(); }

        /**
         * Copies an EntityRef on top of this one, managing the reference count.
         * @param rhs[in] The source to copy from.
         * @return This.
         */
        EntityRef &operator=(const EntityRef &rhs);

        /**
         * @param rhs[in] The EntityRef to compare against.
         * @return True if the two EntityRefs refer to the same Entity.
         */
        bool operator==(const EntityRef &rhs) const;

        /**
         * @param rhs[in] The EntityRef to compare against.
         * @return True if the two EntityRefs do not refer to the same Entity.
         */
        bool operator!=(const EntityRef &rhs) const;

    private:
        dbtype::Entity *entity_ptr; ///< Pointer to entity we contain
        EntityRefCounter *ref_counter_ptr; ///< Ref counter callback

        /**
         * Safely calls the ref counter implementation to indicate the
         * Entity is currently no longer being referenced.
         */
        void release_ref(void);

        /**
         * Safely calls the ref counter implementation to indicate the
         * Entity is now being referenced.
         */
        void hold_ref(void);

        // Discourage getting pointer to this, to try and avoid any
        // bad programming practices.
        //
        EntityRef *operator&();
    };
}
}
#endif //MUTGOS_DBINTERFACE_ENTITYREF_H
