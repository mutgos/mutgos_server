/*
 * socket_CommandProcessor.cpp
 */

#include <string>
#include <sstream>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Id.h"

#include "text/text_StringParsing.h"
#include "text/text_StringConversion.h"
#include "text/text_ExternalPlainText.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"

#include "socketcomm/socket_CommandProcessor.h"
#include "socketcomm/socket_SocketDriver.h"
#include "socketcomm/socket_SocketClientConnection.h"

#include "comminterface/comm_RouterSessionManager.h"
#include "comminterface/comm_ClientSession.h"


namespace
{
    const static std::string COMMAND_SEPARATOR = " ";
    const static std::string CONNECT_COMMAND_1 = "connect";
    const static std::string CONNECT_COMMAND_2 = "conn";
    const static std::string CONNECT_COMMAND_3 = "co";

    const static char COMMAND_PREFIX = '!';
    const static std::string ANSI_ON_COMMAND = "color on";
    const static std::string ANSI_OFF_COMMAND = "color off";
    const static char SEND_TO_AGENT_COMMAND = '!';
    const static std::string HELP_COMMAND = "help";
}

namespace mutgos
{
namespace socket
{
    // ----------------------------------------------------------------------
    CommandProcessor::CommandProcessor(
        mutgos::socket::SocketClientConnection *connection_ptr)
          : auth_attempts(0),
            client_connection_ptr(connection_ptr)
    {
        if (not connection_ptr)
        {
            LOG(fatal, "socket", "CommandProcessor",
                "Connection pointer is null!  Crash will likely follow...");
        }
    }

    // ----------------------------------------------------------------------
    CommandProcessor::~CommandProcessor()
    {
        // Nothing to do
    }

    // ----------------------------------------------------------------------
    void CommandProcessor::process_input(text::ExternalTextLine *line_ptr)
    {
        if (line_ptr)
        {
            const bool logged_in = not client_connection_ptr->
                client_get_entity_id().is_entity_default();

            if (not logged_in)
            {
                const std::string stripped_string =
                    text::ExternalText::to_string(*line_ptr);

                // Handle login commands
                process_login_commands(stripped_string);

                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else
            {
                process_session_commands(line_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    void CommandProcessor::show_login_screen(void)
    {
        /**
         * TODO This will need a lot of work later.  It needs to show the site
         * name and description and be more data driven.  Will also need to
         * consider at some point how to see who is currently online at a site.
         */

        const dbinterface::DatabaseAccess::SiteInfoVector db_sites =
            dbinterface::DatabaseAccess::instance()->get_all_site_info();
        std::string output;

        output += "Welcome to the MUTGOS Alpha Demo.\n";
        output += "Pick a site below and connect to it like this: "
                  "connect <site#> myplayer mypassword\n"
                  "example: connect 2 oren specialword\n\n";
        output += "SITE #     NAME                   DESCRIPTION\n";
        output += "---------------------------------------------\n";

        for (dbinterface::DatabaseAccess::SiteInfoVector::const_iterator
                site_iter = db_sites.begin();
            site_iter != db_sites.end();
            ++site_iter)
        {
            std::ostringstream stream;

            stream << std::right << std::setw(4) << site_iter->get_site_id()
                   << "  "
                   << std::left << std::setw(22)
                   << site_iter->get_site_name().substr(0, 22)
                   << "  "
                   << site_iter->get_site_description().substr(0, 40)
                   << '\n';

            output += stream.str();
        }

        output += "\n\n";

        if (not client_connection_ptr->send_control_text_raw(output))
        {
            LOG(error, "socket", "show_login_screen",
                "Unable to send login screen to client.");
        }
    }

    // ----------------------------------------------------------------------
    void CommandProcessor::process_login_commands(const std::string &command)
    {
        text::StringParsing::SplitStrings split =
            text::StringParsing::split_string(
                command,
                COMMAND_SEPARATOR,
                true);

        if (not split.empty())
        {
            std::string &verb = split.front();
            text::to_lower(verb);

            if (((verb == CONNECT_COMMAND_3) or (verb == CONNECT_COMMAND_2) or
                (verb == CONNECT_COMMAND_1)) and (split.size() == 4))
            {
                // Connect command and has right number of arguments.  Parse
                // site ID and try to authenticate.
                //
                const dbtype::Id::SiteIdType site_id =
                    text::from_string<dbtype::Id::SiteIdType>(split[1]);

                if (not site_id)
                {
                    show_login_screen();
                }
                else
                {
                    client_connection_ptr->client_set_site_id(site_id);

                    // Don't allow them to keep trying over and over.
                    // Only attempt to authenticate if they haven't tried too
                    // many times.
                    //
                    comm::ClientSession * const session_ptr =
                        (auth_attempts > 6 ? 0 :
                        client_connection_ptr->get_driver()->get_router()->
                            reauthorize_client(
                                split[2],  // Name
                                split[3],  // Password
                                client_connection_ptr->get_driver(), // Driver
                                client_connection_ptr, // Connection
                                true));  // Make new if not found

                    client_connection_ptr->client_set_session(session_ptr);

                    if (not session_ptr)
                    {
                        // Bad username/password/site
                        //
                        std::string output;

                        output += "Incorrect site ID, username, or password "
                                  "specified.\n";

                        if (auth_attempts < 500)
                        {
                            ++auth_attempts;
                        }

                        if (not client_connection_ptr->send_control_text_raw(
                            output))
                        {
                            LOG(error, "socket", "process_login_commands",
                                "Unable to send message to client.");
                        }
                    }
                }
            }
            else
            {
                // Wrong command or mismatched arguments.  Remind them of what
                // to do.
                show_login_screen();
            }
        }
    }

    // ----------------------------------------------------------------------
    void CommandProcessor::process_session_commands(
        text::ExternalTextLine *line_ptr)
    {
        if (line_ptr)
        {
            if (line_ptr->empty())
            {
                client_connection_ptr->send_to_input_channel(line_ptr);
            }
            else if ((line_ptr->front()->get_text_type() ==
                    text::ExternalText::TEXT_TYPE_PLAIN_TEXT) or
                (line_ptr->front()->get_text_type() ==
                    text::ExternalText::TEXT_TYPE_FORMATTED_TEXT))
            {
                text::ExternalPlainText * const text_ptr =
                    dynamic_cast<text::ExternalPlainText *>(line_ptr->front());

                if (not text_ptr)
                {
                    LOG(error, "socket", "process_session_commands",
                        "Could not cast to ExternalPlainText!");
                    client_connection_ptr->send_to_input_channel(line_ptr);
                }
                else if ((not text_ptr->get_text().empty()) and
                    text_ptr->get_text()[0] == COMMAND_PREFIX)
                {
                    // Potential command.
                    handle_socket_client_commands(line_ptr);
                }
                else
                {
                    // Not a valid command.  Send as-is.
                    client_connection_ptr->send_to_input_channel(line_ptr);
                }
            }
            else
            {
                // Not a valid command.  Send as-is.
                client_connection_ptr->send_to_input_channel(line_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    void CommandProcessor::handle_socket_client_commands(
        text::ExternalTextLine *line_ptr)
    {
        // Assumes the right type has already been checked.
        //
        text::ExternalPlainText * const text_ptr =
            static_cast<text::ExternalPlainText *>(line_ptr->front());
        std::string &text = text_ptr->get_text_update();

        if ((text.size() >= 2) and (text[0] == COMMAND_PREFIX))
        {
            // Found a potential command
            //
            if (text[1] == SEND_TO_AGENT_COMMAND)
            {
                // Send to agent command.

                // Have to modify/remove parts of the line before we send
                // it to the channel, to get rid of the prefix.
                //
                if (text.size() == 2)
                {
                    // Remove entire text segment since it's just the prefix.
                    //
                    delete text_ptr;
                    line_ptr->erase(line_ptr->begin());
                }
                else
                {
                    text.erase(0, 2);
                }

                client_connection_ptr->send_to_input_channel(line_ptr, true);
            }
            else if ((text.size() == HELP_COMMAND.size() + 1) and
                (text.find(HELP_COMMAND, 1) == 1))
            {
                // Help command
                //
                const std::string help_str =
                    "\nTemporary help (case and space sensitive for now):\n"
                    "  !!<text>     Send text to the agent.  Useful if you're "
                    "currently in a program and want to use another command.\n"
                    "  !help        This text.\n"
                    "  !color on    Turns ANSI color on.\n"
                    "  !color off   Turns ANSI color off.\n\n";

                client_connection_ptr->send_control_text_raw(help_str);
                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else if ((text.size() == ANSI_ON_COMMAND.size() + 1) and
                (text.find(ANSI_ON_COMMAND, 1) == 1))
            {
                // ANSI on command
                //
                client_connection_ptr->set_ansi_enabled(true);
                client_connection_ptr->send_control_text_raw(
                    "ANSI color is now ON.\n");
                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else if ((text.size() == ANSI_OFF_COMMAND.size() + 1) and
                 (text.find(ANSI_OFF_COMMAND, 1) == 1))
            {
                // ANSI off command
                //
                client_connection_ptr->set_ansi_enabled(false);
                client_connection_ptr->send_control_text_raw(
                    "ANSI color is now OFF.\n");
                text::ExternalText::clear_text_line(*line_ptr);
                delete line_ptr;
            }
            else
            {
                // Not a valid command.  Send as-is.
                client_connection_ptr->send_to_input_channel(line_ptr);
            }
        }
        else
        {
            // Not a valid command.  Send as-is.
            client_connection_ptr->send_to_input_channel(line_ptr);
        }
    }
}
}
