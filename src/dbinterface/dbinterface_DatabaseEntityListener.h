/*
 * dbinterface_DatabaseEntityListener.h
 */

#ifndef MUTGOS_DBINTERFACE_DATABASEENTITYLISTENER_H
#define MUTGOS_DBINTERFACE_DATABASEENTITYLISTENER_H

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

namespace mutgos
{
namespace dbinterface
{
    /**
     * Other classes implement this pure virtual (interface) class to be
     * notified when the database subsystem performs certain operations
     * related to Entities.
     *
     * Currently, all operations related to Entities are not combined on this
     * one listener; for updates to an existing Entity you must also implement
     * and use the DatabaseEntityChangeListener.  In the future, this will
     * hopefully be fixed.
     *
     * @see dbinterface::DatabaseEntityChangeListener
     */
    class DatabaseEntityListener
    {
    public:
        /**
         * Default constructor.
         */
        DatabaseEntityListener(void)
          { }

        /**
         * Destructor.
         */
        virtual ~DatabaseEntityListener()
          { }

        /**
         * Called at the moment when an Entity has been created.
         * @param entity_ptr[in] The pointer to the newly created
         * Entity.  Please note it will have only the bare minimum
         * filled in (ID, type, etc).  Do not delete or save this pointer.
         */
        virtual void entity_created(dbtype::Entity *entity_ptr) =0;

        /**
         * Called when an Entity has been requested to be deleted.
         * @param entity_ptr[in] The pointer to the soon-to-be-deleted
         * Entity.  Do not delete or save this pointer.
         */
        virtual void entity_deleted(dbtype::Entity *entity_ptr) =0;

        // TODO need site_created()

        /**
         * Called when a site has been requested to be deleted.
         * @param site_id[in] The ID of the soon-to-be-deleted site.
         */
        virtual void site_deleted(const dbtype::Id::SiteIdType site_id) =0;
    };
}
}

#endif //MUTGOS_DBINTERFACE_DATABASEENTITYLISTENER_H
