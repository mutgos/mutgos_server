/*
 * useragent_TextChannelDocumentWriter.cpp
 */

#include <string>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_PropertyEntity.h"

#include "dbinterface/dbinterface_EntityRef.h"

#include "text/text_ExternalText.h"

#include "channels/events_TextChannelReceiver.h"
#include "channels/events_ChannelControlListener.h"

#include "channels/events_ChannelTextMessage.h"
#include "channels/events_TextChannel.h"

#include "useragent_TextChannelDocumentWriter.h"

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    TextChannelDocumentWriter::TextChannelDocumentWriter(
        dbinterface::EntityRef &entity,
        const std::string &property,
        events::TextChannel *const text_channel_ptr)
          : registered_listener(false),
            channel_ptr(text_channel_ptr),
            entity_ref(entity),
            property_path(property)
    {
        if (not text_channel_ptr)
        {
            LOG(error, "useragent", "TextChannelDocumentWriter",
                "text_channel_ptr is null!");
        }
        else
        {
            if (not entity_ref.valid())
            {
                LOG(error, "useragent", "TextChannelDocumentWriter",
                    "Entity is not valid!");
            }

            // Register for text and control data.
            //
            if (not text_channel_ptr->register_receiver_callback(this))
            {
                LOG(error, "useragent", "TextChannelDocumentWriter",
                    "Unable to register as receiver!");
            }
            else
            {
                registered_listener = true;
            }

            if (not text_channel_ptr->channel_register_control_listener(this))
            {
                LOG(error, "useragent", "TextChannelDocumentWriter",
                    "Unable to register as channel control listener");
            }
            else
            {
                registered_listener = true;
            }
        }
    }

    // ----------------------------------------------------------------------
    TextChannelDocumentWriter::~TextChannelDocumentWriter()
    {
        unregister();
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::text_channel_data(
        const std::string &channel_name,
        events::TextChannel *channel_ptr,
        text::ExternalTextLine &text_line)
    {
        // The document may be full when we try and add the data; if so
        // just ignore it so the program completes.
        // TODO Might need to use System Prims for to_string() at some point
        document.append_line(text::ExternalText::to_string(text_line));
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::channel_flow_blocked(
        const std::string &channel_name,
        events::Channel * const channel_ptr)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::channel_flow_open(
        const std::string &channel_name,
        events::Channel * const channel_ptr)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::channel_flow_closed(
        const std::string &channel_name,
        events::Channel * const channel_ptr)
    {
        // No more output will be received as the Channel is now closed; save
        // to Entity and destruct ourselves.
        //
        if (entity_ref.valid())
        {
            dbtype::PropertyEntity * const property_entity =
                dynamic_cast<dbtype::PropertyEntity *>(entity_ref.get());

            if (not property_entity)
            {
                LOG(error, "useragent", "channel_flow_closed",
                    "Not a PropertyEntity: " + entity_ref.id().to_string(true));
            }
            else
            {
                if (not property_entity->set_property(property_path, document))
                {
                    LOG(error, "useragent", "channel_flow_closed",
                        "Unable to set document on path " + property_path
                        + " on " + entity_ref.id().to_string(true)
                        + ".  Has the application been created?");
                }
            }
        }

        delete this;
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::channel_destructed(
        const std::string &channel_name,
        events::Channel * const channel_ptr)
    {
        // Nothing to do; this is obsolete and can never be called.
    }

    // ----------------------------------------------------------------------
    void TextChannelDocumentWriter::unregister(void)
    {
        if (registered_listener)
        {
            if (channel_ptr)
            {
                channel_ptr->unregister_receiver_callback(this);
                channel_ptr->channel_unregister_control_listener(this);

                // channel_ptr should be considered invalid at this point.
            }

            registered_listener = false;
        }
    }
}
}
