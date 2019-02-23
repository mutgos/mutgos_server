/*
 * comm_RouterEvent.h
 */

#ifndef MUTGOS_COMM_ROUTEREVENT_H
#define MUTGOS_COMM_ROUTEREVENT_H

#include "comminterface/comm_CommonTypes.h"

#include "text/text_ExternalText.h"

#include "clientmessages/message_ChannelStatusChange.h"
#include "clientmessages/message_ClientMessage.h"

namespace mutgos
{
namespace comm
{
    /**
     * A container class that stores an event or action the Router needs
     * to process, and any related metadata about it.
     */
    class RouterEvent
    {
    public:
        /**
         * The types of events the Router can queue up.
         */
        enum EventType
        {
            /** Text data (ExternalText) */
            EVENT_TEXT_DATA,
            /** Enhanced (structured) data.  ClientMessage. */
            EVENT_ENHANCED_DATA,
            /** Channel status changes */
            EVENT_CHANNEL_STATUS_DATA,
            /** Invalid event type.  Used when RouterEvent contains nothing */
            EVENT_INVALID_END
        };

        /**
         * Constructs an invalid RouterEvent, generally used for transfer or
         * marking.
         */
        RouterEvent(void)
          : event_type(EVENT_INVALID_END),
            event_serial_id(0),
            event_channel_id(0)
          { event_data.raw_ptr = 0; }

        /**
         * Constructs a RouterEvent for an event type that has no underlying
         * event data.
         * @param type[in] The type of the event.
         * @param serial_id[in] The serial ID number for the event (optional).
         * @param channel_id[in] The channel ID associated with the event
         * (optional).
         */
        RouterEvent(
            const EventType type,
            const MessageSerialId serial_id = 0,
            const ChannelId channel_id = 0)
          : event_type(type),
            event_serial_id(serial_id),
            event_channel_id(channel_id)
          { event_data.raw_ptr = 0; }


        /**
         * Moves the event contained by the provided RouterEvent into this
         * newly constructed one.  When done, the source will not have the
         * event.
         * @param rhs[in] The source to move the event from.
         */
        RouterEvent(RouterEvent &rhs)
          : event_type(rhs.event_type),
            event_serial_id(rhs.event_serial_id),
            event_channel_id(rhs.event_channel_id),
            event_data(rhs.event_data)
        {
            // Since this is a move, clear out the source
            //
            rhs.event_type = EVENT_INVALID_END;
            rhs.event_serial_id = 0;
            rhs.event_channel_id = 0;
            rhs.event_data.raw_ptr = 0;
        }

        /**
         * Constructs a RouterEvent for a text line.
         * @param text_line_ptr[in] The text line event.  Ownership of the
         * pointer is transferred to this class.
         * @param serial_id[in] The serial ID number for the event.
         * @param channel_id[in] The channel ID associated with the event.
         */
        RouterEvent(
            text::ExternalTextLine *text_line_ptr,
            const MessageSerialId serial_id,
            const ChannelId channel_id)
          : event_type(EVENT_TEXT_DATA),
            event_serial_id(serial_id),
            event_channel_id(channel_id)
          { event_data.text_line_ptr = text_line_ptr; }

        /**
         * Constructs a RouterEvent for enhanced data / client message.
         * @param client_message_ptr[in] The enhanced data message.  Ownership
         * of the pointer is transferred to this class.
         * @param serial_id[in] The serial ID number for the event.
         * @param channel_id[in] The channel ID associated with the event.
         */
        RouterEvent(
            message::ClientMessage *client_message_ptr,
            const MessageSerialId serial_id,
            const ChannelId channel_id)
            : event_type(EVENT_ENHANCED_DATA),
              event_serial_id(serial_id),
              event_channel_id(channel_id)
          { event_data.client_message_ptr = client_message_ptr; }

        /**
         * Constructs a RouterEvent for a channel status.
         * @param channel_status_ptr[in] The channel status.  Ownership of the
         * pointer is transferred to this class.
         * @param serial_id[in] The serial ID number for the event.
         */
        RouterEvent(
            message::ChannelStatusChange *channel_status_ptr,
            MessageSerialId serial_id)
            : event_type(EVENT_CHANNEL_STATUS_DATA),
              event_serial_id(serial_id),
              event_channel_id(0)
          { event_data.channel_status_ptr = channel_status_ptr; }

        /**
         * Copy constructor.  Makes a copy of the event contained within.
         * @param rhs[in] The source of the copy.
         */
        RouterEvent(const RouterEvent &rhs)
            : event_type(EVENT_INVALID_END),
              event_serial_id(0),
              event_channel_id(0)
          {
              event_data.raw_ptr = 0;
              clone_event_data(rhs);
          }

        /**
         * Destructs the class and frees any memory associated with the event.
         */
        ~RouterEvent()
          { cleanup(); }

        /**
         * Assignment.
         * @param rhs[in] The source to copy from.
         * @return This.
         */
        RouterEvent &operator=(const RouterEvent &rhs)
        {
            clone_event_data(rhs);
            return *this;
        }

        /**
         * Used after getting the event data, this releases ownership of
         * the event data from this class and transfers it to the
         * caller.  It does NOT delete the pointer.
         *
         * Caller must have already retrieved the event data before calling
         * this method.
         */
        void transfer(void)
        {
            event_type = EVENT_INVALID_END;
            event_data.raw_ptr = 0;
        }

        /**
         * Transfers the event data from the source into this instance.
         * When done, the source will not have the event.
         * If this instance already has an event, it will first be cleaned up.
         * @param source[in] The source to move the event from.
         */
        void transfer(RouterEvent &source)
        {
            cleanup();

            event_type = source.event_type;
            event_serial_id = source.event_serial_id;
            event_channel_id = source.event_channel_id;
            event_data.raw_ptr = source.event_data.raw_ptr;

            // Since this is a move, clear out the source
            //
            source.event_type = EVENT_INVALID_END;
            source.event_serial_id = 0;
            source.event_channel_id = 0;
            source.event_data.raw_ptr = 0;
        }

        /**
         * @return The event type.
         */
        const EventType get_event_type(void) const
          { return event_type; }

        /**
         * Pointer ownership does NOT transfer to the caller.
         * @return Pointer to text data, or null if not the
         * correct type or not set.
         */
        text::ExternalTextLine *get_text_data(void) const
        {
            text::ExternalTextLine *value = 0;

            if (event_type == EVENT_TEXT_DATA)
            {
                value = event_data.text_line_ptr;
            }

            return value;
        }

        /**
         * Pointer ownership does NOT transfer to the caller.
         * @return Pointer to enhanced data (client message), or null if not
         * the correct type or not set.
         */
        message::ClientMessage *get_enhanced_data(void) const
        {
            message::ClientMessage *value = 0;

            if (event_type == EVENT_ENHANCED_DATA)
            {
                value = event_data.client_message_ptr;
            }

            return value;
        }

        /**
         * Pointer ownership does NOT transfer to the caller.
         * @return Pointer to channel status data, or null if not
         * the correct type or not set.
         */
        message::ChannelStatusChange *get_channel_status_data(void) const
        {
            message::ChannelStatusChange *value = 0;

            if (event_type == EVENT_CHANNEL_STATUS_DATA)
            {
                value = event_data.channel_status_ptr;
            }

            return value;
        }

        /**
         * @return The serial ID number associated with the event.
         */
        MessageSerialId get_serial_id(void) const
          { return event_serial_id; }

        /**
         * @return The channel ID associated with the event.
         */
        ChannelId get_channel_id(void) const
          { return event_channel_id; }

    private:

        /**
         * Frees the event data on this instance and sets the RouterEvent
         * to an invalid/empty state.
         */
        void cleanup(void)
        {
            if (event_data.raw_ptr)
            {
                switch (event_type)
                {
                    case EVENT_TEXT_DATA:
                    {
                        text::ExternalText::clear_text_line(
                            *(event_data.text_line_ptr));
                        delete event_data.text_line_ptr;
                        break;
                    }

                    case EVENT_ENHANCED_DATA:
                    {
                        delete event_data.client_message_ptr;
                        break;
                    }

                    case EVENT_CHANNEL_STATUS_DATA:
                    {
                        delete event_data.channel_status_ptr;
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }

                event_data.raw_ptr = 0;
            }

            event_type = EVENT_INVALID_END;
            event_serial_id = 0;
            event_channel_id = 0;
        }

        /**
         * Clones (deep copies) the provided RouterEvent.  Used as a helper
         * for assignment and copy constructors.
         * @param rhs[in] The source to copy from.
         */
        void clone_event_data(const RouterEvent &rhs)
        {
            cleanup();

            event_type = rhs.event_type;
            event_serial_id = rhs.event_serial_id;
            event_channel_id = rhs.event_channel_id;

            event_data.raw_ptr = 0;

            if (rhs.event_data.raw_ptr)
            {
                // Clone based on type.
                switch (rhs.event_type)
                {
                    case EVENT_TEXT_DATA:
                    {
                        event_data.text_line_ptr =
                            new text::ExternalTextLine(
                                text::ExternalText::clone_text_line(
                                    *(rhs.event_data.text_line_ptr)));
                        break;
                    }

                    case EVENT_ENHANCED_DATA:
                    {
                        event_data.client_message_ptr =
                            rhs.event_data.client_message_ptr->clone();
                        break;
                    }

                    case EVENT_CHANNEL_STATUS_DATA:
                    {
                        event_data.channel_status_ptr =
                            new message::ChannelStatusChange(
                                *(rhs.event_data.channel_status_ptr));
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
            }
        }

        /**
         * Simply a union of all the possible event types, for easy casting.
         */
        union EventClasses
        {
            void *raw_ptr;
            text::ExternalTextLine *text_line_ptr;
            message::ClientMessage *client_message_ptr;
            message::ChannelStatusChange *channel_status_ptr;
        };

        EventType event_type; ///< The router event type
        MessageSerialId event_serial_id; ///< The serial number associated with the event
        ChannelId event_channel_id; ///< The channel ID associated with the event, if any
        EventClasses event_data; ///< The router event itself
    };
}
}
#endif //MUTGOS_COMM_ROUTEREVENT_H
