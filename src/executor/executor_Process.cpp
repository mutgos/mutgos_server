#include "executor/executor_Process.h"

namespace mutgos
{
namespace executor
{
    // ----------------------------------------------------------------------
    Process::Process(void)
    {
    }

    // ----------------------------------------------------------------------
    Process::~Process()
    {
    }

    // ----------------------------------------------------------------------
    void Process::process_added(const PID pid, ProcessServices &services)
    {
    }

    // ----------------------------------------------------------------------
    Process::ProcessStatus Process::process_execute(
        const PID pid,
        ProcessServices &execution,
        ProcessMessage &message)
    {
        return PROCESS_STATUS_ERROR;
    }

    // ----------------------------------------------------------------------
    Process::ProcessStatus Process::process_execute(
        const PID pid,
        ProcessServices &execution,
        const RID rid,
        ProcessMessage &message)
    {
        return PROCESS_STATUS_ERROR;
    }

    // ----------------------------------------------------------------------
    Process::ProcessStatus Process::process_execute(
        const PID pid,
        ProcessServices &execution,
        const RID rid)
    {
        return PROCESS_STATUS_ERROR;
    }

    // ----------------------------------------------------------------------
    osinterface::OsTypes::UnsignedInt Process::process_get_sleep_time(
        const PID pid)
    {
        return 0;
    }

    // ----------------------------------------------------------------------
    Process::ErrorMessageText Process::process_get_error_text(const PID pid)
    {
        ErrorMessageText errors;
        errors.push_back("Not implemented.");

        return errors;
    }

    // ----------------------------------------------------------------------
    void Process::process_killed(
        const PID pid,
        ProcessServices &services)
    {
    }

    // ----------------------------------------------------------------------
    void Process::process_finished(const PID pid)
    {
    }
}
}
