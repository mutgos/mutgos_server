#ifndef MUTGOS_EXECUTOR_PROCESSINFO_H
#define MUTGOS_EXECUTOR_PROCESSINFO_H

#include <string>
#include <queue>
#include <set>
#include <chrono>

#include <boost/thread/recursive_mutex.hpp>

#include "concurrency/concurrency_LockableObject.h"
#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"

#include "executor/executor_CommonTypes.h"

#include "dbtypes/dbtype_Id.h"

// TODO: Will need a way for messages that have come in via RIDs prior to being officially blocked to be placed in a set for use during 'reset' to show what's already received.

namespace mutgos
{
namespace executor
{
    // Forward declarations
    class Process;
    class ProcessMessage;
    class ProcessResource;

    /**
     * Thread safe container class that holds the class instance being executed
     * along with other important details such as execution state, PID, list of
     * resources, etc.
     *
     * As it is a container class, it does only minimal processing.
     *
     * OK ok, maybe it's slightly more than a container class.  Maybe it should
     * be refactored.
     */
    class ProcessInfo : public concurrency::LockableObject
    {
    public:
        /** State of the process as the scheduler sees it. */
        enum ProcessState
        {
            /** Currently being executed. Must be first enum. */
            PROCESS_STATE_EXECUTING,
            /** Can be run, waiting in process run queue */
            PROCESS_STATE_READY,
            /** Process is waiting for a message to come in */
            PROCESS_STATE_WAIT_MESSAGE,
            /** Process is sleeping, but can get messages.  See sleep_time */
            PROCESS_STATE_SLEEPING,
            /** Process is suspended.  Cannot execute, cannot get messages, cannot wake up */
            PROCESS_STATE_SUSPENDED,
            /** Process is blocked waiting on its resources to provide data */
            PROCESS_STATE_BLOCKED,
            /** Process is being examined by the scheduler */
            PROCESS_STATE_SCHEDULING,
            /** Process is being killed.  No new messages can be received */
            PROCESS_STATE_KILLED,
            /** Process has completed execution (killed or not) */
            PROCESS_STATE_COMPLETED,
            /** Initial state for a process; needs to be set to something else */
            PROCESS_STATE_CREATED,
            /** Used for bounds/invalid check only.  Do not use.  */
            PROCESS_STATE_END_INVALID
        };

        /** Absolute wakeup time in monotonic clock time */
        typedef std::chrono::steady_clock::time_point WakeupTimePoint;

        /**
         * Converts the given process state to a string.
         * @param state[in] The state to convert.
         * @return The process state as a string.
         */
        static const std::string &process_state_to_string(
            const ProcessState state);

        /**
         * Creates a process info instance.
         * @param pid[in] The PID of the process.
         * @param process[in] The pointer to the process itself.  While this
         * class does not delete the pointer, it expects the pointer to be
         * valid until destruction.
         * @param db_executable_id[in] An optional ID from something in the
         * database associated with the process, such as the program Entity.
         * May be a default (invalid) ID if a native (C++) process.
         * @param db_owner_id[in] An optional ID from something in the
         * database this process is running for.  It could be the process
         * 'owner' or something else.
         */
        ProcessInfo(
            const PID pid,
            Process * const process,
            const dbtype::Id &db_executable_id,
            const dbtype::Id &db_owner_id);

        /**
         * Destructor.  This will clean up everything but the process
         * pointer, if not already done.
         */
        ~ProcessInfo();

        /**
         * Locks this object for exclusive (read/write) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock(void);

        /**
         * Attempts to lock this object for exclusive (read/write) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock(void);

        /**
         * Attempts to lock this object for shared (read only) access.
         * Does not block.
         * @return True if successfully locked.
         */
        virtual bool try_lock_shared(void);

        /**
         * Locks this object for shared (read only) access.
         * Blocks until lock can be acquired.
         * @return True if successfully locked.
         */
        virtual bool lock_shared(void);

        /**
         * Unlocks this object from an exclusive lock.  Only call if lock()
         * was used!
         * @return True if success.
         */
        virtual bool unlock(void);

        /**
         * Unlocks this object from a shared lock.  Only call if lock_shared()
         * was used!
         * @return True if success.
         */
        virtual bool unlock_shared(void);

        /**
         * Deletes all waiting messages (frees memory), and removes all
         * resources.
         * @param token[in] The lock token.
         * @return True if successfully cleaned.
         */
        bool cleanup(concurrency::WriterLockToken &token);

        /**
         * Deletes all waiting messages (frees memory), and removes all
         * resources.
         * This method will automatically get a lock.
         * @return True if successfully cleaned.
         */
        bool cleanup(void);

        /**
         * This will never block.
         * @return The PID of the process.
         */
        const PID get_pid(void) const
          { return my_pid; }

        /**
         * This will never block.
         * @return The pointer to the process.
         */
        Process *get_process(void) const
          { return process_ptr; }

        /**
         * This will never block.
         * @return The database entity ID associated with the process
         * executable, such as the program Entity.  May be invalid, or only
         * partially filled in (just the site ID, for instance).
         */
        const dbtype::Id &get_db_executable_id(void) const
          { return my_db_executable_id; }

        /**
         * This will never block.
         * @return The database entity ID associated with who the process
         * is running for, such as the process owner.
         * May be invalid, or only partially filled in (just the site ID, for
         * instance).
         */
        const dbtype::Id &get_db_owner_id(void) const
          { return my_db_owner_id; }

        /**
         * @param token[in] The lock token.
         * @return The current process state.
         */
        ProcessState get_process_state(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The current process state.
         */
        ProcessState get_process_state(void);

        /**
         * Sets the process state.
         * @param state[in] The new state.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_process_state(
            const ProcessState state,
            concurrency::WriterLockToken &token);

        /**
         * Sets the process state.
         * This method will automatically get a lock.
         * @param state[in] The new state.
         * @return True if successfully set.
         */
        bool set_process_state(const ProcessState state);

        /**
         * @param token[in] The lock token.
         * @return If a process kill is pending, but not yet 'official'.
         */
        bool get_pending_killed(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return If a process kill is pending, but not yet 'official'.
         */
        bool get_pending_killed(void);

        /**
         * Sets the 'pending kill' flag, true indicating a process kill has been
         * requested but is not yet official.
         * @param killed[in] The pending kill flag value to set.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_pending_killed(
            const bool killed,
            concurrency::WriterLockToken &token);

        /**
         * Sets the 'pending kill' flag, true indicating a process kill has been
         * requested but is not yet official.
         * This method will automatically get a lock.
         * @param killed[in] The pending kill flag value to set.
         * @return True if successfully set.
         */
        bool set_pending_killed(const bool killed);

        /**
         * @param token[in] The lock token.
         * @return If a process suspension is pending, but not yet 'official'.
         */
        bool get_pending_suspended(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return If a process suspension is pending, but not yet 'official'.
         */
        bool get_pending_suspended(void);

        /**
         * Sets the 'pending suspend' flag, true indicating a process suspension
         * has been requested but is not yet official.
         * @param suspended[in] The pending suspension flag value to set.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_pending_suspended(
            const bool suspended,
            concurrency::WriterLockToken &token);

        /**
         * Sets the 'pending suspend' flag, true indicating a process suspension
         * has been requested but is not yet official.
         * This method will automatically get a lock.
         * @param suspended[in] The pending suspension flag value to set.
         * @return True if successfully set.
         */
        bool set_pending_suspended(const bool suspended);

        /**
         * @param token[in] The lock token.
         * @return If a process is a daemon.
         */
        bool get_daemon(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return If a process is a daemon.
         */
        bool get_daemon(void);

        /**
         * Sets the 'daemon' flag, true indicating a process is a daemon and
         * should not normally be killed.
         * @param is_daemon[in] The daemon flag value to set.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_daemon(
            const bool is_daemon,
            concurrency::WriterLockToken &token);

        /**
         * Sets the 'daemon' flag, true indicating a process is a daemon and
         * should not normally be killed.
         * This method will automatically get a lock.
         * @param is_daemon[in] The daemon flag value to set.
         * @return True if successfully set.
         */
        bool set_daemon(const bool is_daemon);

        /**
         * This does not check the current process state, so it will return
         * a wakeup time in the past if the wakeup already happened.
         * @param token[in] The lock token.
         * @return The steady clock time point for a sleeping process to wakeup.
         */
        WakeupTimePoint get_wakeup_time(concurrency::ReaderLockToken &token);

        /**
         * This does not check the current process state, so it will return
         * a wakeup time in the past if the wakeup already happened.
         * This method will automatically get a lock.
         * @return The steady clock time point for a sleeping process to wakeup.
         */
        WakeupTimePoint get_wakeup_time(void);

        /**
         * Sets the absolute sleep time to be now + offset.
         * @param offset_ms[in] The offset, in milliseconds, to sleep.
         * @param token[in] The lock token.
         * @return True if successfully set.
         */
        bool set_sleep_time_offset(
            const osinterface::OsTypes::UnsignedInt offset_ms,
            concurrency::WriterLockToken &token);

        /**
         * Sets the absolute sleep time to be now + offset.
         * This method will automatically get a lock.
         * @param offset_ms[in] The offset, in milliseconds, to sleep.
         * @return True if successfully set.
         */
        bool set_sleep_time_offset(
            const osinterface::OsTypes::UnsignedInt offset_ms);

        /**
         * Adds a message to the back of the queue for the Process to receive
         * at some point in the future.
         * @param message_ptr[in] The message to add.  Control of the pointer
         * will pass to this class if success.
         * @param token[in] The lock token.
         * @return True if successfully added.
         */
        bool add_message(
            ProcessMessage *message_ptr,
            concurrency::WriterLockToken &token);

        /**
         * Adds a message to the back of the queue for the Process to receive
         * at some point in the future.
         * This method will automatically get a lock.
         * @param message_ptr[in] The message to add.  Control of the pointer
         * will pass to this class if success.
         * @return True if successfully added.
         */
        bool add_message(ProcessMessage *message_ptr);

        /**
         * Adds a message to the back of the queue for the Process to receive
         * at some point in the future.
         * The RID will be removed from the list of waiting RIDs.
         * @param message_ptr[in] The message to add.  Control of the pointer
         * will pass to this class.
         * @param rid[in] The ID of the resource that sent the message.
         * @param token[in] The lock token.
         * @return True if successfully added.
         */
        bool add_message(
            ProcessMessage *message_ptr,
            const RID rid,
            concurrency::WriterLockToken &token);

        /**
         * Adds a message to the back of the queue for the Process to receive
         * at some point in the future.
         * The RID will be removed from the list of waiting RIDs.
         * This method will automatically get a lock.
         * @param message_ptr[in] The message to add.  Control of the pointer
         * will pass to this class.
         * @param rid[in] The ID of the resource that sent the message.
         * @return True if successfully added.
         */
        bool add_message(ProcessMessage *message_ptr, const RID rid);

        /**
         * @param token[in] The lock token.
         * @return The next message in the queue, or null if none waiting or
         * error (bad token, etc).
         * Control of the pointer will pass to the caller, who must delete
         * it when done.
         */
        ProcessMessage *get_next_message(concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The next message in the queue, or null if none waiting or
         * error (bad token, etc).
         * Control of the pointer will pass to the caller, who must delete
         * it when done.
         */
        ProcessMessage *get_next_message(void);

        /**
         * @param rid[out] The RID of the resource that sent the message.
         * @param token[in] The lock token.
         * @return The next message in the queue, or null if none waiting or
         * error (bad token, etc).
         * Control of the pointer will pass to the caller, who must delete
         * it when done.
         */
        ProcessMessage *get_next_message(
            RID &rid,
            concurrency::WriterLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param rid[out] The RID of the resource that sent the message.
         * @return The next message in the queue, or null if none waiting or
         * error (bad token, etc).
         * Control of the pointer will pass to the caller, who must delete
         * it when done.
         */
        ProcessMessage *get_next_message(RID &rid);

        /**
         * @param token[in] The lock token.
         * @return True if no messages are waiting for the process.
         */
        bool messages_empty(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return True if no messages are waiting for the process.
         */
        bool messages_empty(void);

        /**
         * Removes all messages waiting in the queue and frees the
         * associated memory.
         * @param token[in] The lock token.
         * @return True if successfully cleared.
         */
        bool clear_all_messages(concurrency::WriterLockToken &token);

        /**
         * Removes all messages waiting in the queue and frees the
         * associated memory.
         * This method will automatically get a lock.
         * @return True if successfully cleared.
         */
        bool clear_all_messages(void);

        /**
         * Adds the given resource to the resource list.  The resources is not
         * blocking, so this just indicates the Process is using the resource
         * in some way.
         * @param rid[in] The resource ID of the resource to add.
         * @param resource_ptr[in] The pointer to the resource.  While control
         * of the pointer does not change to this class, the pointer must not
         * be deleted until after it is removed.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool add_resource(
            const RID rid,
            ProcessResource *resource_ptr,
            concurrency::WriterLockToken &token);

        /**
         * Adds the given resource to the resource list.  The resources is not
         * blocking, so this just indicates the Process is using the resource
         * in some way.
         * This method will automatically get a lock.
         * @param rid[in] The resource ID of the resource to add.
         * @param resource_ptr[in] The pointer to the resource.  While control
         * of the pointer does not change to this class, the pointer must not
         * be deleted until after it is removed.
         * @return True if success.
         */
        bool add_resource(
            const RID rid,
            ProcessResource *resource_ptr);

        /**
         * Adds the given resource to the blocking resource list.
         * @param rid[in] The resource ID of the resource to add.
         * @param resource_ptr[in] The pointer to the resource.  While control
         * of the pointer does not change to this class, the pointer must not
         * be deleted until after it is removed.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool add_blocking_resource(
            const RID rid,
            ProcessResource *resource_ptr,
            concurrency::WriterLockToken &token);

        /**
         * Adds the given resource to the blocking resource list.
         * This method will automatically get a lock.
         * @param rid[in] The resource ID of the resource to add.
         * @param resource_ptr[in] The pointer to the resource.  While control
         * of the pointer does not change to this class, the pointer must not
         * be deleted until after it is removed.
         * @return True if success.
         */
        bool add_blocking_resource(
            const RID rid,
            ProcessResource *resource_ptr);

        /**
         * Removes the given resource from any resource list (by RID).
         * The resource pointer is not freed.
         * @param rid[in] The resource ID of the resource to remove.
         * @param token[in] The lock token.
         * @return The pointer to the resource removed, or null if none
         * found or error.
         */
        ProcessResource *remove_resource(
            const RID rid,
            concurrency::WriterLockToken &token);

        /**
         * Removes the given resource from any resource list (by RID).
         * The resource pointer is not freed.
         * This method will automatically get a lock.
         * @param rid[in] The resource ID of the resource to remove.
         * @return The pointer to the resource removed, or null if none
         * found or error.
         */
        ProcessResource *remove_resource(const RID rid);

        /**
         * Removes the given resource from any resource list (by pointer).
         * The resource pointer is not freed.
         * Note this is a less efficient call compared to removing the
         * resource by RID.
         * @param resource_ptr[in] The pointer of the resource to remove.
         * @param token[in] The lock token.
         * @return The RID of the resource removed, or invalid if none
         * found or error.
         */
        RID remove_resource(
            ProcessResource *resource_ptr,
            concurrency::WriterLockToken &token);

        /**
         * Removes the given resource from any resource list (by pointer).
         * The resource pointer is not freed.
         * Note this is a less efficient call compared to removing the
         * resource by RID.
         * This method will automatically get a lock.
         * @param resource_ptr[in] The pointer of the resource to remove.
         * @return The RID of the resource removed, or invalid if none
         * found or error.
         */
        RID remove_resource(ProcessResource *resource_ptr);

        /**
         * @param token[in] The lock token.
         * @return Array of resource IDs added to this class or empty
         * if none or error.
         */
        ArrayOfRIDs get_resource_ids(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return Array of resource IDs added to this class or empty
         * if none or error.
         */
        ArrayOfRIDs get_resource_ids(void);

        /**
         * Re-populates the set of blocking resources being waited on to
         * all blocking resources.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool reset_blocked_resources(concurrency::WriterLockToken &token);

        /**
         * Re-populates the set of blocking resources being waited on to
         * all blocking resources.
         * This method will automatically get a lock.
         * @return True if success.
         */
        bool reset_blocked_resources(void);

        /**
         * Removes a blocking resource from the current set of resources
         * being waiting on.
         * @param rid[in] The resource ID to remove.
         * @param token[in] The lock token.
         * @return True if no more resources are blocking.
         */
        bool remove_blocked_resource(
            const RID rid,
            concurrency::WriterLockToken &token);

        /**
         * Removes a blocking resource from the current set of resources
         * being waiting on.
         * This method will automatically get a lock.
         * @param rid[in] The resource ID to remove.
         * @return True if no more resources are blocking.
         */
        bool remove_blocked_resource(const RID rid);

        /**
         * @param token[in] The lock token.
         * @return True if there are no blocking resources being waited on.
         */
        bool blocked_resources_empty(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return True if there are no blocking resources being waited on.
         */
        bool blocked_resources_empty(void);

    private:

        /** First is the RID that sent the message (or 0), second is the message
            pointer */
        typedef std::pair<RID, ProcessMessage *> MessageQueueEntry;
        /** Queue for waiting messages */
        typedef std::queue<MessageQueueEntry> MessageQueue;
        /** Map for RID -> resource */
        typedef std::map<RID, ProcessResource *> ResourceMap;
        /** Set of resource IDs */
        typedef std::set<RID> ResourceSet;

        boost::recursive_mutex info_lock; ///< The lock for the ProcessInfo.

        const PID my_pid; ///< PID of this process
        Process * const process_ptr; ///< Pointer to actual process instance
        const dbtype::Id my_db_executable_id; ///< Optional related ID for executable data
        const dbtype::Id my_db_owner_id; ///< Optional related ID this is running for

        ProcessState process_state; ///< Real time state of process
        bool pending_killed; ///< True if process kill has been requested
        bool pending_suspended; ///< True if process suspension has been requested
        bool daemon; ///< True if a process is a daemon (not cleaned up) TODO remove

        WakeupTimePoint wakeup_time; ///< If sleeping, when wakeup occurs

        MessageQueue waiting_messages; ///< ProcessMessages sent to process

        ResourceMap resources; ///< Resources the process is using
        ResourceSet default_blocked_resources; ///< When blocked_resources is reset, this is the template
        ResourceSet blocked_resources; ///< RIDs of resources currently blocked
    };
}
}

#endif //MUTGOS_EXECUTOR_PROCESSINFO_H
