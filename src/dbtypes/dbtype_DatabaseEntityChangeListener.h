/*
 * dbtype_DatabaseEntityChangeListener.h
 */

#ifndef MUTGOS_DBTYPE_DATABASEENTITYCHANGELISTENER_H_
#define MUTGOS_DBTYPE_DATABASEENTITYCHANGELISTENER_H_

#include <set>
#include <map>

#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_Entity.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Other classes implement this pure virtual (interface) class to be
     * notified when anything changes on an Entity.  Other listeners exist for
     * use by the system in general.
     */
    class DatabaseEntityChangeListener
    {
    public:
        /**
         * Default constructor.
         */
        DatabaseEntityChangeListener(void);

        /**
         * Destructor.
         */
        virtual ~DatabaseEntityChangeListener();

        /**
         * Called when the provided entity has changed in some way.
         * Each attribute changed on an entity will cause this to be called,
         * however several changes may be in a single call.
         * Note that the Entity is still locked when this is being called,
         * meaning getting and setting most attributes on the Entity is
         * impossible; it must instead be scheduled for a later time.
         * @param entity[in] The entity that has changed.
         * @param fields[in] The fields that have changed.
         * @param flags_changed[in] Detailed information on what flags have
         * changed.
         * @param ids_changed[in] Detailed information about changes concerning
         * fields of type ID (or lists of IDs).
         */
        virtual void entity_changed(
            Entity *entity,
            const Entity::EntityFieldSet &fields,
            const Entity::FlagsRemovedAdded &flags_changed,
            const Entity::ChangedIdFieldsMap &ids_changed) =0;
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_DATABASEENTITYCHANGELISTENER_H_ */
