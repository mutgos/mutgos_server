/*
 * useragent_PuppetAgent.h
 */

#ifndef MUTGOS_USERAGENT_PUPPETAGENT_H
#define MUTGOS_USERAGENT_PUPPETAGENT_H

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
 * TODO
 * >Start implementation of event processor.  Detect chown
 *
 * >override event processor if message, to create channels if none exist
 * >Override event processor to handle 'ping' and 'commands'
 * > override process_channel_flow() to ignore closed streams - include process services in call
 *
 * >delete creation of context, channels, and move to subclass for player
 * >make major methods virtual and protected
 * >Create a 'quit processor' so instead of disconnecting it would just close the streams
 * >Override process_exit() on UserAgent to check for what to send to puppet
 *
 * Look into how it affects final cleanup
 *
 * Fix telnet and websocket to properly support.  Telnet needs to periodically close non-used streams
 *
 * May need to override message processing to also check for command or wakeup message
 */

    /**
     * The PuppetAgent is what handles commands sent to puppets  so they can
     * move through exits, run programs, etc.  It provides very few built-in
     * commands; the rest are done via softcode.  It is also responsible for
     * creating the initial I/O channels, I/O redirection, subscribing to room
     * and direct events (private messages), and handling logout.
     *
     * The PuppetAgent is spawned by the global PuppetManager process.
     */
    class PuppetAgent : public Agent
    {
    public:
        /**
         * Constructor.
         * @param player[in] The ID of the newly connected player.
         * @param puppet[in] The ID of the puppet this is the agent for.
         * @param manager[in] The PID of the Puppet Manager which spawned
         * this instance.
         */
        PuppetAgent(
            const dbtype::Id &player,
            const dbtype::Id &puppet,
            const executor::PID manager);

        /**
         * Required virtual destructor.
         */
        virtual ~PuppetAgent();

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
         * Subclasses may wish to override this to do any first-time connect
         * stuff like splash screens.
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
         * Similar to the standard process_execute(), this variant is called
         * in response to a message to be processed, originating from a
         * resource.
         * For us, this would be called when something happens to one of our
         * I/O channels.
         * @param pid[in] The PID of the process being executed.
         * @param services[in] Provides a means for the Process to
         * interact with the executor.
         * @param rid[in] The RID of the resource who sent the message.
         * @param message[in] The message to be processed.
         * @return The status indicating if process has completed its work.
         * @see process_execute(const PID pid);
         */
        virtual ProcessStatus process_execute(
            const executor::PID pid,
            executor::ProcessServices &services,
            const executor::RID rid,
            executor::ProcessMessage &message);

        /**
         * This might be called many times, even while the process is
         * executing.
         * @param pid[in] The PID of the process whose name is desired.
         * @return The name of the process.  Must not be empty.
         */
        virtual std::string process_get_name(const executor::PID pid);

    protected:
        /**
         * Called once when process is added, this allows for
         * setting up channels, context (if needed immediately), etc.
         * @param pid[in] The PID of the newly added Process.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         */
        virtual void setup(
            const executor::PID pid,
            executor::ProcessServices &services);

        /**
         * Called when we get a ChannelFlowMessage.  This will process the
         * message contents.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         * @param flow_event_ptr[in] The ChannelFlowMessage.  If null, nothing
         * will happen.
         * @return True if determined the program must terminate immediately.
         */
        virtual bool process_channel_flow(
            executor::ProcessServices &services,
            events::ChannelFlowMessage * const flow_event_ptr);

        /**
         * Overrides processing an action to insert puppet info to channel.
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
         * Creates an output channel for use by a launched process.
         * @param subtype[in] The subtype of the channel.
         * @return The newly created channel.  This never returns null.
         * Caller manages pointer.
         */
        virtual events::TextChannel *make_prog_output_channel(
            const std::string &subtype);

        /**
         * Processes the 'QUIT' command.  Puppets just close their channels.
         * @return True if don't disconnect (continue processing), or
         * false to disconnect the client.  This always returns true for
         * this class.
         */
        virtual bool process_quit_command(void);

        /**
         * Creates the channels, security context, and certain subscriptions
         * if not already created.  Basically, this 'activates' the puppet.
         * @param[in] services Process Services to associate channels to
         * process.
         * @return True if process should keep running, false if puppet
         * is no longer controlled by us, error, etc.
         */
        bool activate(executor::ProcessServices &services);

        /**
         * Closes all open channels, clears out the security context, and
         * unsubscribes from certain subscriptions.  Basically, this
         * 'deactivates' the puppet.
         * @param[in] services Process Services to disassociate channels
         * to process.
         */
        void deactivate(executor::ProcessServices &services);

        /**
         * Sends a shutdown status message to the puppet manager.
         */
        void send_shutdown_status(void);

        /**
         * Called by refresh_context() when the context has been
         * created/updated/refreshed, this will allow the puppet agent
         * to add certain inherited capabilities to the context.
         */
        virtual void modify_refreshed_context(void);

        /**
         * Takes a newly created context used to execute a program and
         * makes any modifications to it before populating it with data
         * from the security subsystem.  This is used primarily when
         * capabilities and such need to be artificially inherited, like for
         * puppets.
         * @param program_context[out] The program context to modify;
         */
        virtual void modify_program_context(security::Context &program_context);

        const executor::PID manager_pid; ///< Puppet Managet PID

        events::SubscriptionId chown_id; ///< Subscription for detecting chowning
    };
}
}

#endif //MUTGOS_USERAGENT_PUPPETAGENT_H
