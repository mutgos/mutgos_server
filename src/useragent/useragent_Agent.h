/*
 * useragent_Agent.h
 */

#ifndef MUTGOS_USERAGENT_AGENT_H
#define MUTGOS_USERAGENT_AGENT_H

#include <string>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_TimeStamp.h"
#include "dbinterface/dbinterface_EntityRef.h"
#include "executor/executor_Process.h"
#include "executor/executor_ProcessMessage.h"

#include "events/events_EventAccess.h"

#include "clientmessages/message_ClientExecuteEntity.h"
#include "clientmessages/message_ClientFindEntityRequest.h"

#include "security/security_Context.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class TextChannel;
    class ClientDataChannel;
    class EmitEvent;
    class MovementEvent;
    class ChannelFlowMessage;
}
namespace dbtype
{
    // Forward declarations
    //
    class DocumentProperty;
}
namespace useragent
{
    // Forward declarations
    //
    class TextChannelDocumentWriter;

    /**
     * Base Agent code shared by all agents (player and puppet).
     * The Agent is what the user types commands into so they can move
     * through exits, run programs, etc.  It provides very few built-in
     * commands; the rest are done via softcode.  It is also responsible for
     * I/O redirection, subscribing to room and direct events (private
     * messages), among other features.
     */
    class Agent : public executor::Process
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~Agent();

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

    protected:
        /**
         * Constructor when session comms owner is one and the same.
         * @param entity[in] The ID of the Entity this will manage.
         */
        Agent(const dbtype::Id &entity);

        /**
         * Constructor when session comms owner is different from the Entity
         * we're the agent for (such as a Puppet).
         * @param entity[in] The ID of the Entity this will manage.
         * @param session_entity[in] The ID of the session owner for entity.
         */
        Agent(const dbtype::Id &entity, const dbtype::Id &session_entity);

        /**
         * Called once when process is added, this allows for
         * setting up channels, context (if needed immediately), etc.
         * @param pid[in] The PID of the newly added Process.
         * @param services[in] Provides a means for the Process to interact
         * with the executor.
         */
        virtual void setup(const executor::PID pid,
            executor::ProcessServices &services) =0;

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
         * Called when we get a MovementEvent.  This will process the message
         * contents, resubscribing as needed.
         * @param movement_event_ptr[in] The MovementEvent.  If null, nothing
         * will happen.
         */
        void process_location_change(
            events::MovementEvent * const movement_event_ptr);

        /**
         * Called when we get an EmitEvent (room messages and private messages).
         * This will process the message contents, sending them to the output
         * Channel as needed.
         * @param subscription_id[in] The subscription ID that was triggerd.
         * @param emit_event_ptr[in] The EmitEvent.  If null, nothing will
         * happen.
         */
        void process_emit(
            const events::SubscriptionId subscription_id,
            events::EmitEvent * const emit_event_ptr);

        /**
         * Subscribes to all needed events, including events based on where
         * we're currently located.
         */
        virtual void subscribe_events(void);

        /**
         * The main method responsible for parsing the actual input
         * from the user.  This will also update the context as needed.
         * @param command_str[in, out] The line entered by user.
         * May be modified as it is parsed.
         * @return True if continue processing, false if disconnect needed.
         */
        virtual bool process_user_command(std::string &command_str);

        /**
         * Processes the 'QUIT' command.  Players would disconnect,
         * puppets would just close, for instance.
         * @return True if don't disconnect (continue processing), or
         * false to disconnect the client.
         */
        virtual bool process_quit_command(void) =0;

        /**
         * Given an Entity ID that is an action, this will either move the
         * context requester through an exit or run a program (optionally with
         * redirection).
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
         * Given an exit, perform the work indicated by the exit.
         * This could be either going through an exit to another room,
         * or if overridden could be used to control a puppet, for
         * instance.
         * @param exit_ref[in] Reference to the exit.
         * @param arguments[in] Arguments for the exit.  Normally
         * not used.
         */
        virtual void process_exit(
            dbinterface::EntityRef &exit_ref,
            std::string &arguments);

        /**
         * Given a ClientExecuteEntity message, process it and execute
         * the entity if authorized.
         * @param message[in] Details of what to execute.
         */
        virtual void process_execute_entity(
            const message::ClientExecuteEntity &message);

        /**
         * Given a ClientMatchNameRequest message that searches by name,
         * process it and send back the resulting matches.
         * @param message[in] Details of what to search for.
         */
        void process_match_name(
            const message::ClientFindEntityRequest &message);

        /**
         * Given a ClientMatchNameRequest message that searches only by type,
         * process it and send back the resulting matches.
         * @param message[in] Details of what to search for.
         */
        void process_match_type(
            const message::ClientFindEntityRequest &message);

        /**
         * If it has permission, list the program source code and send
         * to the output channel.
         * @param program_id_str[in] The program to list as a string (must be
         * ID and not name).
         */
        void process_list_program(const std::string &program_id_str);

        /**
         * Processes the 'edit program' command and its parameters.  If
         * successful, the agent will enter the program editing mode.
         * @param program_id_str[in] The argument to the 'edit program'
         * command, which currently is just the ID of the program to edit.
         */
        void process_enter_edit_program(const std::string &program_id_str);

        /**
         * Process a command to add a line or finish editing a program, which
         * assumes that editing is currently in progress.
         * The program will only be saved if it has permission.
         * @param input[in] The input to process (add line, save edits).
         */
        void process_edit_program(const std::string &input);

        /**
         * Forcibly calls the 'look' command.  Used when connecting or
         * changing rooms.
         */
        void force_look(void);

        /**
         * Creates an output channel for use by a launched process.
         * @param subtype[in] The subtype of the channel.
         * @return The newly created channel.  This never returns null.
         * Caller manages pointer.
         */
        virtual events::TextChannel *make_prog_output_channel(
            const std::string &subtype);

        /**
         * Sends information to the enhanced a client about the location
         * change of their Entity.
         * @param new_container[in] The new container (usually a room)
         * the client's Entity has moved to.
         */
        void send_location_update(const dbtype::Id &new_container);

        /**
         * Sends unformatted text to the output channel.
         * @param text[in] The line of text to send.
         * @param text_is_error[in] If true, text will be sent with a color to
         * indicate an error, otherwise it will be the default color.
         */
        void send_plain_text(
            const std::string &text,
            const bool text_is_error = false);

        /**
         * Refreshes the context's capabilities if needed.
         * @param force[in] Optional.  Will make it do an update even if not
         * yet time.
         */
        void refresh_context(const bool force = false);

        /**
         * Called by refresh_context() when the context has been
         * created/updated/refreshed, this optional method will allow any
         * additional modifications to the context by subclasses.
         */
        virtual void modify_refreshed_context(void);

        /**
         * This optional method takes a newly created context used to execute
         * a program and makes any modifications to it before populating it
         * with data from the security subsystem.  This is used primarily when
         * capabilities and such need to be artificially inherited, like for
         * puppets.
         * @param program_context[out] The program context to modify;
         */
        virtual void modify_program_context(security::Context &program_context);

        bool first_execute; ///< True if this is the first execute call.
        const bool is_player; ///< True if entity_id is a Player object.

        const dbtype::Id entity_id; ///< The entity this agent is managing.
        const dbtype::Id session_entity_id; ///< The entity who owns comms sessions

        dbtype::TimeStamp last_context_refresh; ///< The last time we refreshed our capabilities.
        security::Context *my_context_ptr; ///< Security context for this process; may be null until entity is 'activated'.
        executor::PID my_pid; ///< Our PID.

        dbtype::Id program_source_id; ///< If editing program, this is the program's ID.
        dbtype::DocumentProperty *program_source_ptr; ///< If editing program, this is what's being edited.

        events::SubscriptionId location_subscription_id; ///< Subscription for knowing when we move
        events::SubscriptionId emit_subscription_id; ///< Subscription for emits from room
        events::SubscriptionId private_subscription_id; ///< Subscription for private messages

        events::TextChannel *output_channel_ptr; ///< Pointer to outgoing channel
        executor::RID output_rid; ///< Output channel RID
        events::TextChannel *input_channel_ptr; ///< Pointer to incoming channel
        executor::RID input_rid; ///< Output channel RID

        events::ClientDataChannel *data_output_channel_ptr; ///< Pointer to outgoing data channel, if enhanced
        executor::RID data_output_rid; ///< Data input channel RID
        events::ClientDataChannel *data_input_channel_ptr; ///< Pointer to incoming data channel, if enhanced
        executor::RID data_input_rid; ///< Data input channel RID

    private:
        /**
         * Checks security, and if allowed will create the given player
         * in the current location.
         * format of: player_name password
         * TODO This is a proof of concept temporary function for a demo.
         *      This will be removed and replaced by softcode later.
         * @param params[in] The player name and password.
         */
        void create_player(const std::string &params);

        /**
         * Checks security, and if allowed will create the given puppet
         * in the current location.
         * format of: player_name puppet_name puppet_command
         * TODO This is a proof of concept temporary function for a demo.
         *      This will be removed and replaced by softcode later.
         * @param params[in] The owning player, puppet name, and password.
         */
        void create_puppet(const std::string &params);
    };
}
}

#endif //MUTGOS_USERAGENT_AGENT_H
