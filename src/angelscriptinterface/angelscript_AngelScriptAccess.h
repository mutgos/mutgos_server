/*
 * angelscript_AngelScriptAccess.h
 */

#ifndef MUTGOS_ANGELSCRIPT_ANGELSCRIPTACCESS_H
#define MUTGOS_ANGELSCRIPT_ANGELSCRIPTACCESS_H

#include <string>
#include <map>
#include <angelscript.h>
#include <boost/thread/mutex.hpp>

#include "dbinterface/dbinterface_EntityRef.h"

#include "angelscript_StringFactory.h"

#include "softcode/softcode_Interpreter.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class TextChannel;
}

namespace angelscript
{
    /**
     * Provides methods to start and manage AngelScript processes, scripts,
     * and the AngelScript engines and contexts.
     * Also implements the Interpreter interface which allows the Softcode
     * module to launch AngelScript processes.
     */
    class AngelScriptAccess : public softcode::Interpreter
    {
    public:
        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static AngelScriptAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static AngelScriptAccess *instance(void)
        { return singleton_ptr;  }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up.
         * @return True if successfully started.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Note that all AngelScript processes must be shut down before
         * calling this.  It will attempt to clean them up, but that may
         * result in a crash if the processes are reactivated in the
         * Executor module.
         * @return True if all engines were shutdown, false if some remain
         * running.  If false, destroying the singleton may result in a crash.
         */
        bool shutdown(void);

        /**
         * @return The type of programming language the interpreter runs.
         * This is used to look up the interpreter when presented with a
         * program to execute.
         */
        virtual std::string get_language_name(void);

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
            events::TextChannel *incoming_channel_ptr);

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
            events::TextChannel *outgoing_channel_ptr);

        /**
         * Instructs the interpreter to 'uncompile' the Program, meaning
         * any bytecode is deleted.  This allows for a recompile.
         * @param program_id[in] The Program to uncompile.
         * @return True if program found an uncompiled.
         */
        virtual bool uncompile(const dbtype::Id &program_id);

        /**
         * Returns the given engine and context to the pool for reuse or
         * destruction.
         * @param engine[in] The engine being returned.
         * @param context[in] The associated context being returned.
         */
        void release_engine_context(
            asIScriptEngine * const engine,
            asIScriptContext * const context);

        /**
         * Removes the compiled (binary) script data.
         * @param program_ref[in] The program Entity to remove the compiled
         * data from.
         * @return True if Entity passed in is a program, is an AngelScript,
         * and had its compile data successfully cleared.
         */
        bool uncompile_script(dbinterface::EntityRef &program_ref);

        /**
         * Compiles the given script if not already compiled.  If the script
         * is already compiled, the existing copy of the bytecode will be
         * returned if specified.
         * @param program_ref[in] The program Entity to compile.
         * @param engine[in] The configured engine that will be doing the
         * compile.  A module will be temporarily created for the purpose.
         * @param output_channel_ptr[in,out] The (optional) channel to send
         * compile errors to.  If there are no errors, nothing will be sent.
         * This channel will be left open when done.
         * @param want_bytecode[in] True to copy the compiled or existing
         * bytecode into bytecode_ptr and bytecode_size.  If false, those two
         * parameters will be set to 0.
         * @param bytecode_ptr[out] If want_bytecode is true and there is
         * valid bytecode, a copy of the bytecode will be placed here.  Caller
         * must manage the pointer!
         * @param bytecode_size[out] The size of the data pointed to by
         * bytecode_ptr.
         * @return True if successfully compiled (or if already compiled),
         * false if not.
         */
        bool compile_script(
            dbinterface::EntityRef &program_ref,
            asIScriptEngine &engine,
            events::TextChannel *output_channel_ptr,
            const bool want_bytecode,
            char *&bytecode_ptr,
            size_t &bytecode_size);

    private:

        /**
         * Simple container class to hold an engine and its associated context.
         */
        class EngineContextState
        {
        public:
            /**
             * Default constructor to initialize everything.
             */
            EngineContextState(void)
              : engine_ptr(0),
                context_ptr(0),
                string_factory_ptr(0)
            {
            }

            /**
             * Constructor that sets all attributes.
             * @param engine[in] Engine pointer.
             * @param context[in] Context pointer.
             * @param string_factory[in] Pointer to engine's string factory.
             */
            EngineContextState(
                asIScriptEngine * const engine,
                asIScriptContext * const context,
                StringFactory * const string_factory)
                : engine_ptr(engine),
                  context_ptr(context),
                  string_factory_ptr(string_factory)
            {
            }

            asIScriptEngine *engine_ptr; ///< The AngelScript engine.
            asIScriptContext *context_ptr; ///< The AngelScript context associated with the engine.
            StringFactory *string_factory_ptr; ///< The associated string factory for the engine
        };

        /**
         * Handles sending a compiler message over a TextChannel.
         */
        class MessageCallbackWrapper
        {
        public:
            /**
             * Constructor that provides the program and an output channel.
             * @param id[in] The program being compiled.
             * @param channel_ptr[in] The channel to send messaage callback
             * data on.  The channel will remain open when this instance is
             * destructed.
             */
            MessageCallbackWrapper(
                const dbtype::Id &id,
                events::TextChannel * const channel_ptr);

            /**
             * Destructor.
             */
            ~MessageCallbackWrapper();

            /**
             * Called by AngelScript when it has a message about compilation.
             * @param msg[in] The message.
             * @param param[in] User provided pointer; currently ignored.
             */
            void message(const asSMessageInfo *msg, void *param);

        private:
            dbtype::Id prog_id; ///< ID of program being compiled.
            events::TextChannel * const output_channel_ptr; ///< Channel to send callback message
        };


        typedef std::vector<EngineContextState> Engines;

        /**
         * Private singleton constructor.
         */
        AngelScriptAccess(void);

        /**
         * Private singleton destructor.
         */
        virtual ~AngelScriptAccess();


        /**
         * Finds the next available (unused) engine, marks it in use, and
         * returns the engine info to the caller.
         * @param engine_ptr[out] The pointer to the newly reserved engine.
         * Do not delete this pointer!
         * @param context_ptr[out] The pointer to the newly reserved engine
         * context.  Do not delete this pointer!
         * @return True if successfully returned an available engine, or false
         * if no engines available and creation is impossible.
         */
        bool get_available_engine(
            asIScriptEngine *&engine_ptr,
            asIScriptContext *&context_ptr);

        /**
         * Prepares the given engine and context for reuse and puts it back in
         * the pool of available engines or deletes it if too many are
         * available.
         * @param engine_ptr[in] The engine being returned.  After this call
         * returns, the pointer cannot be used.
         * @param context_ptr[in] The associated context being returned.
         * After this call returns, the pointer cannot be used.
         */
        void return_used_engine(
            asIScriptEngine * const engine_ptr,
            asIScriptContext * const context_ptr);

        /**
         * Checks the return code from compiling, etc. with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the compilation call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        void check_compile_rc(
            const int rc,
            const size_t line,
            bool &current_result);

        // No copying
        //
        AngelScriptAccess &operator=(const AngelScriptAccess &rhs);
        AngelScriptAccess(const AngelScriptAccess &rhs);

        static AngelScriptAccess *singleton_ptr; ///< Singleton pointer

        boost::mutex mutex; ///< Enforces single access at a time.
        bool started; ///< True if startup() successfully called.
        Engines engines_avail; ///< AngelScript engines that are available for use
        Engines engines_used; ///< AngelScript engines that are currently in use
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_ANGELSCRIPTACCESS_H
