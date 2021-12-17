/*
 * useragent_UserAgent.h
 */

#ifndef MUTGOS_USERAGENT_USERAGENT_H
#define MUTGOS_USERAGENT_USERAGENT_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessMessage.h"

#include "events/events_EventAccess.h"

#include "clientmessages/message_ClientExecuteEntity.h"
#include "clientmessages/message_ClientFindEntityRequest.h"

#include "useragent/useragent_Agent.h"

#include "security/security_Context.h"

namespace mutgos
{
namespace useragent
{
    /**
     * The UserAgent is what the user types commands into so they can move
     * through exits, run programs, etc.  It provides very few built-in
     * commands; the rest are done via softcode.  It is also responsible for
     * creating the initial I/O channels, I/O redirection, subscribing to room
     * and direct events (private messages), and handling logout.
     *
     * The UserAgent is spawned by the global LoginManager process.
     */
    class UserAgent : public Agent
    {
    public:
        /**
         * Constructor.
         * @param player[in] The ID of the newly connected player.
         */
        UserAgent(const dbtype::Id &player);

        /**
         * Required virtual destructor.
         */
        virtual ~UserAgent();

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
         * Called when the the executor has finished with the process.  This is
         * called at some point after process_execute() has returned PROCESS_FINISHED
         * or PROCESS_ERROR.  It is also called after process_killed().
         * Typically this is where any resources are cleaned up.
         * If process_delete_when_finished() returns true, this is the last
         * call before destruction of the class instance.
         * This will kill the puppet manager process.
         * @param pid[in] The PID of the finished process.
         */
        virtual void process_finished(const executor::PID pid);

    protected:
        /**
         * Called once when process is added, this allows for
         * setting up channels, context (if needed immediately), etc.
         * @param pid[in] The PID of the newly added Process.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         */
        virtual void setup(const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Overrides processing an action to handle puppet control.
         * @param action_id[in] The ID of the action to execute.
         * @param channel_subtype[in] The output and input channels subtype,
         * if the action results in running a program that has output.  Used
         * primarily by enhanced clients to identify that a newly opened
         * Channel is the result of a prior request.
         * @param arguments[in,out] Arguments to the action.  May be modified
         * to remove redirect info.
         */
        virtual void process_action(
            const dbtype::Id &action_id,
            const std::string &channel_subtype,
            std::string &arguments);

        /**
         * Processes the 'QUIT' command.  Players would disconnect,
         * puppets would just close, for instance.
         * @return True if don't disconnect (continue processing), or
         * false to disconnect the client.
         */
        virtual bool process_quit_command(void);

    private:

        executor::PID puppet_manager_pid; ///< PID for spawned puppet manager
    };
}
}

#endif //MUTGOS_USERAGENT_USERAGENT_H
