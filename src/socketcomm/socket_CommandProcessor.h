/*
 * socket_CommandProcessor.h
 */

#ifndef MUTGOS_SOCKET_COMMANDPROCESSOR_H
#define MUTGOS_SOCKET_COMMANDPROCESSOR_H

#include "osinterface/osinterface_OsTypes.h"

#include "text/text_ExternalText.h"
#include "text/text_StringParsing.h"

namespace mutgos
{
namespace socket
{
    // Forward declarations.
    //
    class SocketClientConnection;

    /**
     * Checks all input from a socket to see if it's a command that we need
     * to process instead of the user agent.  If so, do the processing,
     * otherwise pass it on to the active input Channel.
     *
     * This also handles login, splash screen, etc.
     *
     * In the future, this could be enhanced to have some sort of preferences
     * the user could set, which is why it's a class rather than static
     * methods.
     *
     * This is not thread safe.
     */
    class CommandProcessor
    {
    public:
        /**
         * Constructs the command processor.
         * @param connection_ptr[in] Pointer to the socket client connection
         * this processor is for.  Cannot be null.
         */
        CommandProcessor(SocketClientConnection *connection_ptr);

        /**
         * Destructor.
         */
        ~CommandProcessor();

        /**
         * Checks the input line to see if it has a command we should process,
         * and handles it if so.  If it is not a command for us, send it back
         * to the client connection and through the appropriate channel.
         * @param line_ptr[in] The line to process.  Ownership of the pointer
         * will transfer to this method.
         */
        void process_input(text::ExternalTextLine *line_ptr);

        /**
         * Shows the main splash screen seen before logging in.
         */
        void show_login_screen(void);

    private:

        /**
         * Handles processing of any commands entered at the login/splash
         * screen.
         * @param command[in] The command to parse and execute.
         */
        void process_login_commands(const std::string &command);

        /**
         * Processes any socket module commands entered after logged in, while
         * in the game (session).  If the command doesn't correspond to one we
         * know, pass it along to the correct Channel.
         * @param line_ptr[in] The line to parse and execute.  Method takes
         * ownership of the pointer.
         */
        void process_session_commands(text::ExternalTextLine *line_ptr);

        /**
         * If the incoming line is determined to possibly be a command
         * for the socket module, this will process it or pass the line
         * on to the appropriate Channel if not.
         * @param line_ptr[in] The line to process.  It is assumed not to
         * be null and that the first segment (element) is a external
         * plain text or derivative.  The method takes ownership of the
         * pointer.
         */
        void handle_socket_client_commands(text::ExternalTextLine *line_ptr);

        MG_UnsignedInt auth_attempts; ///< Number of bad attempts to authenticate
        SocketClientConnection * const client_connection_ptr; ///< Pointer to client connection we are processing
    };
}
}

#endif //MUTGOS_SOCKET_COMMANDPROCESSOR_H
