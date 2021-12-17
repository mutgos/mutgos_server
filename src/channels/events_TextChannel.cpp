/*
 * events_TextChannel.cpp
 */

#include <string>

#include "logging/log_Logger.h"

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "text/text_ExternalText.h"

#include "channels/events_Channel.h"
#include "channels/events_ChannelTextMessage.h"
#include "events_TextChannel.h"
#include "events_TextChannelReceiver.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    TextChannel::TextChannel(
        const std::string &name,
        const std::string &subtype,
        const dbtype::Id &entity_id)
      : Channel(name, Channel::CHANNEL_TYPE_TEXT, subtype, entity_id),
        recv_callback_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    TextChannel::~TextChannel()
    {
    }

    // ----------------------------------------------------------------------
    bool TextChannel::send_item(text::ExternalTextLine &item)
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
                        new ChannelTextMessage(item)))
                    {
                        LOG(error, "events", "send_item",
                            "Unable to send to receiver on channel name "
                            + channel_name);
                    }

                    item.clear();
                }
                else if (recv_callback_ptr)
                {
                    recv_callback_ptr->text_channel_data(
                        channel_name,
                        this,
                        item);
                }
                else
                {
                    // This is a 'null' receiver, meaning the item is accepted
                    // but never received by anyone.
                    // Right now no additional actions are needed.
                }

                // If receiver didn't want to keep anything, free it.
                text::ExternalText::clear_text_line(item);
                success = true;
            }

            channel_callback_in_progress = false;
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool TextChannel::register_receiver_callback(
        TextChannelReceiver *callback_ptr)
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
    void TextChannel::unregister_receiver_callback(
        TextChannelReceiver *callback_ptr)
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
    bool TextChannel::receiver_callback_registered(void)
    {
        boost::lock_guard<boost::recursive_mutex> guard(channel_mutex);

        return recv_callback_ptr;
    }
}
}
