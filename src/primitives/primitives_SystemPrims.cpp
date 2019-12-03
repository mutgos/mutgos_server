/*
 * primitives_SystemPrims.cpp
 */

#include <string>
#include <sstream>

#include "primitives_SystemPrims.h"
#include "primitives_CommonTypes.h"
#include "primitives_Result.h"

#include "security/security_Context.h"
#include "security/security_SecurityException.h"
#include "security/security_SecurityAccess.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalPlainText.h"
#include "text/text_ExternalTextConverter.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_ActionEntity.h"
#include "dbtypes/dbtype_Exit.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "executor/executor_ExecutorAccess.h"
#include "executor/executor_ProcessStats.h"

#include "comminterface/comm_CommAccess.h"

#define TELNET_LF '\n'

namespace mutgos
{
namespace primitives
{
    // ----------------------------------------------------------------------
    SystemPrims::SystemPrims(void)
    {
    }

    // ----------------------------------------------------------------------
    SystemPrims::~SystemPrims()
    {
    }

    // ----------------------------------------------------------------------
    Result SystemPrims::get_formatted_processes(
        mutgos::security::Context &context,
        std::string &output,
        const bool throw_on_violation)
    {
        Result result;
        bool security_success = false;

        // Check security
        //
        security_success = security::SecurityAccess::instance()->security_check(
            security::OPERATION_GET_FORMATTED_PROCESSES,
            context,
            throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            output.clear();

            std::ostringstream strstream;

            // Add header at top
            //
            strstream
               << std::left << std::setw(10) << "PID"
               << std::left << std::setw(20) << "STATE"
               << std::left << std::setw(28) << "NAME"
               << std::left << std::setw(20) << "EXECUTABLE"
               << std::left << std::setw(18) << "OWNER"
               << std::endl;

            output += strstream.str();

            // Get list of site IDs
            //
            dbtype::Id::SiteIdVector sites =
                dbinterface::DatabaseAccess::instance()->get_all_site_ids();

            sites.insert(sites.begin(), 0);

            // Run get_process_stats_for_site and format
            //
            for (dbtype::Id::SiteIdVector::const_iterator site_iter =
                    sites.begin();
                site_iter != sites.end();
                ++site_iter)
            {
                const executor::ExecutorAccess::ProcessStatsVector processes =
                    executor::ExecutorAccess::instance()->
                        get_process_stats_for_site(*site_iter);

                for (executor::ExecutorAccess::ProcessStatsVector::const_iterator
                        process_iter = processes.begin();
                    process_iter != processes.end();
                    ++process_iter)
                {
                    format_process(*process_iter, output);
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Result SystemPrims::get_online_players(
        security::Context &context,
        const dbtype::Id::SiteIdType site_id,
        SystemPrims::SessionStatsVector &stats,
        const bool throw_on_violation)
    {
        Result result;
        bool security_success = false;

        // TODO Will need to split this up so that you can get an online list without knowing addresses

        // Check security
        //
        security_success = security::SecurityAccess::instance()->security_check(
            security::OPERATION_CHARACTER_ONLINE,
            context,
            throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            stats = comm::CommAccess::instance()->get_session_stats(site_id);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // Right now this is just a plain wrapper to text::ExternalTextConverter.
    Result SystemPrims::to_external_text(
        security::Context &context,
        const std::string &text,
        text::ExternalTextLine &formatted_text,
        const bool throw_on_violation)
    {
        Result result;

        const text::ExternalTextLine converted_text =
            text::ExternalTextConverter::to_external(text);

        formatted_text.reserve(formatted_text.size() + converted_text.size());

        formatted_text.insert(
            formatted_text.end(),
            converted_text.begin(),
            converted_text.end());

        return result;
    }

    // ----------------------------------------------------------------------
    // TODO Remove once temporary commands gone.  This is not supposed to be used long term.
    Result SystemPrims::to_external_text_multiline_unformatted(
        security::Context &context,
        const std::string &text,
        text::ExternalTextMultiline &multiline,
        const bool throw_on_violation)
    {
        Result result;
        text::ExternalTextLine current_line;

        if (text.empty())
        {
            // Shortcut for empty text. Just add an empty plain text to match.
            //
            current_line.push_back(
                new text::ExternalPlainText(std::string()));

            multiline.push_back(current_line);
            current_line.clear();
            return result;
        }

        size_t index = 0;
        size_t newline_index = text.find(TELNET_LF);

        if (newline_index == std::string::npos)
        {
            // Shortcut for text with no newline at all.
            //
            current_line.push_back(new text::ExternalPlainText(text));
            multiline.push_back(current_line);
            current_line.clear();
            return result;
        }
        else if ((text.size() - 1) == newline_index)
        {
            // Shortcut for text with a newline only at the very end.
            //
            current_line.push_back(new text::ExternalPlainText(
                text.substr(0, newline_index)));

            multiline.push_back(current_line);
            current_line.clear();
            return result;
        }

        // Standard case of one or more newlines within the string.
        //
        while (true)
        {
            current_line.push_back(new text::ExternalPlainText(
                text.substr(index, newline_index - index)));
            multiline.push_back(current_line);
            current_line.clear();
            index = newline_index + 1;

            if (index >= text.size())
            {
                break;
            }
            else
            {
                newline_index = text.find(TELNET_LF, index);

                if (newline_index == std::string::npos)
                {
                    // Shift the index so we get the last part before
                    // exiting.
                    newline_index = text.size();
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    // Right now this is just a plain wrapper to text::ExternalTextConverter.
    Result SystemPrims::from_external_text(
        security::Context &context,
        const text::ExternalTextLine &formatted_text,
        std::string &text,
        const bool throw_on_violation)
    {
        Result result;

        text.clear();
        text = text::ExternalTextConverter::from_external(formatted_text);

        return result;
    }

    // ----------------------------------------------------------------------
    Result SystemPrims::make_id_text(
        security::Context &context,
        const dbtype::Id &entity_id,
        text::ExternalIdText *&id_text_ptr,
        const bool throw_on_violation)
    {
        Result result;
        id_text_ptr = 0;

        if (entity_id.is_default())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        dbinterface::EntityRef entity_ref =
            dbinterface::DatabaseAccess::instance()->get_entity(entity_id);

        if (not entity_ref.valid())
        {
            result.set_status(Result::STATUS_BAD_ARGUMENTS);
            return result;
        }

        // Do security checks for entity type and name
        //
        security::SecurityAccess * const security_access =
            security::SecurityAccess::instance();

        const bool security_success =
            security_access->security_check(
                security::OPERATION_GET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_type,
                throw_on_violation) and
            security_access->security_check(
                security::OPERATION_GET_ENTITY_FIELD,
                context,
                entity_ref,
                dbtype::ENTITYFIELD_name,
                throw_on_violation);

        if (not security_success)
        {
            result.set_status(Result::STATUS_SECURITY_VIOLATION);
        }
        else
        {
            // Determine type of entity.
            //
            text::ExternalIdText::IdType entity_type =
                text::ExternalIdText::ID_TYPE_ENTITY;

            if (dynamic_cast<dbtype::Exit *>(entity_ref.get()))
            {
                // Specifically an exit
                entity_type = text::ExternalIdText::ID_TYPE_EXIT;
            }
            else if (dynamic_cast<dbtype::ActionEntity *>(entity_ref.get()))
            {
                // A generic action
                entity_type = text::ExternalIdText::ID_TYPE_ACTION;
            }

            // Create the ExternalIdText
            //
            id_text_ptr = new text::ExternalIdText(
                entity_id,
                entity_ref->get_entity_name(),
                entity_type);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    void SystemPrims::format_process(
        const executor::ProcessStats &process,
        std::string &output)
    {
        std::ostringstream strstream;

        strstream
            << std::right << std::setw(8) << process.get_pid()
            << "  "
            << std::left << std::setw(20)
            << executor::ProcessInfo::process_state_to_string(
                process.get_process_state())
            << std::left << std::setw(28) << process.get_name()
            << std::left << std::setw(20)
            << get_name(process.get_executable_id())
            << std::left << std::setw(18) << get_name(process.get_owner_id())
            << std::endl;

        output += strstream.str();
    }

    // ----------------------------------------------------------------------
    std::string SystemPrims::get_name(const dbtype::Id &id)
    {
        std::string result;

        if (id.is_default())
        {
            result = "(Invalid ID)";
        }
        else
        {
            dbinterface::EntityRef entity_ref =
                dbinterface::DatabaseAccess::instance()->get_entity(id);

            if (not entity_ref.valid())
            {
                result = "(Invalid Entity - " + id.to_string(true) + ")";
            }
            else
            {
                result = entity_ref->get_entity_name()
                    + "(" + id.to_string(true) + ")";
            }
        }

        return result;
    }
}
}

#undef TELNET_LF