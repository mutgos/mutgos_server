/*
 * security_SecurityAccess.cpp
 */

#include <algorithm>

#include <boost/thread/shared_mutex.hpp>

#include "osinterface/osinterface_OsTypes.h"

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "concurrency/concurrency_ReaderLockToken.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_Group.h"
#include "dbtypes/dbtype_Capability.h"

#include "dbinterface/dbinterface_CommonTypes.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbinterface/dbinterface_EntityRef.h"

#include "events/events_EntityChangedSubscriptionParams.h"
#include "events/events_SiteSubscriptionParams.h"
#include "events/events_SubscriptionCallback.h"
#include "events/events_EventAccess.h"
#include "events/events_EntityChangedEvent.h"
#include "events/events_SiteEvent.h"

#include "security_SecurityAccess.h"
#include "security_OperationsCapabilities.h"
#include "security_SecurityException.h"

#include "security_AcceptAllChecker.h"
#include "security_AdminSecurityChecker.h"
#include "security_CrossSiteChecker.h"
#include "security_CharacterOnlineChecker.h"
#include "security_FindCharacterByNameChecker.h"
#include "security_GetEntityFieldChecker.h"
#include "security_CreateEntityChecker.h"
#include "security_DeleteEntityChecker.h"
#include "security_SetEntityFieldChecker.h"
#include "security_GetSetApplicationSecurityChecker.h"
#include "security_GetApplicationPropertyChecker.h"
#include "security_SetApplicationPropertyChecker.h"
#include "security_UseActionChecker.h"
#include "security_TransferEntityChecker.h"
#include "security_SendTextRoomChecker.h"
#include "security_SendTextRoomUnrestrictedChecker.h"
#include "security_SendTextEntityChecker.h"

namespace mutgos
{
namespace security
{
    // Statics
    //
    SecurityAccess *SecurityAccess::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    SecurityAccess *SecurityAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new SecurityAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::startup(void)
    {
        if (not (capability_subscription_id or site_deletion_subscription_id))
        {
            populate_security();
            subscribe();
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::shutdown(void)
    {
        if (capability_subscription_id or site_deletion_subscription_id)
        {
            unsubscribe();
            depopulate_security();
        }
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if (not context.security_check_cache(
            operation,
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        const dbtype::EntityType entity_type,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if (not context.security_check_cache(
            operation,
            entity_type,
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context,
                        entity_type),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                entity_type,
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context,
                entity_type);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if (not entity_target.valid())
        {
            return result;
        }

        if (not context.security_check_cache(
            operation,
            entity_target.id(),
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context,
                        entity_target),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                entity_target.id(),
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context,
                entity_target);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const std::string &application,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if (not entity_target.valid())
        {
            return result;
        }

        // Normalize application name / strip out remainder of path.
        //
        const std::string application_name =
            dbtype::PropertyEntity::get_application_name_from_path(application);

        if (application_name.empty())
        {
            return result;
        }

        if (not context.security_check_cache(
            operation,
            entity_target.id(),
            application_name,
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context,
                        entity_target,
                        application_name),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                entity_target.id(),
                application_name,
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context,
                entity_target,
                application_name);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        const dbtype::EntityField entity_field,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if (not entity_target.valid())
        {
            return result;
        }

        if (not context.security_check_cache(
            operation,
            entity_target.id(),
            entity_field,
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context,
                        entity_target,
                        entity_field),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                entity_target.id(),
                entity_field,
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context,
                entity_target,
                entity_field);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::security_check(
        const Operation operation,
        Context &context,
        dbinterface::EntityRef &entity_target,
        dbinterface::EntityRef &entity_source,
        const bool throw_exception_on_denied)
    {
        Result result = RESULT_SKIP;

        if ((not entity_target.valid()) or (not entity_source.valid()))
        {
            return result;
        }

        if (not context.security_check_cache(
            operation,
            entity_target.id(),
            entity_source.id(),
            result))
        {
            // Not cached, have to determine manually.
            SecurityVector &security_checkers = *operation_security[operation];

            for (SecurityVector::iterator security_iter =
                security_checkers.begin();
                 security_iter != security_checkers.end();
                 ++security_iter)
            {
                if (not check_result(
                    (*security_iter)->security_check(
                        operation,
                        context,
                        entity_target,
                        entity_source),
                    result))
                {
                    break;
                }
            }

            context.save_security_check_result(
                operation,
                entity_target.id(),
                entity_source.id(),
                result);
        }

        if (throw_exception_on_denied and (result != RESULT_ACCEPT))
        {
            throw SecurityException(
                operation,
                context,
                entity_target,
                entity_source);
        }

        return (result == RESULT_ACCEPT ? true : false);
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::populate_context_capabilities(Context &context)
    {
        if (not context.has_capabilities_set())
        {
            if (context.get_requester().is_site_default())
            {
                LOG(error, "security", "populate_capabilities",
                    "Context has default site for requester!");

                context.capabilities_set();
                return;
            }

            boost::shared_lock<boost::shared_mutex> read_lock(security_lock);

            const dbtype::Id &requester = context.get_requester();

            // See if our local site (based on requester) has capabilities
            // for us...
            populate_context_capabilities(
                requester.get_site_id(),
                read_lock,
                context);

            // Also do global site...
            populate_context_capabilities(
                dbinterface::GLOBAL_SITE_ID,
                read_lock,
                context);

            // We are always an admin if #*-3, #*-4.
            // TODO Put these admin DBs somewhere common
            //
            if ((requester.get_entity_id() == 3) ||
                (requester.get_entity_id() == 4))
            {
                context.set_run_as_admin();
            }

            // Done setting capabilities.
            context.capabilities_set();
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::subscribed_event_matched(
        const events::SubscriptionId id,
        events::Event &event)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            security_lock);

        if ((capability_subscription_id == id) and
            (event.get_event_type() == events::Event::EVENT_ENTITY_CHANGED))
        {
            events::EntityChangedEvent * const entity_event_ptr =
                static_cast<events::EntityChangedEvent *>(&event);
            const dbtype::Id capability_id = entity_event_ptr->get_entity_id();

            // Something about the capability changed, so blow away the entry
            // so it can be re-cached.
            //
            SiteToCapabilities::iterator site_iter = site_to_capabilities.find(
                capability_id.get_site_id());

            if (site_iter != site_to_capabilities.end())
            {
                // This seems inefficient, but it's probably not much worse
                // than looking up the changed/deleted Entity name, then
                // matching it to the equivalent enum.
                //
                for (CapabilityGroupsLookup::iterator capability_groups_iter =
                        site_iter->second.begin();
                     capability_groups_iter != site_iter->second.end();
                     ++capability_groups_iter)
                {
                    if (not capability_groups_iter->empty())
                    {
                        if (capability_groups_iter->front() == capability_id)
                        {
                            // Found the entry.  Clear it to cause a re-cache
                            // and we're done.
                            capability_groups_iter->clear();
                            break;
                        }
                    }
                }
            }
        }
        else if ((site_deletion_subscription_id == id) and
            (event.get_event_type() == events::Event::EVENT_SITE))
        {
            // If the site got deleted, remove everything from the cache
            // related to it.
            //
            events::SiteEvent * const site_event_ptr =
                static_cast<events::SiteEvent *>(&event);

            if (site_event_ptr->get_site_action() ==
                events::SiteEvent::SITE_ACTION_DELETE)
            {
                site_to_capabilities.erase(site_event_ptr->get_site_id());
            }
        }
        else
        {
            LOG(warning, "security", "subscribed_event_matched",
                "Got an unknown subscribed event!");
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::subscription_deleted(
        const events::SubscriptionIdList &ids_deleted)
    {
        boost::unique_lock<boost::shared_mutex> write_lock(
            security_lock);

        // No subscriptions should refer to specific entities;
        // this should never happen.  Log it and resubscribe.
        //
        //
        for (events::SubscriptionIdList::const_iterator id_iter =
                ids_deleted.begin();
             id_iter != ids_deleted.end();
             ++id_iter)
        {
            if (*id_iter == capability_subscription_id)
            {
                LOG(error, "security", "subscription_deleted",
                    "Capability subscription was unexpectedly deleted!  "
                    "Resubscribing...");

                capability_subscription_id = 0;
            }
            else if (*id_iter == site_deletion_subscription_id)
            {
                LOG(error, "security", "subscription_deleted",
                    "Site subscription was unexpectedly deleted!  "
                    "Resubscribing...");

                site_deletion_subscription_id = 0;
            }
        }

        subscribe();
    }

    // ----------------------------------------------------------------------
    SecurityAccess::SecurityAccess(void)
      : capability_subscription_id(0),
        site_deletion_subscription_id(0)
    {
    }

    // ----------------------------------------------------------------------
    SecurityAccess::~SecurityAccess()
    {
        shutdown();
    }

    // ----------------------------------------------------------------------
    bool SecurityAccess::check_result(
        const Result new_result,
        Result &current_result)
    {
        bool check_more = true;

        switch (new_result)
        {
            case RESULT_ACCEPT:
            {
                if (current_result == RESULT_SKIP)
                {
                    current_result = RESULT_ACCEPT;
                }
                break;
            }

            case RESULT_ACCEPT_ALWAYS:
            {
                current_result = RESULT_ACCEPT;
                check_more = false;
                break;
            }

            case RESULT_DENY:
            case RESULT_DENY_ALWAYS:
            {
                current_result = RESULT_DENY;
                check_more = false;
                break;
            }

            case RESULT_SKIP:
            {
                break;
            }

            default:
            {
                LOG(error, "security", "check_result", "Unknown result!");
            }
        }

        return check_more;
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::populate_security(void)
    {
        for (MG_UnsignedInt index = 0;
            index < OPERATION_END_INVALID;
            ++index)
        {
            // Populate the operation entries with an empty array
            // and add the site and admin checker.
            //
            operation_security[index] = new SecurityVector();
            operation_security[index]->push_back(new CrossSiteChecker());
            operation_security[index]->push_back(new AdminSecurityChecker());
        }

        // Add the other security checkers.
        operation_security[OPERATION_FIND_BY_NAME_RELATIVE]->push_back(
            new AcceptAllChecker());
        operation_security[OPERATION_FIND_CHARACTER_BY_NAME]->push_back(
            new FindCharacterByNameChecker());
        operation_security[OPERATION_CHARACTER_ONLINE]->push_back(
            new CharacterOnlineChecker());
        operation_security[OPERATION_GET_CONTAINS]->push_back(
            new GetEntityFieldChecker());
        operation_security[OPERATION_GET_ACTIONS]->push_back(
            new GetEntityFieldChecker());


        operation_security[OPERATION_CREATE_ENTITY]->push_back(
            new CreateEntityChecker());
        operation_security[OPERATION_DELETE_ENTITY]->push_back(
            new DeleteEntityChecker());

        operation_security[OPERATION_GET_ENTITY_FIELD]->push_back(
            new GetEntityFieldChecker());
        operation_security[OPERATION_SET_ENTITY_FIELD]->push_back(
            new SetEntityFieldChecker());

        // TODO Temp operation - will be replaced by something else later
        operation_security[OPERATION_ENTITY_TOSTRING]->push_back(
            new GetEntityFieldChecker());

        operation_security[OPERATION_GET_APPLICATION_SECURITY]->push_back(
            new GetSetApplicationSecurityChecker());
        operation_security[OPERATION_SET_APPLICATION_SECURITY]->push_back(
            new GetSetApplicationSecurityChecker());
        operation_security[OPERATION_GET_APPLICATION_PROPERTY]->push_back(
            new GetApplicationPropertyChecker());
        operation_security[OPERATION_SET_APPLICATION_PROPERTY]->push_back(
            new SetApplicationPropertyChecker());
        operation_security[OPERATION_DELETE_APPLICATION_PROPERTY]->push_back(
            new GetSetApplicationSecurityChecker());

        operation_security[OPERATION_TRANSFER_ENTITY]->push_back(
            new TransferEntityChecker());

        operation_security[OPERATION_SEND_TEXT_ROOM]->push_back(
            new SendTextRoomChecker());
        operation_security[OPERATION_SEND_TEXT_ROOM_UNRESTRICTED]->push_back(
            new SendTextRoomUnrestrictedChecker());
        operation_security[OPERATION_SEND_TEXT_ENTITY]->push_back(
            new SendTextEntityChecker());

        operation_security[OPERATION_USE_ACTION]->push_back(
            new UseActionChecker());
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::depopulate_security(void)
    {
        for (MG_UnsignedInt index = 0;
             index < OPERATION_END_INVALID;
             ++index)
        {
            for (SecurityVector::iterator security_iter =
                    operation_security[index]->begin();
                security_iter != operation_security[index]->end();
                ++security_iter)
            {
                delete *security_iter;
            }

            operation_security[index]->clear();

            delete operation_security[index];
            operation_security[index] = 0;
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::subscribe(void)
    {
        if (not capability_subscription_id)
        {
            // Subscribe to every change of capability member lists.
            //
            events::EntityChangedSubscriptionParams entity_sub;

            entity_sub.add_entity_action(
                events::EntityChangedEvent::ENTITY_UPDATED);
            entity_sub.add_entity_action(
                events::EntityChangedEvent::ENTITY_DELETED);
            entity_sub.add_entity_type(dbtype::ENTITYTYPE_capability);
            entity_sub.add_entity_field(dbtype::ENTITYFIELD_group_ids);
            entity_sub.add_entity_field(dbtype::ENTITYFIELD_group_disabled_ids);

            capability_subscription_id =
                events::EventAccess::instance()->subscribe(
                    entity_sub,
                    events::SubscriptionCallback(this));

            if (not capability_subscription_id)
            {
                LOG(error, "security", "subscribe",
                    "Could not subscribe to Capability entity changes!");
            }
        }

        if (not site_deletion_subscription_id)
        {
            events::SiteSubscriptionParams site_sub;

            site_deletion_subscription_id =
                events::EventAccess::instance()->subscribe(
                    site_sub,
                    events::SubscriptionCallback(this));

            if (not site_deletion_subscription_id)
            {
                LOG(error, "security", "subscribe",
                    "Could not subscribe to Site changes!");
            }
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::unsubscribe(void)
    {
        if (capability_subscription_id)
        {
            events::EventAccess::instance()->unsubscribe(
                capability_subscription_id);

            capability_subscription_id = 0;
        }

        if (site_deletion_subscription_id)
        {
            events::EventAccess::instance()->unsubscribe(
                site_deletion_subscription_id);

            site_deletion_subscription_id = 0;
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::populate_context_capabilities(
        const dbtype::Id::SiteIdType site_id,
        boost::shared_lock<boost::shared_mutex> &read_lock,
        security::Context &context)
    {
        // Go through all capabilities, and see if the user and/or program
        // is listed.
        // If found an empty capability, relock as exclusive and populate.
        //
        dbinterface::DatabaseAccess * const database_ptr =
            dbinterface::DatabaseAccess::instance();
        dbinterface::EntityRef group_ref;
        dbtype::Group * group_ptr = 0;
        const CapabilityGroupsLookup &capabilities =
            site_to_capabilities[site_id];

        if (capabilities.empty())
        {
            // First use of site.  Populate.
            //
            read_lock.unlock();

            // Scope for lock.  Confirm still empty after acquiring.
            {
                boost::unique_lock<boost::shared_mutex> write_lock(
                    security_lock);

                if (capabilities.empty())
                {
                    // Populate any capability to trigger initial vector
                    // creation.
                    populate_capability_cache(site_id, CAPABILITY_ADMIN);
                }
            }

            read_lock.lock();
        }

        // For each capability...
        for (MG_UnsignedInt index = 0; index < CAPABILITY_END_INVALID; ++index)
        {
            const Capability capability = (Capability) index;
            const CapabilityGroups &groups = capabilities[index];

            if (groups.empty())
            {
                // First use of capability, or cache must be refreshed.
                //
                read_lock.unlock();

                // Scope for lock.  Confirm still empty after acquiring.
                {
                    boost::unique_lock<boost::shared_mutex> write_lock(
                        security_lock);

                    if (groups.empty())
                    {
                        populate_capability_cache(site_id, capability);
                    }
                }

                read_lock.lock();
            }

            // Check to see if a group within a capability has our IDs.
            //
            for (CapabilityGroups::const_iterator group_iter = groups.begin();
                group_iter != groups.end();
                ++group_iter)
            {
                // Get from database.
                group_ref = database_ptr->get_entity(*group_iter);

                if (group_ref.valid())
                {
                    group_ptr = dynamic_cast<dbtype::Group *>(group_ref.get());
                }

                if (not group_ptr)
                {
                    LOG(error, "security", "populate_context_capabilities",
                        "Invalid group/capability ID: "
                        + group_iter->to_string(true));
                }
                else
                {
                    concurrency::ReaderLockToken group_lock(*group_ptr);

                    if (context.has_run_as_requester())
                    {
                        // Check requester if valid
                        if (group_ptr->is_in_group(
                            context.get_requester(),
                            group_lock))
                        {
                            // Requester has capability.  Add.  Also set
                            // admin flag if admin capability.
                            //
                            context.add_capability(capability);

                            if (capability == CAPABILITY_ADMIN)
                            {
                                context.set_run_as_admin();
                            }
                        }
                    }

                    // Program has capability.  Add.  Also set
                    // admin flag if admin capability.
                    //
                    if (group_ptr->is_in_group(
                        context.get_program(),
                        group_lock))
                    {
                        // Program has capability.  Add.  Also set
                        // admin flag if admin capability.
                        //
                        context.add_capability(capability);

                        if (capability == CAPABILITY_ADMIN)
                        {
                            context.set_run_as_admin();
                        }
                    }
                }

                group_ptr = 0;
                group_ref.clear();
            }
        }
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::populate_capability_cache(
        const dbtype::Id::SiteIdType site_id,
        const security::Capability capability)
    {
        if (capability >= CAPABILITY_END_INVALID)
        {
            LOG(error, "security", "populate_capability_cache",
                "Trying to populate invalid capability.");
            return;
        }

        dbinterface::DatabaseAccess * const database_ptr =
            dbinterface::DatabaseAccess::instance();

        CapabilityGroupsLookup &capabilities = site_to_capabilities[site_id];

        if (capabilities.empty())
        {
            // First use; add empty capability lists
            //
            for (MG_UnsignedInt index = 0;
                 index < CAPABILITY_END_INVALID;
                 ++index)
            {
                capabilities.push_back(CapabilityGroups());
            }
        }

        CapabilityGroups &groups = capabilities[capability];

        groups.clear();

        const dbtype::Entity::IdVector search_result = database_ptr->find(
            site_id,
            dbtype::ENTITYTYPE_capability,
            0,
            capability_to_string(capability),
            true);

        if (search_result.size() != 1)
        {
            LOG(error, "security", "populate_capability_cache", "Site "
              + text::to_string(site_id)
              + " does not have exactly one capability: "
              + capability_to_string(capability));

            groups.push_back(dbtype::Id());
        }
        else
        {
            groups.push_back(search_result.front());
        }

        // Have the capability, now find out what other groups it references.
        add_referenced_groups(groups.front(), groups);
    }

    // ----------------------------------------------------------------------
    void SecurityAccess::add_referenced_groups(
        const dbtype::Id &input_group,
        security::SecurityAccess::CapabilityGroups &referenced_groups)
    {
        dbinterface::DatabaseAccess * const database_ptr =
            dbinterface::DatabaseAccess::instance();
        dbinterface::EntityRef ref;
        dbtype::Group * group_ptr = 0;

        if (not input_group.is_default())
        {
            ref = database_ptr->get_entity(input_group);

            if (not ref.valid())
            {
                LOG(error, "security", "add_referenced_groups",
                    "Got invalid entity ref: "
                    + input_group.to_string(true));
            }
            else
            {
                group_ptr = dynamic_cast<dbtype::Group *>(ref.get());

                if (not group_ptr)
                {
                    LOG(error, "security", "add_referenced_groups",
                        "ID does not represent a group or subclass: "
                        + input_group.to_string(true));
                }
            }
        }

        if (group_ptr)
        {
            // Valid group, now figure out who else it refers to
            const dbtype::Entity::IdVector entries =
                group_ptr->get_all_in_group();

            // If the entry is a Group or Capability, and not disabled, then
            // add it to referenced_groups.
            //
            for (dbtype::Entity::IdVector::const_iterator entry_iter =
                    entries.begin();
                entry_iter != entries.end();
                ++entry_iter)
            {
                if (not group_ptr->is_in_disabled_group(*entry_iter))
                {
                    const dbtype::EntityType type =
                        database_ptr->get_entity_type(*entry_iter);

                    switch (type)
                    {
                        case dbtype::ENTITYTYPE_group:
                        case dbtype::ENTITYTYPE_capability:
                        {
                            referenced_groups.push_back(*entry_iter);
                            break;
                        }

                        default:
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
}
}
