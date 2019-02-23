/*
 * primitives_SystemPrims.h
 */

#ifndef MUTGOS_PRIMITIVES_SYSTEMPRIMS_H
#define MUTGOS_PRIMITIVES_SYSTEMPRIMS_H

#include <string>

#include "primitives_CommonTypes.h"
#include "primitives_Result.h"

#include "text/text_ExternalText.h"
#include "text/text_ExternalIdText.h"

#include "security/security_Context.h"
#include "executor/executor_ProcessStats.h"
#include "dbtypes/dbtype_Id.h"
#include "comminterface/comm_CommAccess.h"

namespace mutgos
{
namespace primitives
{
    /**
     * Contains primitives related to system functions, such as processes,
     * who is online, etc.  Anything having to do with the system or site
     * as a whole will go here.
     */
    class SystemPrims
    {
    public:
        typedef comm::CommAccess::SessionStatsVector SessionStatsVector;

        /**
         * Constructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        SystemPrims(void);

        /**
         * Destructor.  Not for client use.  Only the 'access' class will use
         * this.
         */
        ~SystemPrims();

        /**
         * Outputs a formatted list of all known processes in the system.
         * This is a TEMPORARY primitive for the prototype and will be
         * replaced with more specific versions later.
         * @param context[in] The execution context.
         * @param output[out] If successful, replaced with a formatted
         * list of all processes in the system and their status.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result get_formatted_processes(
            security::Context &context,
            std::string &output,
            const bool throw_on_violation = true);

        /**
         * Gets a list of all currently online players. including metadata
         * such as idle time, how long they've been online, etc.
         * @param context[in] The execution context.
         * @param site_id[in] The site ID to get the online list for.
         * @param stats[out] A list of IDs and other metadata related to those
         * currently online.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result get_online_players(
            security::Context &context,
            const dbtype::Id::SiteIdType site_id,
            SessionStatsVector &stats,
            const bool throw_on_violation = true);

        /**
         * Converts a plain string (with or without markup) to an
         * ExternalTextLine.
         * @param context[in] The execution context.
         * @param text[in] The (possibly marked up) text to convert.
         * @param formatted_text[out] The text converted to an
         * ExternalTextLine.  Anything currently in this argument will not
         * be erased; new entries will only be appended.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result to_external_text(
            security::Context &context,
            const std::string &text,
            text::ExternalTextLine &formatted_text,
            const bool throw_on_violation = true);

        /**
         * Takes a string with lines delinated by CRs, and converts it to
         * a multiline ExternalText.  All entries will be of type
         * ExternalPlainText for onw.
         * @param context[in] The execution context.
         * @param text[in] The text to convert.
         * @param multiline[out] The converted text will be appended to this.
         * Anything currently in this argument will not be erased; new entries
         * will only be appended.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result to_external_text_multiline_unformatted(
            security::Context &context,
            const std::string &text,
            text::ExternalTextMultiline &multiline,
            const bool throw_on_violation = true);

        /**
         * Converts an ExternalTextLine to a marked up plain string.
         * @param context[in] The execution context.
         * @param formatted_text[in] The external formatted text to
         * convert.
         * @param text[out] This will be cleared and replaced with the
         * marked up text.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result from_external_text(
            security::Context &context,
            const text::ExternalTextLine &formatted_text,
            std::string &text,
            const bool throw_on_violation = true);

        /**
         * Creates an ExternalIdText with everything filled in.
         * @param context[in] The execution context.
         * @param entity_id[in] The entity to turn into an ExternalIdText.
         * @param id_text_ptr[out] The created ExternalIdText will be set
         * here.  Any existing pointer will be overwritten!  Caller is
         * responsible for managing the pointer!  Will be null if error.
         * @param throw_on_violation[in] If true (default), throw a
         * SecurityException if a security violation occurred.
         * @return If the primitive succeeded or not.
         * @throws security::SecurityException If throw_on_violation is true
         * and security denied the execution.
         */
        Result make_id_text(
            security::Context &context,
            const dbtype::Id &entity_id,
            text::ExternalIdText *&id_text_ptr,
            const bool throw_on_violation = true);

    private:
        /**
         * Takes process info and formats it into something user-readable.
         * @param process[in] The process to be formatted.
         * @param output[out] What to append the formatted output to.
         */
        void format_process(
            const executor::ProcessStats &process,
            std::string &output);

        /**
         * Given an ID, return the name of the Entity plus the ID number.
         * Security checks are not done.
         * @param id[in] The ID to convert to a name.
         * @return The formatted name.
         */
        std::string get_name(const dbtype::Id &id);
    };
}
}

#endif //MUTGOS_PRIMITIVES_SYSTEMPRIMS_H
