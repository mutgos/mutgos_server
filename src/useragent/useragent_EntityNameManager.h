/*
 * useragent_EntityNameManager.h
 */

#ifndef MUTGOS_USERAGENT_ENTITYNAMEMANAGER_H
#define MUTGOS_USERAGENT_ENTITYNAMEMANAGER_H

#include "dbtypes/dbtype_Entity.h"

#include "executor/executor_Process.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class ConnectionEvent;
    class EntityChangedEvent;
}
namespace useragent
{
    /**
     * This class is spawned by whatever starts up MUTGOS, and listens
     * for connects, disconnects, and name changes of players and puppets
     * across all sites until the server is shutdown.  If a connection is made,
     * it will register all the names of the Player and Puppets with the name
     * registry.  If a disconnection is performed, it will unregister all
     * the associated names.  If the player or puppets are renamed, the
     * names will be updated in the registry.  Same thing with puppet creates
     * and deletes.  Chowns are monitored in case the new owner is not online;
     * in that case the puppet will be removed from the name registry.
     *
     * In short, this keeps the name registry up to date
     */
    class EntityNameManager : public executor::Process
    {
    public:
        /**
         * Constructor.
         */
        EntityNameManager(void);

        /**
         * Required virtual destructor.
         */
        virtual ~EntityNameManager();

        /**
         * Optionally implemented by any class who wants to be a process and
         * executed by the ExecutorInterface, this is called when the Process
         * is initially added to the executor via ExecutorAccess::add_process().
         * It provides the Process a chance to add resources, etc before
         * execution officially begins.
         * @param pid[in] The PID of the newly added Process.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         */
        virtual void process_added(
            const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Implemented by any class who wants to be a process and be executed by
         * the ExecutorInterface, this is called when the class instance is
         * allowed to 'do work'.  When in this method, the process is being
         * exclusively run on one of potentially many threads.  The process cannot
         * execute forever; when it hits a blocking condition or has executed
         * 'long enough', it must yield to other processes by returning from this
         * method.
         * Even when waiting on messages or resources, in rare situations this
         * method may be called anyway.  If so, just return the correct
         * status to go back to waiting for messages.
         * @param pid[in] The PID of the process being executed.  Useful in case
         * the same class instance executes different processes (note that in the
         * case of running multiple processes, this method may be called
         * reentrantly).
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @return A status indicating if the process has completed its work or
         * would like to be called again later.
         */
        virtual ProcessStatus process_execute(
            const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Similar to the standard process_execute(), this variant is called
         * in response to a message to be processed.
         * For us, this is called when subscriptions to events are
         * satisfied.
         * @param pid[in] The PID of the process being executed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @param message[in] The message to be processed.
         * @return The status indicating if process has completed its work.
         * @see process_execute(const PID pid);
         */
        virtual ProcessStatus process_execute(
            const executor::PID pid,
            executor::ProcessServices &services,
            executor::ProcessMessage &message);

        /**
         * This might be called many times, even while the process is
         * executing.
         * @param pid[in] The PID of the process whose name is desired.
         * @return The name of the process.  Must not be empty.
         */
        virtual std::string process_get_name(const executor::PID pid);

        /**
         * Implemented by subclass.
         * @param pid[in] The PID of the process being queried.
         * @return True if class instance is to be deleted (freed) after
         * the process has been killed or finished executing.
         */
        virtual bool process_delete_when_finished(const executor::PID pid);

        /**
         * Called only after process returns PROCESS_ERROR.
         * @param pid[in] The PID of the process for the error text.
         * @return If process_execute() indicates the process errored out, this
         * will return the reason(s) for the error.
         */
        virtual ErrorMessageText process_get_error_text(
            const executor::PID pid);

        /**
         * Called when the process has been forcibly killed.
         * This will never be called when process_execute() is active.
         * This may be called before any execute() call has been made, if
         * the system is shutting down.
         * @param pid[in] The PID of the process being killed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         */
        virtual void process_killed(
            const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Called when the the executor has finished with the process.  This is
         * called at some point after process_execute() has returned PROCESS_FINISHED
         * or PROCESS_ERROR.  It is also called after process_killed().
         * Typically this is where any resources are cleaned up.
         * If process_delete_when_finished() returns true, this is the last
         * call before destruction of the class instance.
         * @param pid[in] The PID of the finished process.
         */
        virtual void process_finished(const executor::PID pid);

    private:

        /**
         * Subscribes to all needed events,
         */
        void subscribe_events(void);

        /**
         * Handles a connection (includes connects and disconnects) event.
         * @param connect_event_ptr[in] The pointer to the event.  If null,
         * nothing happens.
         */
        void process_connection_event(
            events::ConnectionEvent * const connect_event_ptr);

        /**
         * Handles entity change events.
         * @param connect_event_ptr[in] The pointer to the event.  If null,
         * nothing happens.
         */
        void process_entity_event(
            events::EntityChangedEvent * const entity_event_ptr);

        executor::PID my_pid; ///< Our PID.
        dbtype::Entity::IdSet online_players; ///< Currently online players
    };
}
}

#endif //MUTGOS_USERAGENT_ENTITYNAMEMANAGER_H
