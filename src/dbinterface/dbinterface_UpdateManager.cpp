
#include <list>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <chrono>

#include "dbinterface_UpdateManager.h"

#include "dbinterface_DatabaseAccess.h"
#include "dbinterface_DbBackend.h"
#include "dbinterface_EntityRef.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/microsec_time_clock.hpp>

#include "dbtypes/dbtype_Vehicle.h"
#include "dbtypes/dbtype_Program.h"
#include "dbtypes/dbtype_Player.h"
#include "dbtypes/dbtype_Thing.h"
#include "dbtypes/dbtype_ContainerPropertyEntity.h"
#include "dbtypes/dbtype_ActionEntity.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityField.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_EntityField.h"

#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_ReaderLockToken.h"

#include "logging/log_Logger.h"

// Seconds between commits of changed Entities.  This is approximate
// and may delay up to twice the time.
#define DB_COMMIT_INTERVAL_SECS 5

// The immediate update queue has at least this many elements pre-reserved
#define IMMEDIATE_QUEUE_RESERVE_SIZE 64

namespace mutgos
{
namespace dbinterface
{
    // Statics
    //
    UpdateManager *UpdateManager::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    UpdateManager *UpdateManager::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new UpdateManager();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void UpdateManager::destroy_singleton()
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    void UpdateManager::operator()()
    {
        thread_main();
    }

    // ----------------------------------------------------------------------
    void UpdateManager::startup(void)
    {
        LOG(info, "dbinterface", "startup", "Starting up...");

        if (not thread_ptr)
        {
            thread_ptr = new boost::thread(boost::ref(*this));
            dbtype::Entity::register_change_listener(this);
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::shutdown(void)
    {
        LOG(info, "dbinterface", "shutdown", "Shutting down...");

        if (thread_ptr)
        {
            shutdown_thread_flag.store(true);

            thread_ptr->join();
            delete thread_ptr;
            thread_ptr = 0;

            dbtype::Entity::unregister_change_listener(this);
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::os_time_has_jumped(bool backwards)
    {
        if (backwards)
        {
            // Forward jumping is OK - it means a quicker poll.  Backwards
            // could mean too long a poll.

            // Trigger the semaphore to break it loose.
            immediate_update_queue_sem.post();
        }
    }

    // ----------------------------------------------------------------------
    UpdateManager::EntityUpdate::EntityUpdate(dbtype::Entity *entity)
      : entity_id(entity ? entity->get_entity_id() : dbtype::Id())
    {
    }

    // ----------------------------------------------------------------------
    void UpdateManager::entity_changed(
       dbtype::Entity *entity,
       const dbtype::Entity::EntityFieldSet &fields_changed,
       const dbtype::Entity::FlagsRemovedAdded &flags_changed,
       const dbtype::Entity::ChangedIdFieldsMap &ids_changed)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        // Put the changes into the map.  Another thread will pick them
        // up and process.
        //
        if (entity)
        {
            if (not ids_changed.empty())
            {
                // References changed, so detour to the immediate update
                // queue before committing.
                //
                EntityUpdate * const new_update = new EntityUpdate(entity);

                new_update->fields_changed = fields_changed;
                new_update->flags_changed = flags_changed;
                new_update->ids_changed = ids_changed;

                immediate_update_queue.push_back(new_update);

                if (immediate_update_queue.size() == 1)
                {
                    // First entry.  Post to the semaphore to get everything
                    // picked up.
                    immediate_update_queue_sem.post();
                }
            }
            else
            {
                PendingUpdatesMap::iterator update_iter = pending_updates.find(
                    entity->get_entity_id());

                if (update_iter == pending_updates.end())
                {
                    // Need to create entry, no merge needed.
                    //
                    EntityUpdate * const new_update = new EntityUpdate(entity);

                    new_update->fields_changed = fields_changed;
                    new_update->flags_changed = flags_changed;
                    new_update->ids_changed = ids_changed;

                    pending_updates.insert(std::make_pair(
                        entity->get_entity_id(),
                        new_update));
                }
                else
                {
                    // Existing.  Need to merge.
                    //
                    update_iter->second->merge_update(
                        fields_changed,
                        flags_changed,
                        ids_changed);
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::entities_deleted(const dbtype::Entity::IdSet &entities)
    {
        boost::lock_guard<boost::mutex> guard(mutex);
        pending_deletes.insert(entities.begin(), entities.end());
    }

    // ----------------------------------------------------------------------
    bool UpdateManager::is_entity_delete_pending(const dbtype::Id &entity_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        return (pending_deletes.find(entity_id) != pending_deletes.end());
    }

    // ----------------------------------------------------------------------
    void UpdateManager::site_deleted(const dbtype::Id::SiteIdType site_id)
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        pending_site_deletes.push_back(site_id);
    }

    // ----------------------------------------------------------------------
    void UpdateManager::thread_main(void)
    {
        bool do_shutdown = false;
        std::chrono::steady_clock::time_point last_db_commit_time =
            std::chrono::steady_clock::now();

        // The timing is far from exact, but it will guarantee DB changes
        // will be committed in at most (DB_COMMIT_INTERVAL_SECS * 2) seconds,
        // with the expected time being a lot closer to DB_COMMIT_INTERVAL_SECS.
        //
        // This algorithm can always be made more exact later if timing
        // becomes super important, which it shouldn't need to be.
        //
        while (not do_shutdown)
        {
            // TODO Deleted entities need deleted flag set and saved in case of crash

            // Wait on the immediate update queue
            //
            try
            {
                // Wait a little for semaphore to be posted or a timeout.
                immediate_update_queue_sem.timed_wait(
                    boost::posix_time::microsec_clock::universal_time()
                      + boost::posix_time::seconds(DB_COMMIT_INTERVAL_SECS));
            }
            catch (...)
            {
                LOG(fatal, "dbinterface", "thread_main",
                    "Exception while doing timed_wait() on semaphore!");
            }

            process_immediate_updates();

            if (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - last_db_commit_time).count() >=
                    DB_COMMIT_INTERVAL_SECS)
            {
                // We haven't committed DB changes for a while, so do that now.
                //
                process_db_commits();
                last_db_commit_time = std::chrono::steady_clock::now();
            }

            // Only shutdown if the pending updates are all finished
            //
            {
                boost::lock_guard<boost::mutex> guard(mutex);
                do_shutdown = pending_updates.empty()
                  and pending_deletes.empty()
                  and immediate_update_queue.empty()
                  and shutdown_thread_flag.load();
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::process_immediate_updates(void)
    {
        ImmediateUpdateQueue queue_copy;

        {
            // Grab the stuff to update en masse to avoid locking the data
            // structures for too long.
            //
            boost::lock_guard<boost::mutex> guard(mutex);

            if (not immediate_update_queue.empty())
            {
                queue_copy.reserve(IMMEDIATE_QUEUE_RESERVE_SIZE);
                queue_copy.swap(immediate_update_queue);
            }
        }

        if (not queue_copy.empty())
        {
            // Process the reference changes
            //
            for (ImmediateUpdateQueue::iterator immediate_iter =
                    queue_copy.begin();
                 immediate_iter != queue_copy.end();
                 ++immediate_iter)
            {
                if (not (*immediate_iter)->ids_changed.empty())
                {
                    // References changed. Update them
                    process_id_references(
                        (*immediate_iter)->entity_id,
                        (*immediate_iter)->ids_changed);

                    // Now that we've updated them, take them out of the update
                    // info so we don't do them again.
                    (*immediate_iter)->ids_changed.clear();
                }
            }

            {
                // Move everything processed into the pending update queue so
                // it'll be committed.
                //
                boost::lock_guard<boost::mutex> guard(mutex);

                for (ImmediateUpdateQueue::iterator immediate_iter =
                        queue_copy.begin();
                     immediate_iter != queue_copy.end();
                     ++immediate_iter)
                {
                    PendingUpdatesMap::iterator update_iter =
                        pending_updates.find(
                            (*immediate_iter)->entity_id);

                    if (update_iter == pending_updates.end())
                    {
                        // Add new entry, no merge needed.
                        //
                        pending_updates.insert(std::make_pair(
                            (*immediate_iter)->entity_id,
                            *immediate_iter));
                    }
                    else
                    {
                        // Existing.  Need to merge.
                        //
                        update_iter->second->merge_update(
                            (*immediate_iter)->fields_changed,
                            (*immediate_iter)->flags_changed,
                            (*immediate_iter)->ids_changed);

                        delete *immediate_iter;
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::process_db_commits(void)
    {
        DatabaseAccess * const db = DatabaseAccess::instance();
        PendingUpdatesMap updates_copy;
        dbtype::Entity::IdSet deletes_copy;
        dbtype::Id::SiteIdVector site_deletes_copy;

        {
            // Grab the stuff to update and delete en masse to avoid
            // locking the data structures for too long.
            //
            boost::lock_guard<boost::mutex> guard(mutex);
            updates_copy = pending_updates;
            deletes_copy = pending_deletes;
            site_deletes_copy = pending_site_deletes;

            pending_updates.clear();
            pending_deletes.clear();
            pending_site_deletes.clear();
        }

        for (PendingUpdatesMap::iterator update_iter = updates_copy.begin();
             update_iter != updates_copy.end();
             ++update_iter)
        {
            // References were already updated in process_immediate_updates()

            // Commit the changes to the database.
            //
            EntityRef updated_entity = db->get_entity(update_iter->first);

            if (updated_entity.valid())
            {
                if (not db->internal_commit_entity(updated_entity))
                {
                    LOG(error, "dbinterface", "thread_main",
                        "Could not commit Entity with ID "
                        + update_iter->first.to_string(true)
                        + " to database.");
                }
            }
        }

        // Process deletes.  Remove all references to each Entity being
        // deleted, then attempt to remove it from the database and
        // cache.  If it is not in use this will succeed, otherwise
        // reinsert it into the pending deletes to try again later.
        //
        EntityRef deleted_entity_ref;

        for (dbtype::Entity::IdSet::const_iterator deleted_id_iter =
                 deletes_copy.begin();
             deleted_id_iter != deletes_copy.end();
             ++deleted_id_iter)
        {
            deleted_entity_ref = db->get_entity_deleted(*deleted_id_iter);

            if (deleted_entity_ref.valid())
            {
                remove_all_references(deleted_entity_ref);
            }

            // Scope for clearing dirty info on Entity.  Lock needs to
            // be released to avoid potential crashes while doing the
            // actual delete.
            {
                concurrency::WriterLockToken token(*deleted_entity_ref.get());
                deleted_entity_ref->clear_dirty(token);
            }

            deleted_entity_ref.clear();

            // Remove from pending updates so we don't try and update a
            // deleted Entity.
            //
            {
                // Scoped for mutex
                boost::lock_guard<boost::mutex> guard(mutex);
                pending_updates.erase(*deleted_id_iter);
            }

            // Attempt to do the actual deletion.  If it fails, reinsert
            // to try again later.
            //
            if (db->internal_delete_entity(*deleted_id_iter) ==
                DBRESULTCODE_ERROR_ENTITY_IN_USE)
            {
                boost::lock_guard<boost::mutex> guard(mutex);
                pending_deletes.insert(*deleted_id_iter);
            }
        }

        // Process site deletes.  Simply call DatabaseAccess again.  If it
        // succeeds, then we're done, otherwise the ID has been automatically
        // reinserted into the delete list to try again later.
        //
        for (dbtype::Id::SiteIdVector::iterator site_iter =
            site_deletes_copy.begin();
             site_iter != site_deletes_copy.end();
             ++site_iter)
        {
            db->delete_site(*site_iter);
        }

        // Clear the temporary holds for the next use
        //
        for (PendingUpdatesMap::iterator update_iter = updates_copy.begin();
             update_iter != updates_copy.end();
             ++update_iter)
        {
            delete update_iter->second;
        }

        updates_copy.clear();
        deletes_copy.clear();
        site_deletes_copy.clear();
    }

    // ----------------------------------------------------------------------
    void UpdateManager::process_id_references(
        const dbtype::Id &id,
        const dbtype::Entity::ChangedIdFieldsMap changed_fields)
    {
        DatabaseAccess *db = DatabaseAccess::instance();

        for (dbtype::Entity::ChangedIdFieldsMap::const_iterator field_iter =
                changed_fields.begin();
            field_iter != changed_fields.end();
            ++field_iter)
        {
            for (dbtype::Entity::IdSet::const_iterator removed_iter =
                    field_iter->second.first.begin();
                removed_iter != field_iter->second.first.end();
                ++removed_iter)
            {
                // Process ID removals
                //
                EntityRef entity = db->get_entity_deleted(*removed_iter);

                if (entity.valid())
                {
                    if (not entity.get()->remove_entity_reference(
                        id,
                        field_iter->first))
                    {
                        LOG(error, "dbinterface", "process_id_references",
                            "Could not remove ID " + id.to_string(true)
                            + " reference from "
                            + entity_field_to_string(field_iter->first)
                            + " on " + removed_iter->to_string(true));
                    }
                }
            }

            for (dbtype::Entity::IdSet::const_iterator added_iter =
                    field_iter->second.second.begin();
                 added_iter != field_iter->second.second.end();
                 ++added_iter)
            {
                // Process ID additions
                //
                EntityRef entity = db->get_entity_deleted(*added_iter);

                if (entity.valid())
                {
                    if (not entity.get()->add_entity_reference(
                        id,
                        field_iter->first))
                    {
                        LOG(error, "dbinterface", "process_id_references",
                            "Could not add ID " + id.to_string(true)
                            + " reference to "
                            + entity_field_to_string(field_iter->first)
                            + " on " + added_iter->to_string(true));
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::remove_all_references(EntityRef &entity)
    {
        // TODO Create a new virtual method on Entity that can get a list to avoid all this

        // Try casting the Entity to all the different types it could be
        // that has ID references, and then go to the Entity referenced and
        // remove itself from the other Entity's reference list.
        //

        const dbtype::Id entity_id = entity.id();
        dbtype::Id current_id;

        // Check Entity (owner, security)
        //
        current_id = entity.get()->get_entity_owner();
        remove_reference(entity_id, current_id, dbtype::ENTITYFIELD_owner);

        const dbtype::Security entity_security =
            entity.get()->get_entity_security();
        const dbtype::Security::SecurityIds &security_admin_ids =
            entity_security.get_admin_ids();

        for (dbtype::Security::SecurityIds::const_iterator sec_iter =
                security_admin_ids.begin();
            sec_iter != security_admin_ids.end();
            ++sec_iter)
        {
            remove_reference(entity_id, *sec_iter, dbtype::ENTITYFIELD_security);
        }

        const dbtype::Security::SecurityIds &security_list_ids =
            entity_security.get_list_ids();

        for (dbtype::Security::SecurityIds::const_iterator ids_iter =
            security_list_ids.begin();
             ids_iter != security_list_ids.end();
             ++ids_iter)
        {
            remove_reference(entity_id, *ids_iter, dbtype::ENTITYFIELD_security);
        }

        // Check ContainerPropertyEntity (contained by, programs)
        //
        dbtype::ContainerPropertyEntity *container =
            dynamic_cast<dbtype::ContainerPropertyEntity *>(entity.get());

        if (container)
        {
            current_id = container->get_contained_by();

            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_contained_by);

            const dbtype::Entity::IdVector program_ids =
                container->get_linked_programs();

            for (dbtype::Entity::IdVector::const_iterator iter =
                    program_ids.begin();
                iter != program_ids.end();
                ++iter)
            {
                remove_reference(
                    entity_id,
                    *iter,
                    dbtype::ENTITYFIELD_linked_programs);
            }
        }

        // Check Player (home)
        //
        dbtype::Player *player =
            dynamic_cast<dbtype::Player *>(entity.get());

        if (player)
        {
            current_id = player->get_player_home();

            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_player_home);
        }

        // Check Thing (home)
        //
        dbtype::Thing *thing =
            dynamic_cast<dbtype::Thing *>(entity.get());

        if (thing)
        {
            current_id = thing->get_thing_home();

            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_thing_home);
        }

        // Check Vehicle (interior, controller)
        //
        dbtype::Vehicle *vehicle =
            dynamic_cast<dbtype::Vehicle *>(entity.get());

        if (vehicle)
        {
            current_id = vehicle->get_vehicle_interior();
            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_vehicle_interior);

            current_id = vehicle->get_vehicle_controller();
            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_vehicle_controller);
        }

        // Check Group (members)
        //
        dbtype::Group *group =
            dynamic_cast<dbtype::Group *>(entity.get());

        if (group)
        {
            const dbtype::Entity::IdVector group_members =
                group->get_all_in_group();

            for (dbtype::Entity::IdVector::const_iterator iter =
                    group_members.begin();
                iter != group_members.end();
                ++iter)
            {
                remove_reference(entity_id, *iter, dbtype::ENTITYFIELD_group_ids);
            }
        }

        // Program (code includes)
        //
        dbtype::Program *program = dynamic_cast<dbtype::Program *>(entity.get());

        if (program)
        {
            const dbtype::Entity::IdSet linked_programs =
                program->get_program_includes();

            for (dbtype::Entity::IdSet::const_iterator iter =
                linked_programs.begin();
                 iter != linked_programs.end();
                 ++iter)
            {
                remove_reference(
                    entity_id,
                    *iter,
                    dbtype::ENTITYFIELD_program_includes);
            }
        }

        // ActionEntity (targets, contained by)
        //
        dbtype::ActionEntity *action =
            dynamic_cast<dbtype::ActionEntity *>(entity.get());

        if (action)
        {
            current_id = action->get_action_contained_by();
            remove_reference(
                entity_id,
                current_id,
                dbtype::ENTITYFIELD_action_contained_by);


            const dbtype::Entity::IdVector action_targets =
                action->get_action_targets();

            for (dbtype::Entity::IdVector::const_iterator iter =
                action_targets.begin();
                 iter != action_targets.end();
                 ++iter)
            {
                remove_reference(
                    entity_id,
                    *iter,
                    dbtype::ENTITYFIELD_action_targets);
            }
        }

        // Now, do what we did above in reverse. Basically, if another Entity is
        // referencing this one, that reference needs to be broken.
        //
        const dbtype::Entity::IdFieldsMap references =
            entity.get()->get_all_references();

        for (dbtype::Entity::IdFieldsMap::const_iterator ref_id_iter =
                references.begin();
            ref_id_iter != references.end();
            ++ref_id_iter)
        {
            current_id = ref_id_iter->first;
            EntityRef current_entity = DatabaseAccess::instance()->
                get_entity_deleted(current_id);

            if (entity.valid())
            {
                for (dbtype::Entity::EntityFieldSet::const_iterator field_iter =
                        ref_id_iter->second.begin();
                    field_iter != ref_id_iter->second.end();
                    ++field_iter)
                {
                    remove_reference_from_source(
                        entity_id,
                        current_entity,
                        *field_iter);
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::remove_reference_from_source(
        const dbtype::Id &target,
        EntityRef &source,
        const dbtype::EntityField field)
    {
        // To break the reference, either delete this Entity from the source
        // Entity's field, and/or replace it with a default ID.
        //
        if (not (source.valid() and
            (field < dbtype::ENTITYFIELD_END) and
            (field > dbtype::ENTITYFIELD_BEGIN)))
        {
            LOG(error, "dbinterface", "remove_reference_from_source",
                "Source not valid or field out of range!  Field: "
                  + dbtype::entity_field_to_string(field)
                  + " source: " + source.id().to_string()
                  + " target: " + target.to_string());
        }
        else
        {
            switch (field)
            {
                case dbtype::ENTITYFIELD_contained_by:
                {
                    dbtype::ContainerPropertyEntity *container =
                        dynamic_cast<dbtype::ContainerPropertyEntity *>(
                            source.get());

                    if (container)
                    {
                        container->set_contained_by(
                            dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_action_contained_by:
                {
                    dbtype::ActionEntity *action =
                        dynamic_cast<dbtype::ActionEntity *>(source.get());

                    if (action)
                    {
                        action->set_action_contained_by(dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_player_home:
                {
                    dbtype::Player *player =
                        dynamic_cast<dbtype::Player *>(source.get());

                    if (player)
                    {
                        player->set_player_home(dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_thing_home:
                {
                    dbtype::Thing *thing =
                        dynamic_cast<dbtype::Thing *>(source.get());

                    if (thing)
                    {
                        thing->set_thing_home(dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_program_includes:
                {
                    dbtype::Program *program = dynamic_cast<dbtype::Program *>(
                        source.get());

                    if (program)
                    {
                        // Program isn't going to compile/run at this point,
                        // so clear them all.  It'll be repopulated during
                        // the compile.
                        //
                        program->clear_included_programs();
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_action_targets:
                {
                    dbtype::ActionEntity *action =
                        dynamic_cast<dbtype::ActionEntity *>(source.get());

                    if (action)
                    {
                        action->remove_action_target(target);
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_group_ids:
                {
                    dbtype::Group *group =
                        dynamic_cast<dbtype::Group *>(source.get());

                    if (group)
                    {
                        group->remove_from_group(target);
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_vehicle_controller:
                {
                    dbtype::Vehicle *vehicle =
                        dynamic_cast<dbtype::Vehicle *>(source.get());

                    if (vehicle)
                    {
                        vehicle->set_vehicle_controller(dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_vehicle_interior:
                {
                    dbtype::Vehicle *vehicle =
                        dynamic_cast<dbtype::Vehicle *>(source.get());

                    if (vehicle)
                    {
                        vehicle->set_vehicle_interior(dbtype::Id());
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_linked_programs:
                {
                    dbtype::ContainerPropertyEntity *container =
                        dynamic_cast<dbtype::ContainerPropertyEntity *>(
                            source.get());

                    if (container)
                    {
                        container->remove_program(target);
                    }

                    break;
                }

                case dbtype::ENTITYFIELD_security:
                {
                    concurrency::WriterLockToken token(*source.get());

                    dbtype::Security security =
                        source.get()->get_entity_security(token);

                    security.remove_from_list(target);
                    security.remove_admin(target);

                    source.get()->set_entity_security(security, token);

                    break;
                }

                case dbtype::ENTITYFIELD_owner:
                {
                    source.get()->set_entity_owner(dbtype::Id());
                    break;
                }

                default:
                {
                    LOG(error, "dbinterface", "remove_reference_from_source",
                        "Unknown field " + dbtype::entity_field_to_string(field)
                        + " source: " + source.id().to_string()
                        + " target: " + target.to_string());

                    break;
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    void UpdateManager::remove_reference(
        const dbtype::Id &source,
        const dbtype::Id &target,
        const dbtype::EntityField field)
    {
        if (not target.is_default())
        {
            EntityRef entity = DatabaseAccess::instance()->
                get_entity_deleted(target);

            if (entity.valid())
            {
                entity.get()->remove_entity_reference(source, field);
            }
        }
    }

    // ----------------------------------------------------------------------
    UpdateManager::UpdateManager(void)
      : thread_ptr(0),
        immediate_update_queue_sem(0),
        shutdown_thread_flag(false)
    {
    }

    // ----------------------------------------------------------------------
    UpdateManager::~UpdateManager()
    {
        shutdown();
    }


    // ------ class EntityUpdate ------

    // ----------------------------------------------------------------------
    void UpdateManager::EntityUpdate::merge_update(
        const dbtype::Entity::EntityFieldSet &fields,
        const dbtype::Entity::FlagsRemovedAdded &flags,
        const dbtype::Entity::ChangedIdFieldsMap &ids)
    {
        // Add any new changed fields to the set.
        //
        fields_changed.insert(fields.begin(), fields.end());

        // Add flags removed to removed, flags added to added
        //
        flags_changed.first.insert(flags.first.begin(), flags.first.end());
        flags_changed.second.insert(flags.second.begin(), flags.second.end());


        // Add IDs removed to removed, IDs added to added, creating a new
        // map entry as needed.
        //
        for (dbtype::Entity::ChangedIdFieldsMap::const_iterator incoming_iter =
              ids.begin();
             incoming_iter != ids.end();
            ++incoming_iter)
        {
            dbtype::Entity::ChangedIdFieldsMap::iterator id_iter =
                ids_changed.find(incoming_iter->first);

            if (id_iter == ids_changed.end())
            {
                // New field, insert an empty entry.
                //
                id_iter = ids_changed.insert(std::make_pair(
                    incoming_iter->first,
                    dbtype::Entity::IdsRemovedAdded())).first;
            }

            id_iter->second.first.insert(
                incoming_iter->second.first.begin(),
                incoming_iter->second.first.end());
            id_iter->second.second.insert(
                incoming_iter->second.second.begin(),
                incoming_iter->second.second.end());
        }
    }
}
}
