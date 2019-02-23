/*
 * events_ProcessExecutionSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_PROCESSEXECUTIONSUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_PROCESSEXECUTIONSUBSCRIPTIONPARAMS_H

#include "dbtypes/dbtype_Id.h"

#include "events/events_SubscriptionParams.h"
#include "executor/executor_ProcessInfo.h"
#include "events/events_ProcessTypeSubscription.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class ProcessExecutionEvent;

    /**
     * A process execution status subscription.  This allows the subscriber
     * to watch for process creation and completion, though in the future
     * other states may be available to watch for.
     *
     * Fields that are left at defaults (or empty) are considered wildcards.
     * For an example, not filling anything in will notify the listener of
     * any process creation or completion anywhere.
     *
     * Note that if watching for process activity whose exe ID is a global
     * interpreted program, if an owner is not specified then all worlds will
     * match.
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * cannot be unset.
     *
     * The states that are allowed to be listened for are:
     * PROCESS_STATE_CREATED   - Process creation
     * PROCESS_STATE_KILLED    - Process is being forcibly killed or errored out
     * PROCESS_STATE_COMPLETED - Process completed (killed or not)
     */
    class ProcessExecutionSubscriptionParams : public SubscriptionParams
    {
    public:
        /** A set of interested process states */
        typedef std::set<executor::ProcessInfo::ProcessState> StateSet;

        /**
         * Constructor with nothing set.
         */
        ProcessExecutionSubscriptionParams(void);

        /**
         * Constructor that sets everything.
         * @param pid[in] The PID interested in, or 0 if all.
         * @param exe_id[in] The ID of the executable program in the database,
         * or default ID if all or native.  Like process_owner, you can
         * specify just the site to indicate all programs from a certain site,
         * or default for all programs.
         * @param type[in] The type of process interested in.  Note that if
         * an exe_id is specified, this must be set to 'interpreted'.
         * @param process_owner[in] ID of the process owner, or default for
         * all across all sites.  If desired for all processes on a single
         * site, specify the site ID but leave the Entity ID at default.
         * @param name[in] The name of the process.  Must be an exact match.
         * Empty string for all.
         * @param states[in] The interested process states, or empty for all.
         */
        ProcessExecutionSubscriptionParams(
            const executor::PID pid,
            const dbtype::Id &exe_id,
            const ProcessTypeSubscription type,
            const dbtype::Id &process_owner,
            const std::string &name,
            const StateSet &states);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        ProcessExecutionSubscriptionParams(
            const ProcessExecutionSubscriptionParams &rhs);

        /**
         * Virtual destructor.
         */
        virtual ~ProcessExecutionSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        ProcessExecutionSubscriptionParams &operator=(
            const ProcessExecutionSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const ProcessExecutionSubscriptionParams &rhs) const;

        /**
         * Sets the process ID interested in, or default (0) for all.
         * Optional.
         * @param pid[in] The process ID interested in.
         */
        void set_process_id(const executor::PID pid)
          { process_id = pid; }

        /**
         * @return The process ID interested in.
         */
        const executor::PID &get_process_id(void) const
          { return process_id; }

        /**
         * Sets The executable ID interested in, or default for all.  If
         * the executable ID is set to default, the type may be set to native.
         * If desired for all executables on a single site only, specify the
         * site ID but leave the Entity ID at default.
         * Optional.
         * @param id[in] The executable ID interested in.
         * @param type[in] The type of process interested in.  If id is set
         * to non-default, then this must be set to 'interpreted' (default).
         */
        void set_executable_id(
            const dbtype::Id &id,
            const ProcessTypeSubscription type = PROCESS_SUB_INTERPRETED_ONLY)
          { executable_id = id;
            process_type = type; }

        /**
         * @return The executable ID.
         */
        const dbtype::Id &get_executable_id(void) const
          { return executable_id; }

        /**
         * @return What types of processes the subscription is interested in.
         */
        bool get_process_type(void) const
          { return process_type; }

        /**
         * Sets the owner of the processes interested in.  If site ID is
         * default, all processes on all sites will be watched for.
         * If desired for all processes on a single site only, specify the
         * site ID but leave the Entity ID at default.
         * Optional.
         * @param id[in] The owner whose processes are interested in.
         */
        void set_owner_id(const dbtype::Id &id)
          { owner_id = id; }

        /**
         * @return The owner of the processes.
         */
        const dbtype::Id &get_owner_id(void) const
          { return owner_id; }

        /**
         * Sets the exact match process name interested in.
         * Optional.
         * @param name[in] The process name.
         */
        void set_process_name(const std::string &name)
          { process_name = name; }

        /**
         * @return The process name.
         */
        const std::string &get_process_name(void) const
          { return process_name; }

        /**
         * Adds the given process state as interested in.
         * @param state[in] The state to add.
         * @return True if state is valid to watch for and was added, false
         * if state is not valid.
         */
        bool add_process_state(const executor::ProcessInfo::ProcessState state);

        /**
         * Validates that the subscription is valid (has all needed fields
         * filled in and that they are properly filled in.
         * @return True if subscription is valid, false if there is a problem.
         */
        virtual bool validate(void) const;

        /**
         * @return A copy of this subscription.  Caller is responsible for
         * managing the pointer.
         */
        virtual SubscriptionParams *clone(void) const;

        /**
         * @param id[in] The ID to check.
         * @return True if the subscription parameters specifically reference
         * the given ID anywhere.
         */
        virtual bool references_id(const dbtype::Id &id) const;

        /**
         * @param site_id[in] The site ID to check.
         * @return True if the subscription parameters specifically reference
         * the given site ID anywhere, including in entity IDs.
         */
        virtual bool references_site(
            const dbtype::Id::SiteIdType site_id) const;

        /**
         * @return The subscription as a string, for diagnostic/logging
         * purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * Evaluates the event and determine if it matches this subscription.
         * @param event_ptr[in] The event to evaluate.
         * @return True if subscription matches event.
         */
        virtual bool is_match(const ProcessExecutionEvent *event_ptr) const;

    private:
        executor::PID process_id; ///< Optional process ID to watch for
        dbtype::Id executable_id; ///< Optional related ID for executable data
        ProcessTypeSubscription process_type; ///< What type(s) of processes we want
        dbtype::Id owner_id; ///< Optional process owner
        std::string process_name; ///< Optional process name
        StateSet process_states; ///< Optional states interested in
    };
}
}

#endif //MUTGOS_EVENTS_PROCESSEXECUTIONSUBSCRIPTIONPARAMS_H

