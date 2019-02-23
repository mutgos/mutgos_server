/*
 * events_ProcessExecutionSubscriptionParams.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "text/text_StringConversion.h"

#include "events/events_ProcessExecutionSubscriptionParams.h"
#include "events/events_ProcessExecutionEvent.h"
#include "dbtypes/dbtype_Id.h"
#include "executor/executor_ProcessInfo.h"
#include "events/events_ProcessTypeSubscription.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    ProcessExecutionSubscriptionParams::ProcessExecutionSubscriptionParams(void)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_PROCESS_EXECUTION),
        process_id(0),
        process_type(PROCESS_SUB_NATIVE_AND_INTERPRETED)
    {
    }

    // ----------------------------------------------------------------------
    ProcessExecutionSubscriptionParams::ProcessExecutionSubscriptionParams(
        const executor::PID pid,
        const dbtype::Id &exe_id,
        const ProcessTypeSubscription type,
        const dbtype::Id &process_owner,
        const std::string &name,
        const ProcessExecutionSubscriptionParams::StateSet &states)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_PROCESS_EXECUTION),
        process_id(pid),
        executable_id(exe_id),
        process_type(type),
        owner_id(process_owner),
        process_name(name),
        process_states(states)
    {
    }

    // ----------------------------------------------------------------------
    ProcessExecutionSubscriptionParams::ProcessExecutionSubscriptionParams(
        const ProcessExecutionSubscriptionParams &rhs)
      : SubscriptionParams(rhs),
        process_id(rhs.process_id),
        executable_id(rhs.executable_id),
        process_type(rhs.process_type),
        owner_id(rhs.owner_id),
        process_name(rhs.process_name),
        process_states(rhs.process_states)
    {
    }

    // ----------------------------------------------------------------------
    ProcessExecutionSubscriptionParams::~ProcessExecutionSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    ProcessExecutionSubscriptionParams &ProcessExecutionSubscriptionParams::operator=(
        const ProcessExecutionSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);

        process_id = rhs.process_id;
        executable_id = rhs.executable_id;
        process_type = rhs.process_type;
        owner_id = rhs.owner_id;
        process_name = rhs.process_name;
        process_states = rhs.process_states;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::operator==(
        const ProcessExecutionSubscriptionParams &rhs) const
    {
        bool equals = false;

        if (SubscriptionParams::params_equal_to(rhs))
        {
            equals = (process_id == rhs.process_id) and
                (executable_id == rhs.executable_id) and
                (process_type == rhs.process_type) and
                (owner_id == rhs.owner_id) and
                (process_name == rhs.process_name) and
                (process_states == rhs.process_states);
        }

        return equals;
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::add_process_state(
        const executor::ProcessInfo::ProcessState state)
    {
        bool good_input = false;

        switch (state)
        {
            // Allowed states.
            //
            case executor::ProcessInfo::PROCESS_STATE_CREATED:
            case executor::ProcessInfo::PROCESS_STATE_KILLED:
            case executor::ProcessInfo::PROCESS_STATE_COMPLETED:
            {
                good_input = true;
                process_states.insert(state);
                break;
            }

            // everything else is not allowed.
            //
            default:
            {
                // Not allowed right now.
                break;
            }
        }

        return good_input;
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::validate() const
    {
        bool valid = true;

        // Since everything is optional, the only thing to check is
        // the process type.

        if (process_type != PROCESS_SUB_INTERPRETED_ONLY)
        {
            valid = executable_id.is_default();
        }

        if (executable_id.is_site_default())
        {
            valid = valid and executable_id.is_entity_default();
        }

        if (owner_id.is_site_default())
        {
            valid = valid and owner_id.is_entity_default();
        }

        return valid;
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *ProcessExecutionSubscriptionParams::clone(void) const
    {
        return new ProcessExecutionSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::references_id(
        const dbtype::Id &id) const
    {
        return (executable_id == id) or (owner_id == id);
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return has_site_id(site_id, executable_id) or
               has_site_id(site_id, owner_id);
    }

    // ----------------------------------------------------------------------
    std::string ProcessExecutionSubscriptionParams::to_string() const
    {
        std::ostringstream strstream;

        strstream << "ProcessExecutionSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << "PID:             "
                  << text::to_string(process_id) << std::endl
                  << "EXE ID:          " << executable_id.to_string(true)
                  << std::endl
                  << "Process Type:    " << process_type
                  << std::endl
                  << "Owner ID:        " << owner_id.to_string(true)
                  << std::endl
                  << "Process name:    " << process_name
                  << std::endl
                  << "Process states:";

        for (StateSet::const_iterator state_iter = process_states.begin();
             state_iter != process_states.end();
             ++state_iter)
        {
            strstream << "  " << *state_iter;
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool ProcessExecutionSubscriptionParams::is_match(
        const ProcessExecutionEvent *event_ptr) const
    {
        if (not event_ptr)
        {
            return false;
        }

        // Process type matching
        //
        bool match = process_type_subscription_match(
            event_ptr->is_native(),
            process_type);

        // Process ID matching
        //
        if (match and process_id)
        {
            match = (event_ptr->get_process_id() == process_id);
        }

        if (match and (not executable_id.is_default()))
        {
            // Executable ID matching - non-native
            //
            if (executable_id.is_entity_default())
            {
                // Specific site
                match = event_ptr->get_executable_id().get_site_id() ==
                    executable_id.get_site_id();
            }
            else
            {
                // Specific executable Entity
                match = event_ptr->get_executable_id() == executable_id;
            }
        }

        // Owner matching
        //
        if (match and (not owner_id.is_default()))
        {
            if (owner_id.is_entity_default())
            {
                // Specific site
                match = event_ptr->get_owner_id().get_site_id() ==
                        owner_id.get_site_id();
            }
            else
            {
                // Specific owner Entity
                match = event_ptr->get_owner_id() == owner_id;
            }
        }

        // Process name matching
        //
        if (match and (not process_name.empty()))
        {
            match = event_ptr->get_process_name() == process_name;
        }

        // Process state matching
        //
        if (match and (not process_states.empty()))
        {
            match = process_states.find(event_ptr->get_process_state()) !=
                process_states.end();
        }

        return match;
    }
}
}
