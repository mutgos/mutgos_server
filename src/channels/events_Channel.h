/*
 * events_Channel.h
 */

#ifndef MUTGOS_EVENTS_CHANNEL_H
#define MUTGOS_EVENTS_CHANNEL_H

#include <string>
#include <vector>
#include <map>
#include <boost/thread/recursive_mutex.hpp>

#include "osinterface/osinterface_OsTypes.h"
#include "concurrency/concurrency_LockableObject.h"
#include "executor/executor_ProcessResource.h"
#include "events_ChannelFlowMessage.h"

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class ChannelControlListener;
    class ChannelMessage;

    /**
     * Base abstract class for all channels.  It has the common methods, the
     * process resource listener, and the type of the subclass.  This class
     * and all subclasses are completely thread safe.
     *
     * Channels are a one way data flow composed of a sender and receiver.
     * Items are sent and flow control is done by called methods directly
     * on the Channel class.  Items or state changes can be received either
     * via a callback (any class) or a message (process).  There can be
     * multiple senders, but only one receiver.  By default, no receiver is
     * specified, meaning items disappear upon being sent (like /dev/null).
     *
     * By default the flow control is blocked to allow for proper setup
     * before allowing items through.
     *
     * Both the sender and receiver get callbacks (or messages)
     * when the state changes.
     *
     *
     * Note that there are serious deadlock issues to consider when using this
     * class if the following situations are true:
     *   You get callbacks from this Channel.
     *   You call methods on the Channel.
     *   Your class uses a mutex to lock itself when calling methods on the
     *     Channel.
     * In this situation, you must use the WriteToken to lock the Channel
     * BEFORE locking your own mutex, prior to calling any methods on
     * the Channel.  If this is not done, a deadlock is likely to result
     * at some point.
     *
     *
     * While it is ideal for only one class to keep a pointer to a channel,
     * this is generally impossible.  Therefore, Channels handle pointer
     * ownership as follows:
     *   Channels must only be constructed on the heap (pointers), never as
     *   stack variables.
     *
     *   'delete' should never be directly used on a Channel.
     *
     *   Channel employs basically a reference counting system to know who
     *   holds a pointer to it.  When all references are released, the
     *   Channel will automatically delete itself.
     *
     *   If no one initially registers as a listener or indicates it is
     *   referencing the Channel, it will not delete itself.  This is to
     *   prevent the Channel from deleting itself immediately upon construction.
     *
     *   A Channel is referenced by a class when it registers as a
     *   listener or explicitly indicates it is holding a reference.
     *
     *   Before using the channel (opening, closing, sending messages, locking,
     *   etc), at least one class must explicitly reference the Channel either
     *   via pointers (listener, etc) or as a resource.  If this is not done,
     *   the Channel may auto-delete.
     *
     *   TODO Consider boost shared_ptr in the future, depending on how many need direct Channel access.
     *   TODO The pointer ownership is too complicated and might be reinventing the wheel.
     */
    class Channel : public executor::ProcessResource,
                    public concurrency::LockableObject
    {
    public:
        // TODO eventually convert this to a separate enum with conversions to/from string.  Also update ChannelStatusChange when that happens
        enum ChannelType
        {
            /** This is a text channel (ExternalText). */
            CHANNEL_TYPE_TEXT,
            /** This is a client data channel */
            CHANNEL_TYPE_CLIENT_DATA
        };

        /**
         * Called when process has added this resource to its blocking
         * list.  This may be called multiple times!  It must be thread safe.
         * The same PID and RID combination may also be called multiple times
         * if there is a programming error; this should be gracefully handled.
         * In the context of channels, this is used to indicate a process is
         * sending or receiving on this channel.
         * @param process_id[in] The process ID that has added this resource.
         * Each process can add this resource only once.
         * @param resource_id[in] The unique identifier for this resource as
         * assigned to the given process.  If a different process adds this
         * resource, another unique resource_id will be provided.
         * @return True if this resource could add the process.  If false
         * this means the resource could not add the process (Only one receiver
         * at a time).
         */
        virtual bool resource_added_to_process(
            const executor::PID process_id,
            const executor::RID resource_id);

        /**
         * Called when a process has removed this resource from its blocking
         * list, because either the process has ended (automatic cleanup) or
         * the process manually removed it.  If the resource was not
         * successfully added, this will not get called for that resource ID.
         * It must be thread safe.
         * In the context of channels, this is used to indicate a process is
         * no longer sending or receiving on this channel, and will therefore
         * automatically close the channel.
         * @param process_id[in] The process ID that has removed this resource.
         * @param resource_id[in] The resource ID (previously added) that has
         * been removed.
         * @param process_cleanup[in] If true, removal was due to automatic
         * cleanup due to the process ending or being killed.
         */
        virtual void resource_removed_from_process(
            const executor::PID process_id,
            const executor::RID resource_id,
            const bool process_cleanup);

        /**
         * Indicates that the next resource_added_to_process() will be to add
         * the receiver.  Calling this method does not guarantee the add will
         * succeed.
         *
         * Using the process ID, the class can differentiate multiple resource
         * add calls to determine which is the receiver.  Once the reciever
         * has been added, if the same process again adds the resource it will
         * be as the sender.
         * @param process_id[in] The process ID of the receiver process.
         */
        void next_resource_add_is_receiver(const executor::PID process_id);

        /**
         * @return The channel name.
         */
        const std::string &get_channel_name(void) const
          { return channel_name; }

        /**
         * @return The channel type.
         */
        const ChannelType get_channel_type(void) const
          { return channel_type; }

        /**
         * @return The channel subtype.
         */
        const std::string &get_channel_subtype(void) const
          { return channel_subtype; }

        /**
         * @return True if channel is temporarily blocked.
         */
        bool channel_is_blocked(void);

        /**
         * @return True if channel is permanently closed.
         */
        bool channel_is_closed(void);

        /**
         * Closes the channel.  No new items can be placed on the channel.
         * Once closed, the channel cannot be reopened.
         * If the channel is already closed, nothing will happen.
         * Listeners and processes will be notified of the closure.
         */
        void close_channel(void);

        /**
         * Temporarily blocks the channel.  No new items may be placed on the
         * channel until it is unblocked.  If the channel is already blocked
         * or closed, nothing will happen.
         * Listeners and processes will be notified of the block.
         */
        void block_channel(void);

        /**
         * Unblocks a channel, allowing for items to be sent over it again.
         * This may be called when the channel is already unblocked to
         * change the number of allowed items.
         * Listeners and processes will be notified of the unblock, but not
         * if the allowed items count changes.
         * @param allowed_items[in] How many items may be sent before the
         * channel automatically blocks, or 0 for unlimited.
         * @return True if successfully unblocked or updated the allowed
         * items count, false if failure (channel already closed, etc).
         */
        bool unblock_channel(const MG_UnsignedInt allowed_items = 0);

        /**
         * Registers the given pointer as a listener.  Multiple listeners are
         * allowed.
         * This class will never own the pointer.
         * @param listener_ptr[in] The listener to register.
         * @return True if successfully registered, false if null pointer.
         */
        bool channel_register_control_listener(
            ChannelControlListener * const listener_ptr);

        /**
         * Unregisters the given control listener.
         * After unregistration, consider the pointer to the Channel invalid.
         * Do not reregister after unregistering.
         * @param listener_ptr[in] The listener to unregister.
         */
        void channel_unregister_control_listener(
            ChannelControlListener * const listener_ptr);

        /**
         * Registers a non-listener class instance as holding a pointer to
         * this Channel.  It is safe to call it more than once for the same
         * pointer, but it will be added multiple times.  This is to account
         * for listening as multiple interfaces.
         * @param ptr[in] The class instance holding a pointer to this Channel.
         * @return True if successfully registered, false if null pointer.
         */
        bool channel_register_pointer_holder(void * const ptr);

        /**
         * Unregisters a class holding a pointer to this Channel.
         * After unregistration, consider the pointer to the Channel invalid
         * unless you are positive you have multiple registrations (or
         * listener interfaces) remaining.
         * Do not reregister after unregistering.
         * @param ptr[in] The class instance no longer holding a pointer to
         * this Channel.
         */
        void channel_unregister_pointer_holder(void * const ptr);

        /**
         * Locks this object for exclusive (read/write) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock(void);

        /**
         * Attempts to lock this object for exclusive (read/write) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock(void);

        /**
         * Not supported by this class, will use try_lock()
         */
        virtual bool try_lock_shared(void);

        /**
         * Not supported by this class, will use lock()
         */
        virtual bool lock_shared(void);

        /**
         * Unlocks this object from an exclusive lock.  Only call if lock()
         * was used!
         * @return True if success.
         */
        virtual bool unlock(void);

        /**
         * Not supported by this class, will use unlock()
         */
        virtual bool unlock_shared(void);

    protected:
        /** Vector of channel control listener pointers.  Assumes 2 max */
        typedef std::vector<ChannelControlListener *> ChannelControlListeners;
        /** Vector of non-listener holders of the pointer.  Assumes 2-3 max */
        typedef std::vector<void *> PointerHolders;

        /**
         * Base class constructor.
         * @param name[in] The name of the channel.
         * @param type[in] The type of the subclass.
         * @param subtype[in] The user-defined subtype of the channel
         * (optional).
         */
        Channel(
            const std::string &name,
            const ChannelType type,
            const std::string &subtype = "");

        /**
         * Required virtual destructor.
         */
        virtual ~Channel();

        /**
         * Used to close the channel without initiating a delete.  This
         * is used by other methods that perform a delete after they are
         * totally done with their other functions.
         * @return True if Channel needs to be deleted.
         */
        bool internal_close_channel(void);

        /**
         * Given a channel message, send it to the associated receiver process,
         * if it was added via resources_added_to_process().  If it was not
         * added, an error will be logged and the message will not be sent.
         * @param message_ptr[in] The message to send.  Control of the pointer
         * will pass to this method, success or fail.  The message will
         * have the name of the channel set by this method.
         * @return True if success, false if no process receiver or null
         * pointer.
         */
        bool channel_send_to_receiver(ChannelMessage * const message_ptr);

        /**
         * @return True if the receiver end of the channel is a process, that
         * is, a message must be sent.  If false, a callback may be needed
         * instead.
         */
        bool channel_receiver_is_process(void);

        /**
         * @return True if a callback for the listener has been
         * registered.  Used to make sure both a process and callback aren't
         * both registered as a receiver listener.
         */
        virtual bool receiver_callback_registered(void) =0;

        /**
         * This performs bookkeeping of number of messages sent, alerts
         * listeners of state changes, etc, and therefore must be called
         * just before sending an item.
         * @return True if an item can be sent along the channel, or false
         * if the channel is blocked or closed and therefore the item
         * cannot be sent.
         */
        bool channel_about_to_send_item(void);

        /**
         * Assumes locking has already been performed.
         * @return True if delete_instance() should be called.
         */
        bool need_delete_instance(void);

        /**
         * If no one is referring to this Channel, delete its instance,
         * otherwise do nothing.  It should be always assumed that
         * this call will result in the deletion of the instance.
         */
        void delete_instance(void);

        /** Pair of PID and RID */
        typedef std::pair<executor::PID, executor::RID> PidRid;
        /** Array of PidRid pair.  While arrays are inefficient for search
            and removal, the number of entries is expected to be small (< 3) */
        typedef std::vector<PidRid> PidRidArray;

        boost::recursive_mutex channel_mutex; ///< Lock for any Channel operations.

        const std::string channel_name;
        const std::string channel_subtype;
        const ChannelType channel_type;

        bool channel_callback_in_progress; ///< True if callback in progress that should delay channel deletion

    private:

        /**
         * Checks to see if the channel's state (flow) has changed from
         * the last time listeners were called and ChannelFlowMessages
         * were sent.  If it has, inform the listeners of the change.  If
         * no change detected, do nothing.
         * This method assumes locking has been performed.
         */
        void check_status(void);

        /**
         * Broadcasts the current channel status (channel_last_status) to
         * all callback listeners and processes.
         * This method assumes locking has been performed.
         */
        void broadcast_status(void);

        /**
         * This method assumes locking has been performed.
         * @return A new channel flow message with all attributes set.
         * The status is based on channel_last_status.
         */
        ChannelFlowMessage *make_channel_flow_message(void);

        // No copying
        //
        Channel(const Channel &rhs);
        Channel &operator=(const Channel &rhs);

        PidRidArray channel_send_processes; ///< Lists all sender processes
        executor::PID channel_recv_pid; ///< PID on receiving end, if any.
        executor::RID channel_recv_rid; ///< RID on receiving end, if any.

        ChannelControlListeners channel_control_listeners; ///< Status listeners
        PointerHolders pointer_holders; ///< Non-listeners that hold pointer

        MG_UnsignedInt channel_items_remaining; ///< Items left until block
        bool channel_unlimited_items; ///< If true, unlimits items allowed
        bool channel_blocked; ///< True if channel is currently blocked
        bool channel_closed; ///< True if channel is closed
        MG_UnsignedInt channel_external_locked_count; ///< How many have locked it using token
        executor::PID channel_resource_add_pid; ///< Next add from this PID is recv
        ChannelFlowMessage::ChannelFlowStatus
            channel_last_status; ///< Last status sent out, to avoid duplicates
    };
}
}

#endif //MUTGOS_EVENTS_CHANNEL_H
