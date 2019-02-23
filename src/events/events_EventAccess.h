/*
 * events_EventAccess.h
 */

#ifndef MUTGOS_EVENTS_EVENTACCESS_H
#define MUTGOS_EVENTS_EVENTACCESS_H

#include "dbinterface/dbinterface_DatabaseEntityListener.h"
#include "dbtypes/dbtype_DatabaseEntityChangeListener.h"
#include "dbtypes/dbtype_Entity.h"

#include "events/events_CommonTypes.h"
#include "events/events_SubscriptionParams.h"
#include "events/events_SubscriptionCallback.h"
#include "events/events_EventQueueProcessor.h"

namespace mutgos
{
namespace events
{
    // TODO In subsystem (mostly Params classes) remove all 'sets' where practical.

    // Forward declarations
    //
    class Event;
    class SubscriptionData;

    /**
     * This singleton class is meant to be used by other clients to subscribe
     * to and post events.  Events are different from Process Messages or
     * Channels in that they are broadcast, not point-to-point.  Anyone
     * interested in knowing about specific events can subscribe to them,
     * and they will be notified when said events occur.
     */
    class EventAccess : public dbinterface::DatabaseEntityListener,
                               dbtype::DatabaseEntityChangeListener
    {
    public:
        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static EventAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * This is thread safe.
         * @return The singleton instance, or null if not created.
         */
        static inline EventAccess *instance(void)
          { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * It will also register itself as a listener as needed in other
         * subsystems, create any processor classes, start any threads, etc.
         * Attempting to use other methods prior to calling startup() may
         * result in a crash.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Basically the opposite of what startup() does.
         */
        void shutdown(void);


        // -----  Methods for use by clients (listeners).

        /**
         * Unsubscribes (removes) from a previous subscription.
         * This is thread safe.
         * @param id[in] The ID of the subscription to remove.
         * @return True if found and removed.
         */
        bool unsubscribe(const SubscriptionId id);

        /**
         * Subscribes to an event of interest.
         * If the callback is for a Process, this subscription will
         * automatically be removed when the process ends.
         * This is thread safe.
         * @param params[in] The parameters describing the subscription
         * criteria.
         * @param callback[in] The listener to callback when an event matches
         * the params.
         * @return The subscription ID the refers to this specific
         * subscription, or 0 if error,
         */
        SubscriptionId subscribe(
            const SubscriptionParams &params,
            const SubscriptionCallback &callback);


        // -----  Methods for use by other subsystems.

        /**
         * Submits an event to be processed by the Event subsystem.  It will
         * notify listeners whose parameters match the event.
         * Processing will occur in the background, on a separate thread.
         * This is thread safe.
         * @param event_ptr[in] The event to publish.  Ownership of the pointer
         * passes to this class.
         */
        inline void publish_event(Event * const event_ptr)
          { event_queue_ptr->add_event(event_ptr); }


        // -----  Various listeners for other subsystems that will in turn
        //        create and publish Events.

        /**
         * Called at the moment when an Entity has been created.
         * This may cause an Event to be publishd.
         * @param entity_ptr[in] The pointer to the newly created
         * Entity.  Please note it will have only the bare minimum
         * filled in (ID, type, etc).  Do not delete or save this pointer.
         */
        virtual void entity_created(dbtype::Entity *entity_ptr);

        /**
         * Called when an Entity has been requested to be deleted.
         * This may cause an Event to be publishd.
         * @param entity_ptr[in] The pointer to the soon-to-be-deleted
         * Entity.  Do not delete or save this pointer.
         */
        virtual void entity_deleted(dbtype::Entity *entity_ptr);

        /**
         * Called when a site has been requested to be deleted.
         * This may cause an Event to be publishd.
         * @param site_id[in] The ID of the soon-to-be-deleted site.
         */
        virtual void site_deleted(const dbtype::Id::SiteIdType site_id);

        /**
         * Called when the provided entity has changed in some way.
         * Each attribute changed on an entity will cause this to be called,
         * however several changes may be in a single call.
         * Note that the Entity is still locked when this is being called,
         * meaning getting and setting most attributes on the Entity is
         * impossible; it must instead be scheduled for a later time.
         * This may cause an Event to be publishd.
         * @param entity_ptr[in] The entity that has changed.
         * @param fields[in] The fields that have changed.
         * @param flags_changed[in] Detailed information on what flags have
         * changed.
         * @param ids_changed[in] Detailed information about changes concerning
         * fields of type ID (or lists of IDs).
         */
        virtual void entity_changed(
            dbtype::Entity *entity_ptr,
            const dbtype::Entity::EntityFieldSet &fields,
            const dbtype::Entity::FlagsRemovedAdded &flags_changed,
            const dbtype::Entity::ChangedIdFieldsMap &ids_changed);

    private:
        /**
         * Singleton constructor.
         */
        EventAccess(void);

        /**
         * Singleton destructor.  Shuts down if not already done.
         */
        virtual ~EventAccess();

        // No copying
        //
        EventAccess(const EventAccess &rhs);
        EventAccess &operator=(const EventAccess &rhs);

        static EventAccess *singleton_ptr; ///< Singleton pointer.

        SubscriptionData *subscription_data_ptr; ///< SubscriptionData instance for all classes
        EventQueueProcessor *event_queue_ptr; ///< Processes events on separate thread
    };
}
}

#endif //MUTGOS_SERVER_EVENTACCESS_H
