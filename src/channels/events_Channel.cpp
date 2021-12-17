/*
 * events_Channel.cpp
 */

#include <string>
#include <vector>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "events_Channel.h"
#include "osinterface/osinterface_OsTypes.h"
#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_ProcessResource.h"
#include "events_ChannelControlListener.h"
#include "events_ChannelMessage.h"
#include "events_ChannelFlowMessage.h"

#include "text/text_StringConversion.h"
#include "logging/log_Logger.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    Channel::~Channel()
    {
        // TODO This is obsolete, since by definition, Channel will have no listeners whne being destructed.
        // close_channel();
        // boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);
        // channel_last_status = ChannelFlowMessage::CHANNEL_DESTRUCTED;
        // broadcast_status();
    }

    // ----------------------------------------------------------------------
    bool Channel::resource_added_to_process(
        const executor::PID process_id,
        const executor::RID resource_id)
    {
        bool success = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (process_id == channel_resource_add_pid)
        {
            // The resource is the receiver.  Add if not already added.
            //
            if ((not receiver_callback_registered()) and
                ((not channel_recv_pid) or
                (process_id == channel_recv_pid and
                    resource_id == channel_recv_rid)))
            {
                // Channel not already assigned.
                //
                channel_recv_pid = process_id;
                channel_recv_rid = resource_id;
                success = true;
            }
        }
        else
        {
            // Resource is a sender or other interested listener
            //
            const PidRid pid_rid_pair =
                std::make_pair(process_id, resource_id);

            // Make sure it's not already in the list.
            for (PidRidArray::const_iterator iter =
                    channel_send_processes.begin();
                iter != channel_send_processes.end();
                ++iter)
            {
                if (*iter == pid_rid_pair)
                {
                    // Already in list.
                    success = true;
                    break;
                }
            }

            if (not success)
            {
                channel_send_processes.push_back(pid_rid_pair);
                success = true;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void Channel::resource_removed_from_process(
        const executor::PID process_id,
        const executor::RID resource_id,
        const bool process_cleanup)
    {
        bool need_delete = false;

        // Scope for mutex
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if ((channel_recv_pid == process_id) and
                (channel_recv_rid == resource_id))
            {
                // Receiver is being removed.
                channel_recv_pid = 0;
                channel_recv_rid = 0;
                need_delete = internal_close_channel();
            }
            else
            {
                // See if the process is in the sender list.
                //
                for (PidRidArray::iterator iter =
                    channel_send_processes.begin();
                     iter != channel_send_processes.end();
                     ++iter)
                {
                    if ((iter->first == process_id) and
                        (iter->second == resource_id))
                    {
                        // Found it.  Remove and close the channel.
                        //
                        channel_send_processes.erase(iter);
                        need_delete = internal_close_channel();
                        break;
                    }
                }
            }
        }

        if (need_delete)
        {
            delete_instance();
        }
    }

    // ----------------------------------------------------------------------
    void Channel::next_resource_add_is_receiver(const executor::PID process_id)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        channel_resource_add_pid = process_id;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_is_blocked(void)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        return channel_blocked;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_is_closed(void)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        return channel_closed;
    }

    // ----------------------------------------------------------------------
    void Channel::close_channel(void)
    {
        if (internal_close_channel())
        {
            // In case delete was delayed because of callback.
            delete_instance();
        }
    }

    // ----------------------------------------------------------------------
    void Channel::block_channel(void)
    {
        // Scope for mutex
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if (not channel_closed)
            {
                channel_blocked = true;
                check_status();
            }
        }
    }

    // ----------------------------------------------------------------------
    bool Channel::unblock_channel(const MG_UnsignedInt allowed_items)
    {
        bool success = false;

        // Scope for mutex
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if (not channel_closed)
            {
                channel_blocked = false;
                channel_items_remaining = allowed_items;
                success = true;
                check_status();
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_register_control_listener(
        ChannelControlListener * const listener_ptr)
    {
        bool success = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (listener_ptr)
        {
            // Confirm not already listening.
            //
            for (ChannelControlListeners::const_iterator listener_iter =
                    channel_control_listeners.begin();
                listener_iter != channel_control_listeners.end();
                ++listener_iter)
            {
                if (*listener_iter == listener_ptr)
                {
                    // Already listening.
                    success = true;
                    break;
                }
            }

            if (not success)
            {
                channel_control_listeners.push_back(listener_ptr);
                channel_register_pointer_holder(listener_ptr);
                success = true;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void Channel::channel_unregister_control_listener(
        ChannelControlListener *const listener_ptr)
    {
        bool removed_listener = false;

        // Scope for guard, since we can't be in the mutex when destructing
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if (listener_ptr)
            {
                // Find and remove the listener
                //
                for (ChannelControlListeners::iterator listener_iter =
                        channel_control_listeners.begin();
                     listener_iter != channel_control_listeners.end();
                     ++listener_iter)
                {
                    if (*listener_iter == listener_ptr)
                    {
                        // Found it.
                        channel_control_listeners.erase(listener_iter);
                        removed_listener = true;
                        break;
                    }
                }
            }
        }

        if (removed_listener)
        {
            channel_unregister_pointer_holder(listener_ptr);
        }
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_register_pointer_holder(void * const ptr)
    {
        bool success = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (ptr)
        {
            pointer_holders.push_back(ptr);
            success = true;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void Channel::channel_unregister_pointer_holder(void * const ptr)
    {
        bool need_delete = false;

        // Scope for guard, since we can't be in it while destructing.
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if (ptr)
            {
                // Find and remove the instance
                //
                for (PointerHolders::iterator iter =
                        pointer_holders.begin();
                     iter != pointer_holders.end();
                     ++iter)
                {
                    if (*iter == ptr)
                    {
                        // Found it.
                        pointer_holders.erase(iter);
                        break;
                    }
                }
            }

            // Check while we're still initially locked, to avoid a race
            // condition in deletion.
            need_delete = need_delete_instance();
        }

        if (need_delete)
        {
            delete_instance();
        }
    }

    // ----------------------------------------------------------------------
    Channel::Channel(
        const std::string &name,
        const ChannelType type,
        const std::string &subtype,
        const dbtype::Id &entity_id)
        : executor::ProcessResource(),
          channel_name(name),
          channel_subtype(subtype),
          channel_type(type),
          channel_entity_id(entity_id),
          channel_callback_in_progress(false),
          channel_recv_pid(0),
          channel_recv_rid(0),
          channel_items_remaining(0),
          channel_unlimited_items(true),
          channel_blocked(true),
          channel_closed(false),
          channel_external_locked_count(0),
          channel_resource_add_pid(0),
          channel_last_status(ChannelFlowMessage::CHANNEL_FLOW_BLOCKED)
    {
    }

    // ----------------------------------------------------------------------
    bool Channel::internal_close_channel(void)
    {
        bool needs_delete = false;

        // Scope for mutex
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            channel_closed = true;
            check_status();
            needs_delete = need_delete_instance();
        }

        return needs_delete;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_send_to_receiver(ChannelMessage * const message_ptr)
    {
        bool success = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (message_ptr)
        {
            if (channel_recv_pid > 0)
            {
                // We can send the message
                //
                message_ptr->set_channel_name(channel_name);

                success = executor::ExecutorAccess::instance()->send_message(
                    channel_recv_pid,
                    channel_recv_rid,
                    message_ptr);
            }
            else
            {
                // No one to send the message to.
                delete message_ptr;

                LOG(error, "events", "channel_send_to_receiver",
                    "Unable to send to invalid receiver on channel name "
                      + channel_name);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_receiver_is_process(void)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        return channel_recv_pid > 0;
    }

    // ----------------------------------------------------------------------
    bool Channel::channel_about_to_send_item(void)
    {
        bool can_send = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (channel_last_status == ChannelFlowMessage::CHANNEL_FLOW_OPEN)
        {
            // Channel is open, meaning sending might be possible.
            //

            can_send = channel_unlimited_items;

            if (not can_send)
            {
                // Not unlimited; see if we have any items left.
                if (channel_items_remaining > 0)
                {
                    // We can send. Decrement the count and continue.
                    //
                    --channel_items_remaining;
                    can_send = true;
                }
                else
                {
                    // We've run out.  Change status.
                    //
                    channel_blocked = true;
                    channel_last_status =
                        ChannelFlowMessage::CHANNEL_FLOW_BLOCKED;
                    check_status();
                }
            }
        }

        return can_send;
    }

    // ----------------------------------------------------------------------
    bool Channel::need_delete_instance(void)
    {
        bool need_delete = false;

        if ((not channel_callback_in_progress) and
            (not channel_external_locked_count))
        {
            need_delete = (channel_recv_pid == 0) and
                          (channel_recv_rid == 0) and
                          channel_send_processes.empty() and
                          pointer_holders.empty();
        }

        return need_delete;
    }

    // ----------------------------------------------------------------------
    void Channel::delete_instance()
    {
        bool need_delete = false;

        // Scope for guard, since we can't be in it while destructing.
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);
            need_delete = need_delete_instance();
        }

        if (need_delete)
        {
            delete this;
        }
    }

    // ----------------------------------------------------------------------
    void Channel::check_status(void)
    {
        bool need_callbacks = false;

        if (channel_last_status != ChannelFlowMessage::CHANNEL_DESTRUCTED)
        {
            if ((not channel_unlimited_items) and
                (channel_items_remaining == 0))
            {
                channel_blocked = true;
            }

            if (channel_closed)
            {
                if (channel_last_status !=
                    ChannelFlowMessage::CHANNEL_FLOW_CLOSED)
                {
                    channel_last_status = ChannelFlowMessage::CHANNEL_FLOW_CLOSED;
                    need_callbacks = true;
                }
            }
            else if (channel_blocked)
            {
                if (channel_last_status !=
                    ChannelFlowMessage::CHANNEL_FLOW_BLOCKED)
                {
                    channel_last_status = ChannelFlowMessage::CHANNEL_FLOW_BLOCKED;
                    need_callbacks = true;
                }
            }
            else
            {
                // Must be unblocked
                if (channel_last_status !=
                    ChannelFlowMessage::CHANNEL_FLOW_OPEN)
                {
                    channel_last_status = ChannelFlowMessage::CHANNEL_FLOW_OPEN;
                    need_callbacks = true;
                }
            }
        }

        if (need_callbacks)
        {
            broadcast_status();

            LOG(debug, "events", "check_status",
                "Status of channel changed.  Name: " + channel_name
                + "  Destructed: "
                + text::to_string(
                    channel_last_status == ChannelFlowMessage::CHANNEL_DESTRUCTED)
                + "  Closed: "
                + text::to_string(channel_closed)
                + "  Blocked: "
                + text::to_string(channel_blocked)
                + "  Items remaining: "
                + text::to_string(channel_items_remaining)
                + "  Unlimited items: "
                + text::to_string(channel_unlimited_items));
        }
    }

    // ----------------------------------------------------------------------
    void Channel::broadcast_status(void)
    {
        channel_callback_in_progress = true;

        // Copy listeners to avoid bad iters if vectors are changed during
        // the callback.
        //
        ChannelControlListeners control_listeners(channel_control_listeners);
        PidRidArray send_processes(channel_send_processes);

        if (not control_listeners.empty())
        {
            // First, do all callbacks.
            //
            switch (channel_last_status)
            {
                case ChannelFlowMessage::CHANNEL_FLOW_BLOCKED:
                {
                    for (ChannelControlListeners::iterator iter =
                            control_listeners.begin();
                        iter != control_listeners.end();
                        ++iter)
                    {
                        (*iter)->channel_flow_blocked(channel_name, this);
                    }

                    break;
                }

                case ChannelFlowMessage::CHANNEL_FLOW_OPEN:
                {
                    for (ChannelControlListeners::iterator iter =
                            control_listeners.begin();
                         iter != control_listeners.end();
                         ++iter)
                    {
                        (*iter)->channel_flow_open(channel_name, this);
                    }

                    break;
                }

                case ChannelFlowMessage::CHANNEL_FLOW_CLOSED:
                {
                    for (ChannelControlListeners::iterator iter =
                            control_listeners.begin();
                         iter != control_listeners.end();
                         ++iter)
                    {
                        (*iter)->channel_flow_closed(channel_name, this);
                    }

                    break;
                }

                case ChannelFlowMessage::CHANNEL_DESTRUCTED:
                {
                    for (ChannelControlListeners::iterator iter =
                            control_listeners.begin();
                         iter != control_listeners.end();
                         ++iter)
                    {
                        (*iter)->channel_destructed(channel_name, this);
                    }

                    break;
                }
            }
        }

        // Then do all messages to processes.
        //
        executor::ExecutorAccess * const executor_ptr =
            executor::ExecutorAccess::instance();

        if (channel_recv_pid)
        {
            if (not executor_ptr->send_message(
                channel_recv_pid,
                channel_recv_rid,
                make_channel_flow_message()))
            {
                LOG(warning, "events", "broadcast_status",
                    "Could not send message to receiver, PID "
                    + text::to_string(channel_recv_pid));
            }
        }

        for (PidRidArray::const_iterator proc_iter =
                send_processes.begin();
            proc_iter != send_processes.end();
            ++proc_iter)
        {
            if (not executor_ptr->send_message(
                proc_iter->first,
                proc_iter->second,
                make_channel_flow_message()))
            {
                LOG(warning, "events", "broadcast_status",
                    "Could not send message to listener, PID "
                    + text::to_string(proc_iter->first));
            }
        }

        channel_callback_in_progress = false;
    }

    // ----------------------------------------------------------------------
    ChannelFlowMessage* Channel::make_channel_flow_message(void)
    {
        ChannelFlowMessage * const message_ptr =
            new ChannelFlowMessage(channel_last_status);
        message_ptr->set_channel_name(channel_name);

        return message_ptr;
    }

    // ----------------------------------------------------------------------
    bool Channel::lock(void)
    {
        try
        {
            channel_mutex.lock();
            ++channel_external_locked_count;
            return true;
        }
        catch (...)
        {
            LOG(fatal, "events", "lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool Channel::try_lock(void)
    {
        try
        {
            if (channel_mutex.try_lock())
            {
                ++channel_external_locked_count;
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (...)
        {
            LOG(fatal, "events", "try_lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool Channel::try_lock_shared(void)
    {
        return try_lock();
    }

    // ----------------------------------------------------------------------
    bool Channel::lock_shared(void)
    {
        return lock();
    }

    // ----------------------------------------------------------------------
    bool Channel::unlock(void)
    {
        try
        {
            --channel_external_locked_count;
            channel_mutex.unlock();

            // In case delete was delayed because of lock
            delete_instance();
            return true;
        }
        catch (...)
        {
            LOG(fatal, "events", "unlock",
                "Trying to release exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool Channel::unlock_shared(void)
    {
        return unlock();
    }
}
}
