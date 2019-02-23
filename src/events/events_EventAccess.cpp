/*
 * events_EventAccess.cpp
 */

#include <string>
#include "text/text_StringConversion.h"
#include <dbinterface/dbinterface_DatabaseAccess.h>

#include "events/events_EventAccess.h"

#include "events/events_Event.h"
#include "events/events_SubscriptionData.h"
#include "events/events_SubscriptionProcessor.h"
#include "events/events_EventQueueProcessor.h"

#include "events/events_EntityChangedEvent.h"
#include "events/events_SiteEvent.h"

#include "events/events_ConnectionEventProcessor.h"
#include "events/events_EmitEventProcessor.h"
#include "events/events_EntityChangedEventProcessor.h"
#include "events/events_MovementEventProcessor.h"
#include "events/events_ProcessExecutionEventProcessor.h"
#include "events/events_SiteEventProcessor.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbtypes/dbtype_Entity.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace events
{
    // Statics
    //
    EventAccess *EventAccess::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    EventAccess *EventAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new EventAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void EventAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool EventAccess::startup(void)
    {
        if (not subscription_data_ptr)
        {
            subscription_data_ptr = new SubscriptionData();
            event_queue_ptr = new EventQueueProcessor(subscription_data_ptr);

            // Register the processors
            //
            subscription_data_ptr->register_subscription_processor(
                new ConnectionEventProcessor(subscription_data_ptr));
            subscription_data_ptr->register_subscription_processor(
                new EmitEventProcessor(subscription_data_ptr));
            subscription_data_ptr->register_subscription_processor(
                new EntityChangedEventProcessor(subscription_data_ptr));
            subscription_data_ptr->register_subscription_processor(
                new MovementEventProcessor(subscription_data_ptr));
            subscription_data_ptr->register_subscription_processor(
                new ProcessExecutionEventProcessor(subscription_data_ptr));
            subscription_data_ptr->register_subscription_processor(
                new SiteEventProcessor(subscription_data_ptr));

            // Start everything up
            //
            event_queue_ptr->startup();

            // Register as a listener
            //
            dbinterface::DatabaseAccess::instance()->add_entity_listener(this);
            dbtype::Entity::register_change_listener(this);
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void EventAccess::shutdown(void)
    {
        if (subscription_data_ptr)
        {
            // Unregister as a listener
            //
            dbinterface::DatabaseAccess::instance()->remove_entity_listener(
                this);
            dbtype::Entity::unregister_change_listener(this);

            // Shut down the event processing thread
            event_queue_ptr->shutdown();

            // Clean up memory
            //
            delete event_queue_ptr;
            event_queue_ptr = 0;

            delete subscription_data_ptr;
            subscription_data_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    bool EventAccess::unsubscribe(const SubscriptionId id)
    {
        // First, find out what processor handles this ID
        //
        const Event::EventType type =
            subscription_data_ptr->get_subscription_type(id);
        bool success = (type != Event::EVENT_END_INVALID);

        // Then, tell that processor to unsubscribe.
        //
        if (success)
        {
            SubscriptionProcessor * const processor_ptr =
                subscription_data_ptr->get_subscription_processor(type);

            if (not processor_ptr)
            {
                LOG(error, "events", "unsubscribe",
                    "Subscription ID " + text::to_string(id)
                      + " belongs to a processor that doesn't exist.");
            }
            else
            {
                success = processor_ptr->remove_subscription(id);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    SubscriptionId EventAccess::subscribe(
        const SubscriptionParams &params,
        const SubscriptionCallback &callback)
    {
        SubscriptionId id = 0;

        // First, find out which event processor will handle this.
        //
        Event::EventType event_type = Event::EVENT_END_INVALID;

        switch (params.get_type())
        {
            case SubscriptionParams::SUBSCRIPTION_CONNECTION:
            {
                event_type = Event::EVENT_CONNECTION;
                break;
            }

            case SubscriptionParams::SUBSCRIPTION_ENTITY_CHANGED:
            {
                event_type = Event::EVENT_ENTITY_CHANGED;
                break;
            }

            case SubscriptionParams::SUBSCRIPTION_EMIT:
            {
                event_type = Event::EVENT_EMIT;
                break;
            }

            case SubscriptionParams::SUBSCRIPTION_MOVEMENT:
            {
                event_type = Event::EVENT_MOVEMENT;
                break;
            }

            case SubscriptionParams::SUBSCRIPTION_PROCESS_EXECUTION:
            {
                event_type = Event::EVENT_PROCESS_EXECUTION;
                break;
            }

            case SubscriptionParams::SUBSCRIPTION_SITE:
            {
                event_type = Event::EVENT_SITE;
                break;
            }

            default:
            {
                LOG(error, "events", "subscribe",
                    "Unknown subscription params type: "
                      + text::to_string(params.get_type()));
                break;
            }
        }


        SubscriptionProcessor * const processor_ptr =
            subscription_data_ptr->get_subscription_processor(event_type);

        // Then, tell that processor to subscribe.
        //
        if (processor_ptr)
        {
            id = processor_ptr->add_subscription(params, callback);
        }

        return id;
    }

    // ----------------------------------------------------------------------
    void EventAccess::entity_created(dbtype::Entity *entity_ptr)
    {
        if (entity_ptr)
        {
            publish_event(new EntityChangedEvent(
                entity_ptr->get_entity_id(),
                entity_ptr->get_entity_type(),
                EntityChangedEvent::ENTITY_CREATED));
        }
    }

    // ----------------------------------------------------------------------
    void EventAccess::entity_deleted(dbtype::Entity *entity_ptr)
    {
        if (entity_ptr)
        {
            publish_event(new EntityChangedEvent(
                entity_ptr->get_entity_id(),
                entity_ptr->get_entity_type(),
                EntityChangedEvent::ENTITY_DELETED));
        }
    }

    // ----------------------------------------------------------------------
    void EventAccess::site_deleted(const dbtype::Id::SiteIdType site_id)
    {
        if (site_id)
        {
            publish_event(new SiteEvent(
                SiteEvent::SITE_ACTION_DELETE,
                site_id,
                ""));
        }
    }

    // ----------------------------------------------------------------------
    void EventAccess::entity_changed(
        dbtype::Entity *entity_ptr,
        const dbtype::Entity::EntityFieldSet &fields,
        const dbtype::Entity::FlagsRemovedAdded &flags_changed,
        const dbtype::Entity::ChangedIdFieldsMap &ids_changed)
    {
        if (entity_ptr)
        {
            publish_event(new EntityChangedEvent(
                entity_ptr->get_entity_id(),
                entity_ptr->get_entity_type(),
                fields,
                flags_changed,
                ids_changed));
        }
    }

    // ----------------------------------------------------------------------
    EventAccess::EventAccess(void)
      : subscription_data_ptr(0),
        event_queue_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    EventAccess::~EventAccess()
    {
        shutdown();
    }
}
}