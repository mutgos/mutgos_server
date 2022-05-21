/*
 * useragent_PuppetManager.h
 */

#ifndef MUTGOS_USERAGENT_PUPPETMANAGER_H
#define MUTGOS_USERAGENT_PUPPETMANAGER_H

#include <map>
#include <string>

#include "dbtypes/dbtype_Id.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ProcessServices.h"

#include "events/events_EventAccess.h"

namespace mutgos
{
namespace events
{
// Forward declarations
class EntityChangedEvent;
}

namespace useragent
{
    /**
     * The PuppetManager is what spawns off the puppet agent for
     * existing puppets upon login, or newly created puppets as they
     * are created.  It will also route old-style commands for puppet
     * control to the right puppet agent.
     *
     * The PuppetManager is spawned by the UserAgent process.
     */
    class PuppetManager : public executor::Process
    {
    public:
        /**
         * Constructor.
         * @param player[in] The ID of the newly connected player.
         * @param player_pid[in] The PID of the player's agent.
         */
        PuppetManager(
            const dbtype::Id &player,
            const executor::PID player_pid);

        /**
         * Required virtual destructor.
         */
        virtual ~PuppetManager();

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
         * Performs 'first run' tasks like starting up all the puppet
         * agents.  Sets the first run flag to false after.
         */
        void do_first_run(void);

        /**
         * Handles an entity change event.
         * @param subscription_id[in]
         * @param event[in] The event to process.
         */
        void process_entity_change_event(
            const events::SubscriptionId subscription_id,
            events::EntityChangedEvent * const event);

        /**
         * Creates the puppet agent for the given puppet, if it doesn't
         * already exist.
         * @param puppet_id[in] The ID of the puppet whose agent is to be
         * created.
         */
        void spawn_puppet(const dbtype::Id &puppet_id);

        /**
         * Kills the agent for the given puppet ID.
         * @param puppet_id[in] The ID of the puppet whose agent process is
         * to be killed.
         */
        void kill_puppet(const dbtype::Id &puppet_id);

        /**
         * Kills all puppet agents associated with the owner.
         */
        void kill_all_puppets(void);

        /**
         * Sends an error message back to the owner's agent.
         * @param message[in] The error message to send.
         */
        void send_error_message(
            const dbtype::Id &puppet_id,
            const std::string &message);

        /**
         * Sends a command to a puppet's agent to process (such as movement,
         * 'say', 'page', etc).
         * @param puppet_id[in] The puppet to send the command to.
         * @param message[in] The command/message to send.
         * @return True if success.  False if puppet agent not found, etc.
         */
        bool send_puppet_message(
            const dbtype::Id &puppet_id,
            const std::string &message);

        /**
         * Sends a 'ping' message to the puppet's agent, to trigger it to
         * open needed channels, recheck ownership, etc.
         * @param puppet_id[in] The ID of the puppet whose agent is to be
         * pinged.
         * @return True if success.  False if puppet agent not found, etc.
         */
        bool send_puppet_ping(const dbtype::Id &puppet_id);


        typedef std::map<dbtype::Id, executor::PID> PuppetMap;

        const dbtype::Id player_id; ///< The player entity this agent is managing puppets for
        const executor::PID player_agent_pid; ///< PID of Player's agent
        executor::PID my_pid; ///< Our PID.
        events::SubscriptionId create_delete_sub_id; ///< Subscription for knowing when our puppets are created or deleted
        events::SubscriptionId update_sub_id; ///< Subscription for knowing when a puppet is chowned to us
        PuppetMap puppet_map; ///< Maps puppet entity ID to the PID for its agent

        bool first_run; ///< True if first time executor calls us
        std::string error_message; ///< When not empty, indicates
    };
}
}

#endif //MUTGOS_USERAGENT_PUPPETMANAGER_H
