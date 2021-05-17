#ifndef MUTGOS_DBINTERFACE_UPDATE_MANAGER_H
#define MUTGOS_DBINTERFACE_UPDATE_MANAGER_H

#include <map>
#include <list>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/atomic/atomic.hpp>

#include "osinterface/osinterface_TimeJumpListener.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_DatabaseEntityChangeListener.h"
#include "dbinterface_EntityRef.h"
#include "dbtypes/dbtype_EntityField.h"

namespace mutgos
{
namespace dbinterface
{
    // TODO Might need a primitive to allow programs/commands to wait until
    //   next commit cycle has finished. -- Need to find a way to avoid this!!

    /**
     * Basically, this listens to all Entity updates and deletes, and commits
     * them to the database.  In the case of deletes, it will also delete
     * anything contained by the Entity.  The program that issues the delete
     * is responsible for moving anything out that shouldn't be deleted.
     *
     * Pending operations are queued for periodic processing on a thread.
     * In the future this could allow for some sort of transaction-based updates
     * to preserve integrity.
     */
    class UpdateManager : public dbtype::DatabaseEntityChangeListener,
                                 osinterface::TimeJumpListener
    {
    public:
        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static UpdateManager *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static UpdateManager *instance()
          { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Used by Boost threads to start our threaded code.
         */
        void operator()();

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up.
         */
        void startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         */
        void shutdown(void);

        /**
         * Called when a massive (more than a few seconds) system time jump has
         * been detected.
         * @param backwards[in] True if the jump was backwards.
         */
        virtual void os_time_has_jumped(bool backwards);

        /**
         * Called when the provided entity has changed in some way.
         * Each attribute changed on an entity will cause this to be called,
         * however several changes may be in a single call.
         * @param entity[in] The entity that has changed.
         * @param fields_changed[in] The fields that have changed.
         * @param flags_changed[in] Detailed information on what flags have
         * changed.
         * @param ids_changed[in] Detailed information about changes concerning
         * fields of type ID (or lists of IDs).
         */
        virtual void entity_changed(
            dbtype::Entity *entity,
            const dbtype::Entity::EntityFieldSet &fields_changed,
            const dbtype::Entity::FlagsRemovedAdded &flags_changed,
            const dbtype::Entity::ChangedIdFieldsMap &ids_changed);

        /**
         * Provides a chance to veto a change to a program's registration
         * name.
         * @param entity[in] The Program Entity that will be changed.  This
         * will be in a write lock while this method is called.
         * @param token[in] The write token for the entity, in case
         * other attributes need to be read.
         * @param old_name[in] The old registration name, or empty for none.
         * @param new_name[in] The new registration name, or empty for none.
         * @return True if registration name change allowed, false to
         * veto (disallow) it.  If true is returned, the change will be
         * made.  If false is returned, the change will not be made.
         */
        virtual bool check_program_registration_name(
            dbtype::Entity *entity,
            concurrency::WriterLockToken &token,
            const std::string &old_name,
            const std::string &new_name);

        /**
         * Given a site and registration name, see if any programs whose
         * registration is being renamed match.
         * @param site_id[in] The site ID to check.
         * @param reg_name[in] The program registration name to look for.
         * @return The ID of the program being renamed whose registration name
         * matches, or default/invalid if none match.
         */
        dbtype::Id get_prog_reg_rename_id(
            const dbtype::Id::SiteIdType site_id,
            const std::string &reg_name);

        /**
         * Adds the given Entity IDs to the list of pending database deletes.
         * The Entities must already be marked as deleted and have all
         * pre-processing done (which at this point is to mark all Entities
         * in the set as deleted and to include all Entities contained by each
         * Entity in this set as well).
         * At a later point in time, each Entity will automatically have
         * references removed and will be deleted from the database.
         * @param entities[in] The IDs of the Entitys to delete.
         */
        void entities_deleted(const dbtype::Entity::IdSet &entities);

        /**
         * Used to determine if an Entity is in the process of being deleted.
         * If Entity is being deleted, do not try and bring it back into
         * the database cache, or deletion will fail!  Other database areas
         * can use this method before trying to retrieve an Entity to make sure
         * it's 'safe'.
         * @param entity_id[in] The ID of the Entity to check.
         * @return True if Entity is in the process of being deleted.
         */
        bool is_entity_delete_pending(const dbtype::Id &entity_id);

        /**
         * Adds the given site ID to the list of pending database delete
         * commits.  The site must already have been marked as delete pending.
         * At a later point in time, the site will be automatically deleted from
         * the database.
         * @param site_id[in] The site to delete.
         */
        void site_deleted(const dbtype::Id::SiteIdType site_id);

        /**
         * Main loop of UpdateManager thread.
         */
        void thread_main(void);

    private:

        typedef std::list<dbtype::Id> IdList;

        /**
         * Container class to hold all the updates pending for a given
         * Entity.
         */
        class EntityUpdate
        {
        public:
            /**
             * Container constructor.
             * @param entity The Entity the update is for.
             */
            EntityUpdate(dbtype::Entity *entity);

            /**
             * Takes the provided changes and merges them into what's
             * already in this instance.
             * @param fields[in] The fields that have changed.
             * @param flags[in] Detailed information on what flags have changed.
             * @param ids[in] Detailed information about changes
             * concerning fields of type ID (or lists of IDs).
             */
            void merge_update(
                const dbtype::Entity::EntityFieldSet &fields,
                const dbtype::Entity::FlagsRemovedAdded &flags,
                const dbtype::Entity::ChangedIdFieldsMap &ids);

            const dbtype::Id entity_id; ///< ID of the Entity being updated
            dbtype::Entity::EntityFieldSet fields_changed; ///< Changed fields
            dbtype::Entity::FlagsRemovedAdded flags_changed; ///< Changed flags
            dbtype::Entity::ChangedIdFieldsMap ids_changed; ///< ID Sets changed
        };

        typedef std::map<dbtype::Id, EntityUpdate *> PendingUpdatesMap;

        void process_immediate_updates(void);

        /**
         * Called by the update thread, this will commit any changed
         * objects.
         */
        void process_db_commits(void);

        /**
         * Given the IDs added and removed on a given Entity, update the
         * Entities it is referencing with a back-reference.
         * The lock mutex is assumed to be UNLOCKED.
         * @param id[in] The Entity whose fields are being updated.
         * @param changed_fields[in] The fields which have IDs being added
         * and/or removed.
         */
        void process_id_references(
            const dbtype::Id &id,
            const dbtype::Entity::ChangedIdFieldsMap changed_fields);

        /**
         * Removes all references to the provided Entity.
         * @param entity[in] The Entity to remove all references to.
         */
        void remove_all_references(EntityRef &entity);

        /**
         * Given a valid target and source, remove the target from the source's
         * field.
         * @param target[in] The target to be removed from the source's field.
         * @param source[in,out] The source to remove the target from.
         * @param field[in] The field to remove the target from.
         */
        void remove_reference_from_source(
            const dbtype::Id &target,
            EntityRef &source,
            const dbtype::EntityField field);

        /**
         * Given a source Entity who has a field that references a target
         * entity, remove the back-reference from the target.
         * @param source[in] The source Entity (making the reference).
         * @param target[in] The target Entity (has a back-reference).
         * @param field[in] The field which has the reference.
         */
        void remove_reference(
            const dbtype::Id &source,
            const dbtype::Id &target,
            const dbtype::EntityField field);

        /**
         * Assumes locking has been done.
         * @param site_id[in] The site ID to check.
         * @param reg_name[in] The reg name to check.
         * @return True if program registration name is in the pending
         * rename list (either current name or previous name), false
         * if not.
         */
        bool is_prog_reg_name_in_progress(
            const dbtype::Id::SiteIdType site_id,
            const std::string &reg_name) const;

        /**
         * Assumes locking has been done.
         * Will determine if an Entity is a program and if its
         * program registration name matches whats in the DB.
         * If it does match, it will be taken out of the pending renames
         * data structures since the rename completed.
         * @param entity_id[in] The ID of the updated Entity to process.
         *
         */
        void process_prog_reg_rename_update(
            const dbtype::Id &entity_id);

        /**
         * Singleton constructor.
         */
        UpdateManager(void);

        /**
         * Singleton destructor.
         */
        virtual ~UpdateManager();

        static UpdateManager *singleton_ptr; ///< Singleton pointer.

        typedef std::vector<EntityUpdate *> ImmediateUpdateQueue;
        typedef std::pair<std::string, std::string> OldNewProgRegName;
        typedef std::map<dbtype::Id::EntityIdType, OldNewProgRegName> IdRegInfo;
        typedef std::map<dbtype::Id::SiteIdType , IdRegInfo> PendingProgReg;

        boost::mutex mutex; ///< Enforces single access at a time.
        boost::thread *thread_ptr; ///< Non-null when thread is running.

        PendingUpdatesMap pending_updates; ///< Updates to be committed
        ImmediateUpdateQueue immediate_update_queue; ///< Updates to be processed immediately
        PendingProgReg pending_program_registrations; ///< Program registrations about to be committed.

        /** Semaphore associated with the immediate update queue so thread can
            easily block and wait for work. */
        boost::interprocess::interprocess_semaphore immediate_update_queue_sem;

        dbtype::Entity::IdSet pending_deletes; ///< Deletes to be committed
        dbtype::Id::SiteIdVector pending_site_deletes; ///< Pending site deletes
        boost::atomic<bool> shutdown_thread_flag; ///< True if thread should shutdown
    };

}
}

#endif //MUTGOS_DBINTERFACE_UPDATE_MANAGER_H
