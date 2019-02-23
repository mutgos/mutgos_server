#ifndef MUTGOS_EXECUTOR_RESOURCE_H
#define MUTGOS_EXECUTOR_RESOURCE_H

#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace executor
{
    /**
     * Interface class that represents a 'resource' that a Process is using
     * in some way.  Resources that wish to be automatically managed by the
     * executor and support being waited on (highly recommended) need to
     * implement this.
     */
    class ProcessResource
    {
    public:
        /**
         * Virtual interface destructor.
         */
        virtual ~ProcessResource();

        /**
         * Called when process has added this resource to its blocking
         * list.  It must be thread safe.
         * When the resource has been added, the process will keep a pointer
         * to this instance.  Therefore, do not delete the pointer until the
         * resource has been removed.
         * @param process_id[in] The process ID that has added this resource.
         * Each process can add this resource only once.
         * @param resource_id[in] The unique identifier for this resource as
         * assigned to the given process.  If a different process adds this
         * resource, another unique resource_id will be provided.
         * @return True if this resource could add the process.  If false
         * this means the resource could not add the process (perhaps it only
         * supports one process at a time).  The Executor will return
         * appropriate status to the Process based on what's returned here.
         */
        virtual bool resource_added_to_process(
            const PID process_id,
            const RID resource_id) =0;

        /**
         * Called when a process has removed this resource from its blocking
         * list, because either the process has ended (automatic cleanup) or
         * the process manually removed it.  If the resource was not
         * successfully added, this will not get called for that resource ID.
         * It must be thread safe.
         * After the resource has been removed, it is safe to assume the
         * Process no longer has a pointer to this instance (safe to delete).
         * @param process_id[in] The process ID that has removed this resource.
         * @param resource_id[in] The resource ID (previously added) that has
         * been removed.
         * @param process_cleanup[in] If true, removal was due to automatic
         * cleanup due to the process ending or being killed.
         */
        virtual void resource_removed_from_process(
            const PID process_id,
            const RID resource_id,
            const bool process_cleanup) =0;

    protected:
        /**
         * Interface constructor.
         */
        ProcessResource(void);
    };
}
}
#endif //MUTGOS_EXECUTOR_RESOURCE_H
