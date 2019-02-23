/*
 * events_ClientDataChannel.cpp
 */

#include <string>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "logging/log_Logger.h"

#include "channels/events_ClientDataChannel.h"
#include "channels/events_ClientDataReceiver.h"
#include "channels/events_ChannelClientDataMessage.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    ClientDataChannel::ClientDataChannel(
        const std::string &name,
        const std::string &subtype)
      : Channel(name, Channel::CHANNEL_TYPE_CLIENT_DATA, subtype),
        recv_callback_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    ClientDataChannel::~ClientDataChannel()
    {
    }

    // ----------------------------------------------------------------------
    bool ClientDataChannel::send_item(message::ClientMessage *item_ptr)
    {
        bool success = false;

        // Scope for mutex
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            channel_callback_in_progress = true;

            if (channel_about_to_send_item())
            {
                // We can send item.  Figure out how to reach receiver.
                if (channel_receiver_is_process())
                {
                    if (not channel_send_to_receiver(
                        new ChannelClientDataMessage(item_ptr)))
                    {
                        LOG(error, "events", "send_item",
                            "Unable to send to receiver on channel name "
                            + channel_name);
                    }
                }
                else if (recv_callback_ptr)
                {
                    recv_callback_ptr->client_channel_data(
                        channel_name,
                        this,
                        item_ptr);
                }
                else
                {
                    // This is a 'null' receiver, meaning the item is accepted
                    // but never received by anyone.
                    // Right now no additional actions are needed.
                }

                success = true;
            }

            channel_callback_in_progress = false;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientDataChannel::register_receiver_callback(
        ClientDataReceiver *callback_ptr)
    {
        bool success = false;

        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        if (callback_ptr)
        {
            if (not channel_receiver_is_process())
            {
                if ((not recv_callback_ptr) or
                    (recv_callback_ptr == callback_ptr))
                {
                    recv_callback_ptr = callback_ptr;
                    channel_register_pointer_holder(callback_ptr);
                    success = true;
                }
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void ClientDataChannel::unregister_receiver_callback(
        ClientDataReceiver *callback_ptr)
    {
        bool removed_listener = false;

        // Scope for guard, since we can't be in it while destructing.
        {
            boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

            if (callback_ptr and (callback_ptr == recv_callback_ptr))
            {
                recv_callback_ptr = 0;
                removed_listener = true;
                internal_close_channel();
            }
        }

        if (removed_listener)
        {
            channel_unregister_pointer_holder(callback_ptr);
        }
    }

    // ----------------------------------------------------------------------
    bool ClientDataChannel::receiver_callback_registered(void)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        return recv_callback_ptr;
    }
}
}
