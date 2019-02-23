#ifndef MUTGOS_DBINTERFACE_CACHEDENTITY_H
#define MUTGOS_DBINTERFACE_CACHEDENTITY_H

#include <boost/thread/mutex.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbinterface_EntityRef.h"
#include "dbinterface_EntityRefCounter.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * Contains metadata and the pointer to an Entity being cached.
     * Also used to help determine if an Entity is memory is still being
     * referenced.
     */
    class CachedEntity : public EntityRefCounter
    {
    public:
        /**
         * Creates a CachedEntity with the provided Entity.
         * @param entity[in] The Entity to represent in this cache.  Must never
         * be null!
         */
        CachedEntity(dbtype::Entity *entity);

        /**
         * Destructor.
         */
        virtual ~CachedEntity();

        /**
         * For internal database interface use only.
         * @return The pointer of the Entity in this CachedEntity.
         */
        dbtype::Entity *get_entity(void)
            { return entity_ptr; }

        /**
         * Indicates a new in-memory reference to the Entity.
         * @param entity[in] The Entity.
         */
        virtual void mem_reference_added(const dbtype::Entity *entity);

        /**
         * Indicates an in-memory reference to the Entity has been removed.
         * @param entity[in] The Entity.
         */
        virtual void mem_reference_removed(const dbtype::Entity *entity);

        /**
         * Sets the provided EntityRef to refer to the Entity contained by
         * this class.  Also sets the ref counter callback.
         * @param ref[out] The EntityRef to update.
         */
        void get_reference(EntityRef &ref);

        /**
         * @return True if something is referencing the Entity.
         */
        bool is_referenced(void);

        /**
         * @return The ID of the cached Entity.
         */
        const dbtype::Id &get_id(void) const;

    private:
        boost::mutex mutex; ///< Enforces single access at a time.
        unsigned int ref_count; ///< How many references to Entity exist
        dbtype::Entity *entity_ptr; ///< Pointer to cached Entity.

        // No copying
        //
        CachedEntity(const CachedEntity &rhs);
        CachedEntity &operator=(const CachedEntity &rhs);
    };
}
}

#endif //MUTGOS_DBINTERFACE_CACHEDENTITY_H
