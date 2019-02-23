/*
 * events_EventQueueProcessor.h
 */

#ifndef MUTGOS_EVENTS_EVENTQUEUEPROCESSOR_H
#define MUTGOS_EVENTS_EVENTQUEUEPROCESSOR_H

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread/thread.hpp>

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class Event;
    class SubscriptionData;

    /**
     * A queue where published events are stored until they can be processed
     * on a background thread, which is also implemented in this class.
     *
     * The background thread will pull events off the queue and dispatch
     * them to the appropriate EventProcessor.
     */
    class EventQueueProcessor
    {
    public:
        /**
         * Constructor.
         * @param data_ptr[in] Pointer to SubscriptionData instance used
         * by all processors.
         */
        EventQueueProcessor(SubscriptionData *data_ptr);

        /**
         * Destructor.  Will shut down if currently running.
         */
        ~EventQueueProcessor();

        /**
         * Starts the processing thread, if not already started.
         * Not thread safe.
         */
        void startup(void);

        /**
         * Stops the processing thread, if not already stopped.
         * Not thread safe.
         */
        void shutdown(void);

        /**
         * Adds an event to the queue to be processed.
         * @param event_ptr[in] The event to add.  Ownership of the pointer
         * transfers to this class.  Null events are ignored.
         */
        void add_event(Event *event_ptr);

        /**
         * Used by Boost threads to start our threaded code.
         */
        void operator()();

        /**
         * Main loop of EventQueueProcessor thread.
         */
        void thread_main(void);

    private:
        /** Lock free queue of processes waiting to be executed */
        typedef boost::lockfree::queue<Event *> EventQueue;

        SubscriptionData * const subscription_data; ///< Subscription and processor data

        // TODO Will need to handle semaphore overflow ( > 32,000) at some point
        /** Semaphore associated with the event queue so the thread can easily
            block and wait for the next event to process.  Thead safe. */
        boost::interprocess::interprocess_semaphore event_queue_semaphore;
        EventQueue event_queue; ///< The event queue.  A null entry means to shut down.
        boost::atomic<boost::thread *> thread_ptr; ///< Non-null when thread is running.
    };
}
}

#endif //MUTGOS_EVENTS_EVENTQUEUEPROCESSOR_H
