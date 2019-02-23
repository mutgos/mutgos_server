/*
 * softcode_SoftcodeAccess.cpp
 */

#include <string>
#include <vector>
#include <map>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "dbtypes/dbtype_Id.h"
#include "executor/executor_CommonTypes.h"
#include "security/security_Context.h"
#include "security/security_SecurityAccess.h"
#include "channels/events_TextChannel.h"

#include "dbinterface/dbinterface_EntityRef.h"
#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbtypes/dbtype_Program.h"

#include "softcode_Interpreter.h"
#include "softcode_SoftcodeAccess.h"

namespace mutgos
{
namespace softcode
{
    // Statics
    //
    SoftcodeAccess *SoftcodeAccess::singleton_ptr = 0;


    // ----------------------------------------------------------------------
    SoftcodeAccess* SoftcodeAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new SoftcodeAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void SoftcodeAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::register_language(Interpreter * const interpreter)
    {
        bool success = false;

        if (not interpreter)
        {
            LOG(error, "softcode", "register_language",
                "Null interpreter provided!");
        }
        else
        {
            // See if already registered.
            //
            if (find_interpreter(interpreter->get_language_name()))
            {
                // Already registered.  Abort.
                LOG(error, "softcode", "register_language",
                    "Language " + interpreter->get_language_name()
                    + " already registered!");
            }
            else
            {
                // Add to the list!
                //
                interpreters.push_back(std::make_pair(
                    interpreter->get_language_name(),
                    interpreter));
                success = true;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    void SoftcodeAccess::unregister_language(Interpreter * const interpreter)
    {
        if (not interpreter)
        {
            LOG(error, "softcode", "unregister_language",
                "Null interpreter provided!");
        }
        else
        {
            LanguageInterpreters::iterator iter;

            if (find_interpreter(interpreter->get_language_name(), iter))
            {
                interpreters.erase(iter);
            }
            else
            {
                LOG(error, "softcode", "unregister_language",
                    "Could not find language "
                    + interpreter->get_language_name());
            }
        }
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::is_language_registered(
        const std::string &language) const
    {
        return find_interpreter(language);
    }

    // ----------------------------------------------------------------------
    SoftcodeAccess::RegisteredLanguages SoftcodeAccess::get_registered_languages(
        void) const
    {
        RegisteredLanguages languages;
        languages.reserve(interpreters.size());

        for (LanguageInterpreters::const_iterator iter = interpreters.begin();
             iter != interpreters.end();
             ++iter)
        {
            languages.push_back(iter->first);
        }

        return languages;
    }

    // ----------------------------------------------------------------------
    executor::PID SoftcodeAccess::make_process(
        security::Context *security_context_ptr,
        const std::string &command_name,
        const std::string &arguments,
        events::TextChannel *outgoing_channel_ptr,
        events::TextChannel *incoming_channel_ptr)
    {
        executor::PID pid = 0;

        // Validate inputs.
        if (not security_context_ptr)
        {
            LOG(error, "softcode", "make_process",
                "Security context is null!");
            return pid;
        }

        if (security_context_ptr->get_requester().is_default() or
            security_context_ptr->get_program().is_default())
        {
            LOG(error, "softcode", "make_process",
                "Requester or program have invalid IDs!");

            delete security_context_ptr;
            return pid;
        }

        // Confirm it is actually a program that we can execute
        //
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(
                security_context_ptr->get_program());

        if (not program_ref.valid())
        {
            LOG(error, "softcode", "make_process",
                "Cannot find program with ID "
                + security_context_ptr->get_program().to_string(true));

            delete security_context_ptr;
            return pid;
        }

        dbtype::Program * const program_ptr =
            dynamic_cast<dbtype::Program *>(program_ref.get());

        if (not program_ptr)
        {
            LOG(error, "softcode", "make_process",
                "Not a program: "
                + security_context_ptr->get_program().to_string(true));

            delete security_context_ptr;
            return pid;
        }

        // Look up interpreter
        //
        LanguageInterpreters::iterator interpreter_iter;

        if (not find_interpreter(
            program_ptr->get_program_language(),
            interpreter_iter))
        {
            LOG(error, "softcode", "make_process",
                "No interpreter has registered to handle language "
                + program_ptr->get_program_language() + " for "
                + security_context_ptr->get_program().to_string(true));

            delete security_context_ptr;
            return pid;
        }

        // Fill out security context with current permissions
        //
        security::SecurityAccess::instance()->populate_context_capabilities(
            *security_context_ptr);

        // Call interpreter
        //
        pid = interpreter_iter->second->make_process(
            security_context_ptr,
            command_name,
            arguments,
            outgoing_channel_ptr,
            incoming_channel_ptr);

        return pid;
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::compile(
        const dbtype::Id &program_id,
        events::TextChannel *outgoing_channel_ptr)
    {
        bool success = false;

        if (program_id.is_default())
        {
            LOG(error, "softcode", "compile", "Program is an invalid ID!");
            return success;
        }

        // Confirm it is actually a program that we can compile
        //
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);

        if (not program_ref.valid())
        {
            LOG(error, "softcode", "compile",
                "Cannot find program with ID " + program_id.to_string(true));
            return success;
        }

        dbtype::Program * const program_ptr =
            dynamic_cast<dbtype::Program *>(program_ref.get());

        if (not program_ptr)
        {
            LOG(error, "softcode", "compile",
                "Not a program: " + program_id.to_string(true));
            return success;
        }

        // Look up interpreter
        //
        LanguageInterpreters::iterator interpreter_iter;

        if (not find_interpreter(
            program_ptr->get_program_language(),
            interpreter_iter))
        {
            LOG(error, "softcode", "compile",
                "No interpreter has registered to handle language "
                + program_ptr->get_program_language() + " for "
                + program_id.to_string(true));
            return success;
        }

        // Call interpreter
        //
        success =
            interpreter_iter->second->compile(program_id, outgoing_channel_ptr);

        return success;
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::uncompile(const dbtype::Id &program_id)
    {
        bool success = false;

        if (program_id.is_default())
        {
            LOG(error, "softcode", "uncompile", "Program is an invalid ID!");
            return success;
        }

        // Confirm it is actually a program that we can compile
        //
        dbinterface::EntityRef program_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(program_id);

        if (not program_ref.valid())
        {
            LOG(error, "softcode", "uncompile",
                "Cannot find program with ID " + program_id.to_string(true));
            return success;
        }

        dbtype::Program * const program_ptr =
            dynamic_cast<dbtype::Program *>(program_ref.get());

        if (not program_ptr)
        {
            LOG(error, "softcode", "uncompile",
                "Not a program: " + program_id.to_string(true));
            return success;
        }

        // Look up interpreter
        //
        LanguageInterpreters::iterator interpreter_iter;

        if (not find_interpreter(
            program_ptr->get_program_language(),
            interpreter_iter))
        {
            LOG(error, "softcode", "uncompile",
                "No interpreter has registered to handle language "
                + program_ptr->get_program_language() + " for "
                + program_id.to_string(true));
            return success;
        }

        // Call interpreter
        //
        success = interpreter_iter->second->uncompile(program_id);

        return success;
    }

    // ----------------------------------------------------------------------
    SoftcodeAccess::SoftcodeAccess(void)
    {
    }

    // ----------------------------------------------------------------------
    SoftcodeAccess::~SoftcodeAccess()
    {
        if (not interpreters.empty())
        {
            LOG(warning, "softcode", "~SoftcodeAccess",
                "There are still registered interpreters!");
        }
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::find_interpreter(
        const std::string &language,
        SoftcodeAccess::LanguageInterpreters::iterator &iter)
    {
        bool found = false;

        for (iter = interpreters.begin(); iter != interpreters.end(); ++iter)
        {
            if (iter->first == language)
            {
                found = true;
                break;
            }
        }

        return found;
    }

    // ----------------------------------------------------------------------
    bool SoftcodeAccess::find_interpreter(const std::string &language) const
    {
        bool found = false;

        for (LanguageInterpreters::const_iterator iter = interpreters.begin();
            iter != interpreters.end();
            ++iter)
        {
            if (iter->first == language)
            {
                found = true;
                break;
            }
        }

        return found;
    }
}
}
