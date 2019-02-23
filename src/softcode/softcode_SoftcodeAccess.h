/*
 * softcode_SoftcodeAccess.h
 */

#ifndef MUTGOS_SOFTCODE_SOFTCODEACCESS_H
#define MUTGOS_SOFTCODE_SOFTCODEACCESS_H

#include <string>
#include <vector>
#include <map>

#include "dbtypes/dbtype_Id.h"
#include "executor/executor_CommonTypes.h"

namespace mutgos
{
namespace security
{
    // Forward declarations
    class Context;
}
namespace events
{
    // Forward declarations
    class TextChannel;
}
namespace softcode
{
    // Forward declarations
    class Interpreter;

    // TODO Should this class auto-start the languages?

    // TODO If a Program is deleted or modified while being ran, that could cause problems.

    /**
     * The interface that other modules can use to execute, compile, etc
     * softcode.  Softcode is defined as any program that is stored within the
     * database and executed within a site.
     *
     * The main initialization sequence is reponsible for determining what
     * softcode languages should be registered with this class.
     *
     * While registering/unregistering is not thread safe, once everything
     * is registered it is safe to call make_process(), compile(), uncompile()
     * on multiple threads.
     */
    class SoftcodeAccess
    {
    public:
        typedef std::vector<std::string> RegisteredLanguages;

        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static SoftcodeAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * @return The singleton instance, or null if not created.
         */
        static SoftcodeAccess *instance(void)
        { return singleton_ptr;  }

        /**
         * Destroys the singleton instance if it exists.
         */
        static void destroy_singleton(void);

        /**
         * Registers a language interpreter.
         * This is not thread safe.
         * @param interpreter[in] The interpreter to register.  The pointer
         * must not be deleted until after it is unregistered.
         * @return True if registere, false if error or language is already
         * registered.
         */
        bool register_language(Interpreter * const interpreter);

        /**
         * Unregisters a language interpreter.
         * This is not thread safe.
         * @param interpreter[in] The interpreter to unregister.
         */
        void unregister_language(Interpreter * const interpreter);

        /**
         * Thread safe.
         * @param language[in] The language to check for.
         * @return True if an interpreter for the language is registered.
         */
        bool is_language_registered(const std::string &language) const;

        /**
         * @return All the registered languages.
         */
        RegisteredLanguages get_registered_languages(void) const;

        /**
         * Create a new Process for the Program specified in the security
         * context.  The process will not be started, but will be ready to
         * start.  If the Program isn't currently compiled and if compilation
         * is supported, it will automatically be compiled by the time it is
         * started.
         * The method will also set the PID on the security context, when
         * known.
         * Thread safe.
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
         * @return The PID of the created process, or 0 if error or the
         * programming language isn't known.
         */
        executor::PID make_process(
            security::Context *security_context_ptr,
            const std::string &command_name,
            const std::string &arguments,
            events::TextChannel *outgoing_channel_ptr,
            events::TextChannel *incoming_channel_ptr);

        /**
         * Compile a Program without running it.  If the Program is already
         * compiled or compilation is not supported, then nothing happens and
         * it returns success.
         * Thread safe.
         * @param program_id[in] The Program to compile.
         * @param outgoing_channel_ptr[in] The Channel used to write compiler
         * information out.  The pointer must not be deleted until this method
         * returns.
         * @return True if successfully compiled (or compilation not
         * supported), false if error or the language isn't known.
         */
        bool compile(
            const dbtype::Id &program_id,
            events::TextChannel *outgoing_channel_ptr);

        /**
         * 'Uncompile' the Program, meaning any bytecode is deleted.  This
         * allows for a recompile.
         * Thread safe.
         * @param program_id[in] The Program to uncompile.
         * @return True if program found an uncompiled, false if not found or
         * the language isn't known.
         */
        bool uncompile(const dbtype::Id &program_id);

    private:
        typedef std::pair<std::string, Interpreter *> LanguageInterpreter;
        typedef std::vector<LanguageInterpreter> LanguageInterpreters;


        /**
         * Private singleton constructor.
         */
        SoftcodeAccess(void);

        /**
         * Private singleton destructor.
         */
        ~SoftcodeAccess();

        /**
         * Finds an interpreter by name.
         * @param language[in] The interpreter to find, by language.
         * @param iter[out] An iterator which represents the result of the
         * search.
         * @return True if found.
         */
        bool find_interpreter(
            const std::string &language,
            LanguageInterpreters::iterator &iter);

        /**
         * Finds an interpreter by name.
         * @param language[in] The interpreter to find, by language.
         * @return True if found.
         */
        bool find_interpreter(const std::string &language) const;

        // No copying
        //
        SoftcodeAccess &operator=(const SoftcodeAccess &rhs);
        SoftcodeAccess(const SoftcodeAccess &rhs);


        static SoftcodeAccess *singleton_ptr;  ///< Singleton pointer
        LanguageInterpreters interpreters; ///< Pairs language to interpreter
    };
}
}

#endif //MUTGOS_SOFTCODE_SOFTCODEACCESS_H
