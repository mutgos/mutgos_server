
#include <string>
#include <chrono>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include "text/text_StringConversion.h"

#include "executor/executor_ProcessInfo.h"
#include "executor/executor_ProcessMessage.h"
#include "executor/executor_ProcessResource.h"
#include "logging/log_Logger.h"

namespace
{
    const static std::string PROCESS_STATE_AS_STRING[] =
    {
        "EXECUTING",
        "READY",
        "WAIT_MESSAGE",
        "SLEEPING",
        "SUSPENDED",
        "BLOCKED",
        "SCHEDULING",
        "KILLED",
        "COMPLETED",
        "CREATED",
        "INVALID"
    };
}

namespace mutgos
{
namespace executor
{
    // ----------------------------------------------------------------------
    const std::string &ProcessInfo::process_state_to_string(
        const ProcessInfo::ProcessState state)
    {
        if ((state >= PROCESS_STATE_END_INVALID) or
            (state < PROCESS_STATE_EXECUTING))
        {
            return PROCESS_STATE_AS_STRING[PROCESS_STATE_END_INVALID];
        }

        return PROCESS_STATE_AS_STRING[state];
    }

    // ----------------------------------------------------------------------
    ProcessInfo::ProcessInfo(
        const PID pid,
        Process * const process,
        const dbtype::Id &db_executable_id,
        const dbtype::Id &db_owner_id)
        : my_pid(pid),
          process_ptr(process),
          my_db_executable_id(db_executable_id),
          my_db_owner_id(db_owner_id),
          process_state(ProcessInfo::PROCESS_STATE_CREATED),
          pending_killed(false),
          pending_suspended(false),
          daemon(false)
    {
        if (not process)
        {
            LOG(fatal, "executor", "ProcessInfo",
                "process pointer is null!");
        }

        if (not pid)
        {
            LOG(fatal, "executor", "ProcessInfo",
                "pid is invalid!");
        }
    }

    // ----------------------------------------------------------------------
    ProcessInfo::~ProcessInfo()
    {
        cleanup();
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::lock(void)
    {
        try
        {
            info_lock.lock();
            return true;
        }
        catch (...)
        {
            LOG(fatal, "executor", "lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::try_lock(void)
    {
        try
        {
            if (info_lock.try_lock())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (...)
        {
            LOG(fatal, "executor", "try_lock",
                "Trying to get exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::try_lock_shared(void)
    {
        // TODO Right now just using a simple lock.  This is here for future expansion.
        return try_lock();
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::lock_shared(void)
    {
        // TODO Right now just using a simple lock.  This is here for future expansion.
        return lock();
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::unlock(void)
    {
        try
        {
            info_lock.unlock();
            return true;
        }
        catch (...)
        {
            LOG(fatal, "executor", "unlock",
                "Trying to release exclusive lock threw an exception!");
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::unlock_shared(void)
    {
        // TODO Right now just using a simple lock.  This is here for future
        // expansion.
        return unlock();
    }

    // ----------------------------------------------------------------------
    ProcessInfo::ProcessState ProcessInfo::get_process_state(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return process_state;
        }
        else
        {
            LOG(fatal, "executor", "get_process_state",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return PROCESS_STATE_CREATED;
    }

    // ----------------------------------------------------------------------
    ProcessInfo::ProcessState ProcessInfo::get_process_state(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_process_state(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_process_state(
        const ProcessInfo::ProcessState state,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            process_state = state;
            return true;
        }
        else
        {
            LOG(fatal, "executor", "set_process_state",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_process_state(
        const ProcessInfo::ProcessState state)
    {
        concurrency::WriterLockToken token(*this);

        return set_process_state(state, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_pending_killed(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return pending_killed;
        }
        else
        {
            LOG(fatal, "executor", "get_pending_killed",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_pending_killed(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_pending_killed(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_pending_killed(
        const bool killed,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            pending_killed = killed;
            return true;
        }
        else
        {
            LOG(fatal, "executor", "set_pending_killed",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_pending_killed(
        const bool killed)
    {
        concurrency::WriterLockToken token(*this);

        return set_pending_killed(killed, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_pending_suspended(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return pending_suspended;
        }
        else
        {
            LOG(fatal, "executor", "get_pending_suspended",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_pending_suspended(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_pending_suspended(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_pending_suspended(
        const bool suspended,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            pending_suspended = suspended;
            return true;
        }
        else
        {
            LOG(fatal, "executor", "set_pending_suspended",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_pending_suspended(const bool suspended)
    {
        concurrency::WriterLockToken token(*this);

        return set_pending_suspended(suspended, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_daemon(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return daemon;
        }
        else
        {
            LOG(fatal, "executor", "get_daemon",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::get_daemon(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_daemon(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_daemon(
        const bool is_daemon,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            daemon = is_daemon;
            return true;
        }
        else
        {
            LOG(fatal, "executor", "set_daemon",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_daemon(const bool is_daemon)
    {
        concurrency::WriterLockToken token(*this);

        return set_daemon(is_daemon, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::cleanup(concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            // Delete all messages
            //
            while (not waiting_messages.empty())
            {
                delete waiting_messages.front().second;
                waiting_messages.pop();
            }

            if (not resources.empty())
            {
                LOG(error, "executor", "cleanup",
                    "Resources are still present.  PID "
                    + text::to_string(my_pid));
            }

            // Free all resources
            //
            resources.clear();
            blocked_resources.clear();

            return true;
        }
        else
        {
            LOG(fatal, "executor", "cleanup",
                "Using the wrong lock token!  PID "
                  + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::cleanup(void)
    {
        concurrency::WriterLockToken token(*this);

        return cleanup(token);
    }

    // ----------------------------------------------------------------------
    ProcessInfo::WakeupTimePoint ProcessInfo::get_wakeup_time(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return wakeup_time;
        }
        else
        {
            LOG(fatal, "executor", "get_wakeup_time",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return std::chrono::steady_clock::now();
    }

    // ----------------------------------------------------------------------
    ProcessInfo::WakeupTimePoint ProcessInfo::get_wakeup_time(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_wakeup_time(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_sleep_time_offset(
        const osinterface::OsTypes::UnsignedInt offset_ms,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            wakeup_time =
                std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(offset_ms);

            return true;
        }
        else
        {
            LOG(fatal, "executor", "set_sleep_time_offset",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::set_sleep_time_offset(
        const osinterface::OsTypes::UnsignedInt offset_ms)
    {
        concurrency::WriterLockToken token(*this);

        return set_sleep_time_offset(offset_ms, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_message(
        ProcessMessage *message_ptr,
        concurrency::WriterLockToken &token)
    {
        return add_message(message_ptr, 0, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_message(ProcessMessage *message_ptr)
    {
        return add_message(message_ptr, 0);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_message(
        ProcessMessage *message_ptr,
        const RID rid,
        concurrency::WriterLockToken &token)
    {
        bool result = true;

        if (not message_ptr)
        {
            LOG(fatal, "executor", "add_message",
                "Message pointer is null!  PID "
                + text::to_string(my_pid));
        }
        else if (token.has_lock(*this))
        {
            LOG(debug, "executor", "add_message",
                "Adding message for PID "
                + text::to_string(my_pid)
                + " with RID "
                + text::to_string(rid));

            if (not rid)
            {
                waiting_messages.push(std::make_pair(rid, message_ptr));
            }
            else
            {
                // Confirm this RID is actually ours.
                if (resources.find(rid) == resources.end())
                {
                    // Not ours!
                    result = false;

                    LOG(error, "executor", "add_message",
                        "Using unknown RID!  PID "
                        + text::to_string(my_pid)
                        + ", RID "
                        + text::to_string(rid));
                }
                else
                {
                    waiting_messages.push(std::make_pair(rid, message_ptr));
                    remove_blocked_resource(rid);
                }
            }

            return true;
        }
        else
        {
            LOG(fatal, "executor", "add_message",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_message(ProcessMessage *message_ptr, const RID rid)
    {
        concurrency::WriterLockToken token(*this);

        return add_message(message_ptr, rid, token);
    }

    // ----------------------------------------------------------------------
    ProcessMessage *ProcessInfo::get_next_message(
        concurrency::WriterLockToken &token)
    {
        RID rid;

        return get_next_message(rid, token);
    }

    // ----------------------------------------------------------------------
    ProcessMessage *ProcessInfo::get_next_message(void)
    {
        RID rid;

        return get_next_message(rid);
    }

    // ----------------------------------------------------------------------
    ProcessMessage *ProcessInfo::get_next_message(
        RID &rid,
        concurrency::WriterLockToken &token)
    {
        ProcessMessage *next_message_ptr = 0;

        if (token.has_lock(*this))
        {
            rid = waiting_messages.front().first;
            next_message_ptr = waiting_messages.front().second;
            waiting_messages.pop();
        }
        else
        {
            LOG(fatal, "executor", "get_next_message",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return next_message_ptr;
    }

    // ----------------------------------------------------------------------
    ProcessMessage* ProcessInfo::get_next_message(RID &rid)
    {
        concurrency::WriterLockToken token(*this);

        return get_next_message(rid, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::messages_empty(concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            return waiting_messages.empty();
        }
        else
        {
            LOG(fatal, "executor", "messages_empty",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return true;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::messages_empty(void)
    {
        concurrency::ReaderLockToken token(*this);

        return messages_empty(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::clear_all_messages(concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            while (not waiting_messages.empty())
            {
                delete waiting_messages.front().second;
                waiting_messages.pop();
            }

            return true;
        }
        else
        {
            LOG(fatal, "executor", "clear_all_messages",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::clear_all_messages(void)
    {
        concurrency::WriterLockToken token(*this);

        return clear_all_messages(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_resource(
        const RID rid,
        ProcessResource *resource_ptr,
        concurrency::WriterLockToken &token)
    {
        if (not rid)
        {
            LOG(fatal, "executor", "add_resource",
                "RID is invalid!  PID "
                + text::to_string(my_pid));
            return false;
        }

        if (not resource_ptr)
        {
            LOG(fatal, "executor", "add_resource",
                "Resource pointer is null!  PID "
                + text::to_string(my_pid)
                + "  RID " + text::to_string(rid));
            return false;
        }

        if (token.has_lock(*this))
        {
            if (resources.find(rid) != resources.end())
            {
                LOG(warning, "executor", "add_resource",
                    "Adding resource that already exists!  PID "
                    + text::to_string(my_pid)
                    + "  RID " + text::to_string(rid));
            }

            resources[rid] = resource_ptr;

            return true;
        }
        else
        {
            LOG(fatal, "executor", "add_resource",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_resource(
        const RID rid,
        ProcessResource *resource_ptr)
    {
        concurrency::WriterLockToken token(*this);

        return add_resource(rid, resource_ptr, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_blocking_resource(
        const RID rid,
        ProcessResource *resource_ptr,
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            if (add_resource(rid, resource_ptr, token))
            {
                // Make sure we're waiting on it.
                //
                blocked_resources.insert(rid);
                default_blocked_resources.insert(rid);

                return true;
            }
        }
        else
        {
            LOG(fatal, "executor", "add_blocking_resource",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::add_blocking_resource(
        const RID rid,
        ProcessResource *resource_ptr)
    {
        concurrency::WriterLockToken token(*this);

        return add_blocking_resource(rid, resource_ptr, token);
    }

    // ----------------------------------------------------------------------
    ProcessResource *ProcessInfo::remove_resource(
        const RID rid,
        concurrency::WriterLockToken &token)
    {
        ProcessResource *resource_ptr = 0;

        if (token.has_lock(*this))
        {
            ResourceMap::iterator resource_iter = resources.find(rid);

            if (resource_iter != resources.end())
            {
                resource_ptr = resource_iter->second;
                resources.erase(resource_iter);
                // Remove from wait list since resource is removed
                blocked_resources.erase(rid);
                default_blocked_resources.erase(rid);
            }
        }
        else
        {
            LOG(fatal, "executor", "remove_resource(RID)",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return resource_ptr;
    }

    // ----------------------------------------------------------------------
    ProcessResource *ProcessInfo::remove_resource(const RID rid)
    {
        concurrency::WriterLockToken token(*this);

        return remove_resource(rid, token);
    }

    // ----------------------------------------------------------------------
    RID ProcessInfo::remove_resource(
        ProcessResource *resource_ptr,
        concurrency::WriterLockToken &token)
    {
        RID rid = 0;

        if (not resource_ptr)
        {
            LOG(fatal, "executor", "remove_resource(resource ptr)",
                "Resource pointer is null!  PID "
                + text::to_string(my_pid));
        }
        else if (token.has_lock(*this))
        {
            ResourceMap::iterator resource_iter = resources.begin();

            while (resource_iter != resources.end())
            {
                if (resource_iter->second == resource_ptr)
                {
                    // Found it.
                    // Remove from wait list since resource is removed
                    blocked_resources.erase(resource_iter->first);

                    rid = resource_iter->first;
                    resources.erase(resource_iter);
                    break;
                }

                ++resource_iter;
            }
        }
        else
        {
            LOG(fatal, "executor", "remove_resource(resource ptr)",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return rid;
    }

    // ----------------------------------------------------------------------
    RID ProcessInfo::remove_resource(ProcessResource *resource_ptr)
    {
        concurrency::WriterLockToken token(*this);

        return remove_resource(resource_ptr, token);
    }

    // ----------------------------------------------------------------------
    ArrayOfRIDs ProcessInfo::get_resource_ids(
        concurrency::ReaderLockToken &token)
    {
        ArrayOfRIDs blocking_rids;

        if (token.has_lock(*this))
        {
            blocking_rids.reserve(resources.size());

            ResourceMap::const_iterator resource_iter = resources.begin();

            while (resource_iter != resources.end())
            {
                blocking_rids.push_back(resource_iter->first);
                ++resource_iter;
            }
        }
        else
        {
            LOG(fatal, "executor", "get_resource_ids",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return blocking_rids;
    }

    // ----------------------------------------------------------------------
    ArrayOfRIDs ProcessInfo::get_resource_ids(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_resource_ids(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::reset_blocked_resources(
        concurrency::WriterLockToken &token)
    {
        if (token.has_lock(*this))
        {
            blocked_resources = default_blocked_resources;
            return true;
        }
        else
        {
            LOG(fatal, "executor", "reset_blocked_resources",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return false;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::reset_blocked_resources(void)
    {
        concurrency::WriterLockToken token(*this);

        return reset_blocked_resources(token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::remove_blocked_resource(
        const RID rid,
        concurrency::WriterLockToken &token)
    {
        bool empty = true;

        if (token.has_lock(*this))
        {
            ResourceSet::iterator resources_iter =
                blocked_resources.find(rid);

            if (resources_iter == blocked_resources.end())
            {
                LOG(warning, "executor", "remove_blocked_resource",
                    "Could not find RID "
                    + text::to_string(rid)
                    + ".  PID "
                    + text::to_string(my_pid));
            }
            else
            {
                blocked_resources.erase(resources_iter);
            }

            empty = blocked_resources.empty();
        }
        else
        {
            LOG(fatal, "executor", "remove_blocked_resource",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return empty;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::remove_blocked_resource(const RID rid)
    {
        concurrency::WriterLockToken token(*this);

        return remove_blocked_resource(rid, token);
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::blocked_resources_empty(
        concurrency::ReaderLockToken &token)
    {
        bool empty = false;

        if (token.has_lock(*this))
        {
            empty = blocked_resources.empty();
        }
        else
        {
            LOG(fatal, "executor", "blocked_resources_empty",
                "Using the wrong lock token!  PID "
                + text::to_string(my_pid));
        }

        return empty;
    }

    // ----------------------------------------------------------------------
    bool ProcessInfo::blocked_resources_empty(void)
    {
        concurrency::ReaderLockToken token(*this);

        return blocked_resources_empty(token);
    }
}
}
