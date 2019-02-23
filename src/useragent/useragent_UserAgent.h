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

#include "security/security_Context.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class TextChannel;
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
     * The UserAgent is what the user types commands into so they can move
     * through exits, run programs, etc.  It provides very few built-in
     * commands; the rest are done via softcode.  It is also responsible for
     * creating the initial I/O channels, I/O redirection, subscribing to room
     * and direct events (private messages), and handling logout.
     *
     * The UserAgent is spawned by the global LoginManager process.
     *
     * This is currently only usable for non-enhanced clients (text only).
     */
    class UserAgent : public executor::Process
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
         * Called when we get a ChannelFlowMessage.  This will process the
         * message contents.
         * @param flow_event_ptr[in] The ChannelFlowMessage.  If null, nothing
         * will happen.
         * @return True if determined the program must terminate immediately.
         */
        bool process_channel_flow(
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
        void subscribe_events(void);

        /**
         * The main method responsible for parsing the actual input
         * from the user.  This will also update the context as needed.
         * @param command_str[in, out] The line entered by user, converted
         * from external text to the internal format.  May be modified
         * as it is parsed.
         * @return True if continue processing, false if disconnect needed.
         */
        bool process_user_command(std::string &command_str);

        /**
         * Given an Entity ID that is an action, this will either move the
         * context requester through an exit or run a program (optionally with
         * redirection).
         * @param action_id[in] The ID of the action to execute.
         * @param arguments[in,out] Arguments to the action.  May be modified
         * to remove redirect info.
         */
        void process_action(
            const dbtype::Id &action_id,
            std::string &arguments);

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


        bool first_execute; ///< True if this is the first execute call.

        dbtype::TimeStamp last_context_refresh; ///< The last time we refreshed our capabilities.
        security::Context my_context; ///< Security context for this process
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
    };
}
}

#endif //MUTGOS_USERAGENT_USERAGENT_H
