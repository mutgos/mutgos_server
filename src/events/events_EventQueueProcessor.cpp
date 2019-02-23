/*
 * events_EventQueueProcessor.cpp
 */

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread/thread.hpp>

#include "executor/executor_ProcessInfo.h"

#include "events/events_CommonTypes.h"
#include "events/events_EventAccess.h"
#include "events/events_EventQueueProcessor.h"
#include "events/events_Event.h"
#include "events/events_SubscriptionProcessor.h"
#include "events/events_SubscriptionData.h"
#include "events/events_EntityChangedEvent.h"
#include "events/events_SiteEvent.h"
#include "events/events_ProcessExecutionEvent.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    EventQueueProcessor::EventQueueProcessor(SubscriptionData *data_ptr)
        : subscription_data(data_ptr),
          event_queue_semaphore(0),
          event_queue(1)
    {
        thread_ptr.store(0);
    }

    // ----------------------------------------------------------------------
    EventQueueProcessor::~EventQueueProcessor()
    {
        // Shutdown and deallocate all memory
        //
        shutdown();

        Event *event_ptr = 0;

        while (not event_queue.empty())
        {
            event_queue.pop(event_ptr);

            delete event_ptr;
            event_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    void EventQueueProcessor::startup(void)
    {
        if (not thread_ptr.load())
        {
            thread_ptr = new boost::thread(boost::ref(*this));
        }
    }

    // ----------------------------------------------------------------------
    void EventQueueProcessor::shutdown(void)
    {
        if (thread_ptr.load())
        {
            // A null signals for the thread to shut down.
            //
            event_queue.push(0);
            event_queue_semaphore.post();

            thread_ptr.load()->join();
            thread_ptr.store(0);
        }
    }

    // ----------------------------------------------------------------------
    void EventQueueProcessor::add_event(Event *event_ptr)
    {
        if (event_ptr)
        {
            event_queue.push(event_ptr);
            event_queue_semaphore.post();
        }
    }

    // ----------------------------------------------------------------------
    void EventQueueProcessor::operator()()
    {
        thread_main();
    }

    // ----------------------------------------------------------------------
    void EventQueueProcessor::thread_main(void)
    {
        bool running = true;
        Event *event_ptr = 0;
        SubscriptionProcessor *processor_ptr = 0;

        LOG(debug, "events", "thread_main",
            "EventQueueProcessor thread started.");

        while (running)
        {
            // Simply pick up the next event, call the appropriate processor,
            // then perform any optional post-processing depending on the
            // event.
            //
            event_queue_semaphore.wait();

            if (event_queue.pop(event_ptr))
            {
                if (not event_ptr)
                {
                    // Shutdown
                    running = false;
                }
                else
                {
                    // Lookup and call processor
                    //
                    processor_ptr =
                        subscription_data->get_subscription_processor(
                            event_ptr->get_event_type());

                    if (processor_ptr)
                    {
                        processor_ptr->process_event(event_ptr);
                    }

                    switch (event_ptr->get_event_type())
                    {
                        // If entity deletion, let every processor know.
                        //
                        case Event::EVENT_ENTITY_CHANGED:
                        {
                            EntityChangedEvent * const entity_event_ptr =
                                static_cast<EntityChangedEvent *>(event_ptr);

                            if (entity_event_ptr->get_entity_action() ==
                                EntityChangedEvent::ENTITY_DELETED)
                            {
                                const dbtype::Id &deleted_id =
                                    entity_event_ptr->get_entity_id();
                                SubscriptionProcessor *processor_ptr = 0;

                                for (int index = 0;
                                     index < Event::EVENT_END_INVALID;
                                     ++index)
                                {
                                    processor_ptr = subscription_data->
                                        get_subscription_processor(
                                          (Event::EventType) index);

                                    if (processor_ptr)
                                    {
                                        processor_ptr->entity_deleted(
                                            deleted_id);
                                    }
                                }
                            }

                            break;
                        }

                        // If site deletion, let every processor know.
                        //
                        case Event::EVENT_SITE:
                        {
                            SiteEvent * const site_event_ptr =
                                static_cast<SiteEvent *>(event_ptr);

                            if (site_event_ptr->get_site_action() ==
                                SiteEvent::SITE_ACTION_DELETE)
                            {
                                const dbtype::Id::SiteIdType deleted_site_id =
                                    site_event_ptr->get_site_id();
                                SubscriptionProcessor *processor_ptr = 0;

                                for (int index = 0;
                                     index < Event::EVENT_END_INVALID;
                                     ++index)
                                {
                                    processor_ptr = subscription_data->
                                        get_subscription_processor(
                                            (Event::EventType) index);

                                    if (processor_ptr)
                                    {
                                        processor_ptr->site_deleted(
                                            deleted_site_id);
                                    }
                                }
                            }

                            break;
                        }

                        // Auto unsubscribe subscriptions for a process when
                        // it has ended.
                        //
                        case Event::EVENT_PROCESS_EXECUTION:
                        {
                            ProcessExecutionEvent * const process_event_ptr =
                                static_cast<ProcessExecutionEvent *>(event_ptr);

                            if (process_event_ptr->get_process_state() ==
                                executor::ProcessInfo::PROCESS_STATE_COMPLETED)
                            {
                                const SubscriptionIdList subscriptions =
                                    subscription_data->get_subscriptions_for_process(
                                        process_event_ptr->get_process_id());

                                for (SubscriptionIdList::const_iterator iter =
                                        subscriptions.begin();
                                    iter != subscriptions.end();
                                    ++iter)
                                {
                                    EventAccess::instance()->unsubscribe(
                                        *iter);
                                }
                            }

                            break;
                        }

                        default:
                        {
                            break;
                        }
                    }

                    delete event_ptr;
                    event_ptr = 0;
                }
            }
        }

        LOG(debug, "events", "thread_main",
            "EventQueueProcessor thread stopped.");
    }
}
}
