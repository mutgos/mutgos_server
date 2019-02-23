/*
 * security_Context.h
 */

#ifndef MUTGOS_SECURITY_CONTEXT_H
#define MUTGOS_SECURITY_CONTEXT_H

#include <map>
#include <vector>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_EntityType.h"
#include "dbtypes/dbtype_EntityField.h"

#include "executor/executor_CommonTypes.h"

#include "security_OperationsCapabilities.h"

namespace mutgos
{
namespace security
{
    // TODO May need to recheck capabilities if program is long running

    /**
     * A security context gives the security subsystem a frame of reference
     * when evaluating a security condition.  Basically, it answers the
     * questions of 'who is doing the operation?', 'what capabilities
     * do they have?'  To this end, it provides basic caching of assigned
     * capabilities.
     *
     * This should be created when starting a program and shall be destructed
     * when the program has completed.  For non-softcode uses, the Context may
     * be kept and reused, however if the assigned capabilities change, it
     * will not take effect until re-instantiated or reset.
     */
    class Context
    {
    public:
        /** First is true if cache contains a result, second is the result */
        typedef std::pair<bool, Result> CachedResult;

        /**
         * Constructs a Context.  This is the only method users should call;
         * the rest are reserved for the security subsystem.
         * @param requesting_entity[in] The entity the request is being made
         * for.  This must always be a container property entity (Player,
         * Guest, Puppet, etc) and must always be valid.
         * @param requesting_program[in] The program operating on behalf of the
         * entity that is making the request. (optional)
         */
        Context(
            const dbtype::Id requesting_entity,
            const dbtype::Id requesting_program);

        /**
         * Destructor.
         */
        ~Context();

        /**
         * Clears out the capabilities and marks context as not populated.
         * This is used when a Context is long running and needs to be
         * refreshed with an updated set.
         */
        void reset_capabilities(void);

        /**
         * Sets the PID.
         * @param prog_pid[in] The program's PID.
         */
        void set_pid(const executor::PID prog_pid)
        { pid = prog_pid; }

        /**
         * @return The PID, or 0 (default) if not set.
         */
        executor::PID get_pid(void) const
        { return pid; }

        /**
         * For use by security subsystem only.
         * @return Internal state as a string, for logging/debugging purposes.
         */
        std::string to_string(void) const;

        /**
         * For use by security subsystem only.
         * @return The Entity requesting the operation (indirectly or directly).
         */
        const dbtype::Id &get_requester(void) const
        { return requester; }

        /**
         * For use by security subsystem only.
         * @return The program requesting the operation, or default if none
         * (typically if native program).
         */
        const dbtype::Id &get_program(void) const
        { return program; }

        /**
         * For use by security subsystem only.
         * Sets the 'populated_capabilities' flag to indicate all capabilities
         * and run-as settings have been populated and cached.
         */
        void capabilities_set(void)
        { populated_capabilities = true; }

        /**
         * For use by security subsystem only.
         * Adds the given capability to the capability list.  Duplicates
         * are ignored.
         * @param capability[in] The capability to add.
         */
        void add_capability(const Capability capability);

        /**
         * For use by security subsystem only.
         * Sets the 'admin' flag to true, indicating requester should be
         * treated as an admin.
         */
        void set_run_as_admin(void)
        { admin = true; }

        /**
         * For use by security subsystem only.
         * Sets the 'run as requester' flag to true, indicating program will
         * run with the permissions of the requester.
         */
        void set_run_as_requester(void)
        { run_as_requester = true; }

        /**
         * For use by security subsystem only.
         * @return True if all capabilities and other settings have been
         * populated/cached.
         */
        bool has_capabilities_set(void) const
        { return populated_capabilities; }

        /**
         * For use by security subsystem only.
         * Note that when 'admin' is true, this will return true for every
         * capability checked.
         * @param capability[in] The capability to check.
         * @return True if context has the capability.
         */
        bool has_capability(const Capability capability) const;

        /**
         * For use by security subsystem only.
         * @return True if running with full admin privileges.
         */
        bool has_admin(void) const
        { return admin; }

        /**
         * For use by security subsystem only.
         * @return True if running with privileges of requester, false
         * if running as 'other'.
         */
        bool has_run_as_requester(void) const
        { return run_as_requester; }

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            Result &result) const;

        /**
         * Saves the result of a security check to the cache.  Older entries
         * may be overwritten.
         * @param operation[in] The operation being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const Result result);

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param entity_type[in] The type of entity being checked.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            const dbtype::EntityType entity_type,
            Result &result) const;

        /**
         * Saves the result of a security check to the cache.  Older entries
         * may be overwritten.
         * @param operation[in] The operation being checked.
         * @param entity_type[in] The type of entity being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const dbtype::EntityType entity_type,
            const Result result);

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param entity_target[in] The Entity being checked.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            const dbtype::Id &entity_target,
            Result &result) const;

        /**
         * Saves the result of a security check to the cache.  Older entries
         * may be overwritten.
         * @param operation[in] The operation being checked.
         * @param entity_target[in] The Entity being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const dbtype::Id &entity_target,
            const Result result);

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param entity_target[in] The Entity being checked.
         * @param application[in] The application or property on the
         * entity_target being checked.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            const dbtype::Id &entity_target,
            const std::string &application,
            Result &result) const;

        /**
         * Saves the result of a security check to the cache.  Older entries
         * may be overwritten.
         * @param operation[in] The operation being checked.
         * @param entity_target[in] The Entity being checked.
         * @param application[in] The application or property on the
         * entity_target being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const dbtype::Id &entity_target,
            const std::string &application,
            const Result result);

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param entity_target[in] The Entity being checked.
         * @param entity_field[in] The field on the entity_target being
         * checked.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            const dbtype::Id &entity_target,
            const dbtype::EntityField entity_field,
            Result &result) const;

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation being checked.
         * @param entity_target[in] The Entity being checked.
         * @param entity_field[in] The Entity field being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const dbtype::Id &entity_target,
            const dbtype::EntityField entity_field,
            const Result result);

        /**
         * Checks security result cache with the given parameters.
         * @param operation[in] The operation to check.
         * @param entity_target[in] The destination or target Entity being
         * checked.
         * @param entity_source[in] The source of the entity_target, or the
         * source Entity being moved to entity_target.
         * @param result[out] If matching entry, this will be set to the
         * cached result of the check.  If false, no change made.
         * @return True if matching cache entry found.
         */
        bool security_check_cache(
            const Operation operation,
            const dbtype::Id &entity_target,
            const dbtype::Id &entity_source,
            Result &result) const;

        /**
         * Saves the result of a security check to the cache.  Older entries
         * may be overwritten.
         * @param operation[in] The operation being checked.
         * @param entity_target[in] The Entity being checked.
         * @param entity_source[in] The source of entity_target being checked,
         * or the source Entity being moved to entity_target being checked.
         * @param result[in] The result of the check.
         */
        void save_security_check_result(
            const Operation operation,
            const dbtype::Id &entity_target,
            const dbtype::Id &entity_source,
            const Result result);

        /**
         * Removes all entries in the security cache.
         */
        void clear_security_check_cache(void);

    private:
        /**
         * Adds an operation result entry (generic) into the cache, overwriting
         * the oldest if needed.  Not every parameter is valid for every
         * operation (use defaults if needed).
         * @param operation_result[in] Result of the security check.
         * @param operation[in] The operation checked.
         * @param entity_type[in] Type of entity being operated on.
         * @param entity_field[in] The entity field being operated on.
         * @param application_name[in] Application being operated on.
         * @param entity_target[in] Target entity being operated on
         * @param entity_source[in] Source entity being operated on
         */
        void add_cache_entry(
            const Result operation_result,
            const Operation operation,
            const dbtype::EntityType entity_type,
            const dbtype::EntityField entity_field,
            const std::string &application_name,
            const dbtype::Id &entity_target,
            const dbtype::Id &entity_source);

        /**
         * Simple container class that has cached security data for a
         * particular operation and parameters.
         */
        class ContextCacheEntry
        {
        public:
            /**
             * Default constructor for STL.
             */
            ContextCacheEntry(void);

            Result operation_result; ///< Result of the original security check
            Operation operation; ///< Operation being checked

            dbtype::EntityType entity_type; ///< Type of entity being operated on
            dbtype::EntityField entity_field; ///< The entity field being operated on

            std::string application_name; ///< Application being operated on

            dbtype::Id entity_target; ///< Target entity being operated on
            dbtype::Id entity_source; ///< Source entity being operated on
        };

        typedef std::vector<Capability> Capabilities;
        typedef std::vector<ContextCacheEntry> ContextCache;

        const dbtype::Id requester; ///< Entity requesting operation (indirectly via program or directly)
        const dbtype::Id program; ///< Softcode Program running requesting operation (optional)
        executor::PID pid; ///< Program's PID.
        bool populated_capabilities; ///< True if known capabilities and run as have been added
        bool admin; ///< True if running with full admin rights
        bool run_as_requester; ///< True if the program is running-as the requester, false for 'other'
        Capabilities capabilities; ///< Aggregated special capabilities

        MG_SignedInt context_cache_index; ///< What index to overwrite next when full.
        ContextCache context_cache; ///< Cached security checks and their result
    };
}
}

#endif //MUTGOS_SECURITY_CONTEXT_H
