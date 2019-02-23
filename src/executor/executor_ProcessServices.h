#ifndef MUTGOS_EXECUTOR_PROCESSSERVICES_H
#define MUTGOS_EXECUTOR_PROCESSSERVICES_H

#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace executor
{
    // Forward declarations
    class ProcessResource;
    class ProcessScheduler;
    class ProcessInfo;

    /**
     * Passed to a currently executing process, this provides a way for
     * a running process to communicate with the Executor.
     * A process can assume the instance provided already knows who they are.
     * The pointer must not be shared nor kept.
     */
    class ProcessServices
    {
    public:
        /**
         * Used by the executor module only, this creates an instance of
         * the services.
         * @param info_ptr[in] A pointer to the currently active process's info
         * this class instance uses when processing requests.  It is the
         * info for the process that will be making requests.  It is expected
         * the info will change after construction (hence pointer to a pointer,
         * which is updated by whoever constructed this class before executing a
         * process).
         * @param process_scheduler_ptr[in] Pointer to the process scheduler.
         */
        ProcessServices(
            ProcessInfo ** const info_ptr,
            ProcessScheduler * const process_scheduler_ptr);

        /**
         * Destructor.
         */
        ~ProcessServices();

        /**
         * Adds a resource to the resource list, used when the process wishes
         * to use it but not block.
         * @param resource_ptr[in] Pointer to the resource.  Pointer control
         * is not transferred, but it is held onto if returns successful.
         * @param rid[out] If add is successful, this will be populated with
         * the RID for the newly added resource.
         * @return True if success (rid populated), false if error (
         * resource errored out, etc).
         */
        bool add_resource(
            ProcessResource * const resource_ptr,
            RID &rid);

        /**
         * Adds a resource to the resource list, used when the process wishes
         * to use it but not block.  No RID will be provided;
         * some processes may not have a use for it.
         * @param resource_ptr[in] Pointer to the resource.  Pointer control
         * is not transferred, but it is held onto if returns successful.
         * @return True if success, false if error (duplicate pointer, resource
         * errored out, etc).
         */
        bool add_resource(ProcessResource * const resource_ptr);

        /**
         * Adds a blocking resource to the blocking resource list, used
         * when the process wishes to block on them.
         * @param resource_ptr[in] Pointer to the resource.  Pointer control
         * is not transferred, but it is held onto if returns successful.
         * @param rid[out] If add is successful, this will be populated with
         * the RID for the newly added resource.
         * @return True if success (rid populated), false if error (
         * resource errored out, etc).
         */
        bool add_blocking_resource(
            ProcessResource * const resource_ptr,
            RID &rid);

        /**
         * Adds a blocking resource to the blocking resource list, used
         * when the process wishes to block on them.  No RID will be provided;
         * some processes may not have a use for it.
         * @param resource_ptr[in] Pointer to the resource.  Pointer control
         * is not transferred, but it is held onto if returns successful.
         * @return True if success, false if error (duplicate pointer, resource
         * errored out, etc).
         */
        bool add_blocking_resource(ProcessResource * const resource_ptr);

        /**
         * Removes a resource previously added, by pointer.  If the resource
         * is not found, nothing happens.
         * @param resource_ptr[in] The resource to remove.
         * @returns True if found.
         */
        bool remove_resource(ProcessResource * const resource_ptr);

        /**
         * Removes a resource previously added, by RID.  If the resource
         * is not found, nothing happens.  The pointer to the resource will no
         * longer be held onto.
         * @param rid[in] The resource to remove.
         * @returns True if found.
         */
        bool remove_resource(const RID rid);

    private:
        ProcessInfo  ** const process_info_ptr; ///< Pointer to active process
        ProcessScheduler * const scheduler_ptr; ///< Pointer to scheduler
    };
}
}

#endif //MUTGOS_EXECUTOR_PROCESSSERVICES_H
