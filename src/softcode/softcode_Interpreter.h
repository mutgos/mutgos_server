/*
 * softcode_Interpreter.h
 */

#ifndef MUTGOS_SOFTCODE_INTERPRETER_H
#define MUTGOS_SOFTCODE_INTERPRETER_H

#include <string>

#include "dbtypes/dbtype_Id.h"
#include "executor/executor_CommonTypes.h"

#include "security/security_Context.h"
#include "channels/events_TextChannel.h"

namespace mutgos
{
namespace softcode
{
    class Interpreter
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~Interpreter()
        { }

        /**
         * The language name should be short, for lookup efficiency.
         * @return The type of programming language the interpreter runs.
         * This is used to look up the interpreter when presented with a
         * program to execute.
         */
        virtual std::string get_language_name(void) =0;

        /**
         * Instructs the interpreter to create a new Process for the Program
         * specified in the security context.  The process will not be started,
         * but will be ready to start.  If the Program isn't currently
         * compiled and if compilation is supported, it will automatically be
         * compiled by the time it is started.
         * The method will also set the PID on the security context, when
         * known.
         * @param security_context_ptr[in] The security context, completely
         * filled in.  Control of the pointer will pass to this method.
         * @param command_name[in] The name of the command which caused
         * the program to execute, if known or applicable.
         * @param arguments[in] The arguments to the program provided as a
         * single string, if applicable.
         * @param outgoing_channel_ptr[in] The Channel used to write text out.
         * The pointer must not be deleted until released.
         * @param incoming_channel_ptr[in] The Channel used to read text in.
         * The pointer must not be deleted until released.  Optional.
         * @return The PID of the created process, or 0 if error.
         */
        virtual executor::PID make_process(
            security::Context *security_context_ptr,
            const std::string &command_name,
            const std::string &arguments,
            events::TextChannel *outgoing_channel_ptr,
            events::TextChannel *incoming_channel_ptr) =0;

        /**
         * Instructs the interpreter to compile a Program without running it.
         * If the Program is already compiled or compilation is not supported,
         * then nothing happens and it returns success.
         * @param program_id[in] The Program to compile.
         * @param outgoing_channel_ptr[in] The Channel used to write compiler
         * information out.  The pointer must not be deleted until this method
         * returns.
         * @return True if successfully compiled (or compilation not
         * supported), false if error.
         */
        virtual bool compile(
            const dbtype::Id &program_id,
            events::TextChannel *outgoing_channel_ptr) =0;

        /**
         * Instructs the interpreter to 'uncompile' the Program, meaning
         * any bytecode is deleted.  This allows for a recompile.
         * @param program_id[in] The Program to uncompile.
         * @return True if program found an uncompiled.
         */
        virtual bool uncompile(const dbtype::Id &program_id) =0;

    protected:
        /**
         * Abstract class constructor.
         */
        Interpreter(void)
        { }
    };
}
}

#endif //MUTGOS_SOFTCODE_INTERPRETER_H
