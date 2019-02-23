#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

#include "executor/executor_ProcessScheduler.h"
#include "executor/executor_ProcessServices.h"
#include "executor/executor_ProcessResource.h"
#include "executor/executor_ProcessInfo.h"

namespace mutgos
{
namespace executor
{
    // ----------------------------------------------------------------------
    ProcessServices::ProcessServices(
        ProcessInfo ** const info_ptr,
        ProcessScheduler * const process_scheduler_ptr)
      : process_info_ptr(info_ptr),
        scheduler_ptr(process_scheduler_ptr)
    {
        if (not info_ptr)
        {
            // We will crash shortly.
            LOG(fatal, "executor", "ProcessServices",
                "Process info pointer is null!");
        }

        if (not scheduler_ptr)
        {
            // We will crash shortly.
            LOG(fatal, "executor", "ProcessServices",
                "Scheduler pointer is null!");
        }
    }

    // ----------------------------------------------------------------------
    ProcessServices::~ProcessServices()
    {
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::add_resource(
        ProcessResource * const resource_ptr,
        RID &rid)
    {
        rid = 0;

        const PID pid = (*process_info_ptr)->get_pid();

        if (not resource_ptr)
        {
            LOG(error, "executor", "add_resource",
                "Resource pointer is null!  PID "
                + text::to_string(pid));
        }
        else
        {
            // Get a new RID
            rid = scheduler_ptr->get_next_rid(pid);

            if (not rid)
            {
                LOG(error, "executor", "add_resource",
                    "Unable to get new RID!  PID "
                    + text::to_string(pid));
            }
            else
            {
                // Let resource know...
                if (resource_ptr->resource_added_to_process(pid, rid))
                {
                    // ...and store it in process info
                    if ((*process_info_ptr)->add_resource(
                        rid,
                        resource_ptr))
                    {
                        LOG(debug, "executor", "add_resource",
                            "Added RID "
                            + text::to_string(rid)
                            + " to PID "
                            + text::to_string(pid));
                    }
                    else
                    {
                        // Failed to add.  Release RID, let resource know,  and
                        // error out.
                        //
                        resource_ptr->resource_removed_from_process(
                            pid,
                            rid,
                            false);
                        scheduler_ptr->release_rid(pid, rid);
                        rid = 0;

                        LOG(error, "executor", "add_resource",
                            "Failed to add resource to process info!  PID "
                            + text::to_string(pid));
                    }
                }
                else
                {
                    scheduler_ptr->release_rid(pid, rid);
                    rid = 0;

                    LOG(error, "executor", "add_resource",
                        "Failed to add process to resource!  PID "
                        + text::to_string(pid));
                }
            }
        }

        return rid;
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::add_resource(ProcessResource *const resource_ptr)
    {
        // Throwaway RID
        RID rid = 0;

        return add_resource(resource_ptr, rid);
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::add_blocking_resource(
        ProcessResource * const resource_ptr,
        RID &rid)
    {
        rid = 0;

        const PID pid = (*process_info_ptr)->get_pid();

        if (not resource_ptr)
        {
            LOG(error, "executor", "add_blocking_resource",
                "Resource pointer is null!  PID "
                  + text::to_string(pid));
        }
        else
        {
            // Get a new RID
            rid = scheduler_ptr->get_next_rid(pid);

            if (not rid)
            {
                LOG(error, "executor", "add_blocking_resource",
                    "Unable to get new RID!  PID "
                    + text::to_string(pid));
            }
            else
            {
                // Let resource know...
                if (resource_ptr->resource_added_to_process(pid, rid))
                {
                    // ...and store it in process info
                    if ((*process_info_ptr)->add_blocking_resource(
                        rid,
                        resource_ptr))
                    {
                        LOG(debug, "executor", "add_blocking_resource",
                            "Added RID "
                              + text::to_string(rid)
                              + " to PID "
                              + text::to_string(pid));
                    }
                    else
                    {
                        // Failed to add.  Release RID, let resource know,  and
                        // error out.
                        //
                        resource_ptr->resource_removed_from_process(
                            pid,
                            rid,
                            false);
                        scheduler_ptr->release_rid(pid, rid);
                        rid = 0;

                        LOG(error, "executor", "add_blocking_resource",
                            "Failed to add resource to process info!  PID "
                            + text::to_string(pid));
                    }
                }
                else
                {
                    scheduler_ptr->release_rid(pid, rid);
                    rid = 0;

                    LOG(error, "executor", "add_blocking_resource",
                        "Failed to add process to resource!  PID "
                        + text::to_string(pid));
                }
            }
        }

        return rid;
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::add_blocking_resource(
        ProcessResource *const resource_ptr)
    {
        // Throwaway RID
        RID rid = 0;

        return add_blocking_resource(resource_ptr, rid);
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::remove_resource(ProcessResource * const resource_ptr)
    {
        bool success = false;
        const PID pid = (*process_info_ptr)->get_pid();

        if (not resource_ptr)
        {
            LOG(error, "executor", "remove_resource(resource)",
                "Resource pointer is null!  PID "
                  + text::to_string(pid));
        }
        else
        {
            // First, try and remove it from the process info
            //
            const RID rid = (*process_info_ptr)->remove_resource(resource_ptr);

            if (not rid)
            {
                LOG(error, "executor", "remove_resource(resource)",
                    "Could not locate RID from resource pointer!  PID "
                    + text::to_string(pid));
            }
            else
            {
                // Then call the resource to let it know
                resource_ptr->resource_removed_from_process(pid, rid, false);
                // Finally, release the RID
                scheduler_ptr->release_rid(pid, rid);

                success = true;
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool ProcessServices::remove_resource(const RID rid)
    {
        bool success = false;
        const PID pid = (*process_info_ptr)->get_pid();

        if (not rid)
        {
            LOG(error, "executor", "remove_resource(rid)",
                "RID is invalid!  PID "
                + text::to_string(pid));
        }
        else
        {
            // First, try and remove it from the process info
            //
            ProcessResource * const resource_ptr =
                (*process_info_ptr)->remove_resource(rid);

            if (not resource_ptr)
            {
                LOG(error, "executor", "remove_resource(rid)",
                    "Could not locate resource pointer from RID!  PID "
                    + text::to_string(pid)
                    + ", RID "
                    + text::to_string(rid));
            }
            else
            {
                // Then call the resource to let it know
                resource_ptr->resource_removed_from_process(pid, rid, false);
                // Finally, release the RID
                scheduler_ptr->release_rid(pid, rid);

                success = true;
            }
        }

        return success;
    }
}
}
