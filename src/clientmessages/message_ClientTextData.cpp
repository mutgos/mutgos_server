/*
 * message_ClientTextData.cpp
 */

#include "logging/log_Logger.h"

#include "text/text_ExternalText.h"
#include "message_ClientMessage.h"

#include "clientmessages/message_MessageFactory.h"
#include "utilities/json_JsonUtilities.h"

#include "message_ClientTextData.h"

namespace
{
    // Static registration
    const bool CHANNEL_DATA_FACTORY_REG =
        mutgos::message::MessageFactory::register_message(
            mutgos::message::CLIENTMESSAGE_TEXT_DATA,
            mutgos::message::ClientTextData::make_instance);

    const static std::string TEXT_LINE_KEY = "textData";
}

namespace mutgos
{
namespace message
{
    // ----------------------------------------------------------------------
    ClientTextData::ClientTextData(void)
      : ClientMessage(CLIENTMESSAGE_TEXT_DATA),
        text_line_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    ClientTextData::ClientTextData(text::ExternalTextLine &line)
      : ClientMessage(CLIENTMESSAGE_TEXT_DATA)
    {
        text_line_ptr = new text::ExternalTextLine();
        text_line_ptr->swap(line);
    }

    // ----------------------------------------------------------------------
    ClientTextData::ClientTextData(const ClientTextData &rhs)
      : ClientMessage(rhs),
        text_line_ptr(0)
    {
        if (rhs.text_line_ptr)
        {
            text_line_ptr = new text::ExternalTextLine(
                text::ExternalText::clone_text_line(*rhs.text_line_ptr));
        }
    }

    // ----------------------------------------------------------------------
    ClientTextData::~ClientTextData()
    {
        if (text_line_ptr)
        {
            text::ExternalText::clear_text_line(*text_line_ptr);
            delete text_line_ptr;
            text_line_ptr = 0;
        }
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientTextData::make_instance(void)
    {
        return new ClientTextData();
    }

    // ----------------------------------------------------------------------
    ClientMessage* ClientTextData::clone(void) const
    {
        return new ClientTextData(*this);
    }

    // ----------------------------------------------------------------------
    void ClientTextData::set_text_line(text::ExternalTextLine &line)
    {
        if (text_line_ptr)
        {
            text::ExternalText::clear_text_line(*text_line_ptr);
        }
        else
        {
            text_line_ptr = new text::ExternalTextLine();
        }

        text_line_ptr->swap(line);
    }

    // ----------------------------------------------------------------------
    text::ExternalTextLine *ClientTextData::transfer_text_line(void)
    {
        text::ExternalTextLine * const temp = text_line_ptr;
        text_line_ptr = 0;
        return temp;
    }

    // ----------------------------------------------------------------------
    bool ClientTextData::save(json::JSONRoot &root, json::JSONNode &node) const
    {
        bool success = ClientMessage::save(root, node);

        if (text_line_ptr)
        {
            JSON_MAKE_MAP_NODE(text_data);

            success = text::ExternalText::save_line(
                *text_line_ptr,
                root,
                text_data) and success;

            if (success)
            {
                success = json::add_static_key_value(
                    TEXT_LINE_KEY,
                    text_data,
                    node,
                    root) and success;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ClientTextData::restore(const json::JSONNode &node)
    {
        bool success = ClientMessage::restore(node);

        if (text_line_ptr)
        {
            text::ExternalText::clear_text_line(*text_line_ptr);
        }
        else
        {
            text_line_ptr = new text::ExternalTextLine();
        }

        const json::JSONNode *text_node = 0;
        json::get_key_value(TEXT_LINE_KEY, node, text_node);

        if (not text_node)
        {
            LOG(error, "message", "restore", "No text data found!");
            success = false;
        }
        else
        {
            success = text::ExternalText::restore_line(
                *text_node,
                *text_line_ptr) and success;
        }

        return success;
    }
}
}
