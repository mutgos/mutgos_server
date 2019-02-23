/*
 * useragent_ConnectionLifecycleManager.cpp
 */

#include <string>

#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

#include "executor/executor_ProcessServices.h"
#include "executor/executor_Process.h"
#include "executor/executor_ExecutorAccess.h"

#include "events/events_EventAccess.h"
#include "events/events_SubscriptionCallback.h"
#include "events/events_EventMatchedMessage.h"
#include "events/events_ConnectionSubscriptionParams.h"
#include "events/events_ConnectionEvent.h"

#include "useragent_UserAgent.h"
#include "useragent_ConnectionLifecycleManager.h"

namespace
{
    const std::string PROCESS_NAME = "Connection Lifecycle Manager";
}

namespace mutgos
{
namespace useragent
{
    // ----------------------------------------------------------------------
    ConnectionLifecycleManager::ConnectionLifecycleManager(void)
      : my_pid(0)
    {
    }

    // ----------------------------------------------------------------------
    ConnectionLifecycleManager::~ConnectionLifecycleManager()
    {
    }

    // ----------------------------------------------------------------------
    void ConnectionLifecycleManager::process_added(
        const mutgos::executor::PID pid,
        mutgos::executor::ProcessServices &services)
    {
        my_pid = pid;
        subscribe_events();
    }

    // ----------------------------------------------------------------------
    ConnectionLifecycleManager::ProcessStatus
    ConnectionLifecycleManager::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // We are event-driven only, so wait for the next message.
        return PROCESS_STATUS_WAIT_MESSAGE;
    }

    // ----------------------------------------------------------------------
    ConnectionLifecycleManager::ProcessStatus
    ConnectionLifecycleManager::process_execute(
        const executor::PID pid,
        executor::ProcessServices &services,
        executor::ProcessMessage &message)
    {
        ProcessStatus status = PROCESS_STATUS_WAIT_MESSAGE;

        switch (message.message_get_type())
        {
            case executor::ProcessMessage::MESSAGE_EVENT:
            {
                events::EventMatchedMessage const * event_matched_ptr =
                    dynamic_cast<events::EventMatchedMessage *>(&message);

                if (event_matched_ptr)
                {
                    switch (event_matched_ptr->get_event_type())
                    {
                        case events::Event::EVENT_CONNECTION:
                        {
                            process_connection_event(
                                dynamic_cast<events::ConnectionEvent *>(
                                    & event_matched_ptr->get_event()));

                            break;
                        }

                        default:
                        {
                            LOG(error, "useragent", "process_execute(message)",
                                "Unknown event type: " + text::to_string(
                                    event_matched_ptr->get_event_type()));
                        }
                    }
                }

                break;
            }

            case executor::ProcessMessage::MESSAGE_SUBSCRIPTION_DELETED:
            {
                // We should never get this.  If we do, abort.
                //
                LOG(error, "useragent", "process_execute(message)",
                    "Our subscription was deleted!  Shutting down process...");
                status = PROCESS_STATUS_FINISHED;

                break;
            }

            default:
            {
                LOG(error, "useragent", "process_execute(message)",
                    "Unknown message type: "
                    + text::to_string(message.message_get_type()));
            }
        }

        return status;
    }

    // ----------------------------------------------------------------------
    std::string ConnectionLifecycleManager::process_get_name(
        const executor::PID pid)
    {
        return PROCESS_NAME;
    }

    // ----------------------------------------------------------------------
    bool ConnectionLifecycleManager::process_delete_when_finished(
        const executor::PID pid)
    {
        return true;
    }

    // ----------------------------------------------------------------------
    ConnectionLifecycleManager::ErrorMessageText
    ConnectionLifecycleManager::process_get_error_text(const executor::PID pid)
    {
        // Not supported.
        return ErrorMessageText();
    }

    // ----------------------------------------------------------------------
    void ConnectionLifecycleManager::process_killed(
        const executor::PID pid,
        executor::ProcessServices &services)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void ConnectionLifecycleManager::process_finished(const executor::PID pid)
    {
        // Nothing to do.
    }

    // ----------------------------------------------------------------------
    void ConnectionLifecycleManager::subscribe_events(void)
    {
        // Default gives all connections and disconnections everywhere.
        events::ConnectionSubscriptionParams connection_params;
        events::SubscriptionCallback callback(my_pid);

        events::EventAccess::instance()->subscribe(connection_params, callback);
    }

    // ----------------------------------------------------------------------
    void ConnectionLifecycleManager::process_connection_event(
        events::ConnectionEvent * const connect_event_ptr)
    {
        if (connect_event_ptr)
        {
            switch (connect_event_ptr->get_action())
            {
                case events::ConnectionEvent::ACTION_CONNECTED:
                {
                    // New connection; start the user agent.
                    //
                    UserAgent * const user_agent_ptr =
                        new UserAgent(connect_event_ptr->get_entity_id());

                    const executor::PID pid =
                        executor::ExecutorAccess::instance()->add_process(
                            dbtype::Id(),  // Native process
                            connect_event_ptr->get_entity_id(),
                            user_agent_ptr);

                    if (not pid)
                    {
                        LOG(error, "useragent", "process_connection_event",
                            "Unable to create user agent process for " +
                                connect_event_ptr->get_entity_id().to_string(true));
                    }
                    else
                    {
                        if (not executor::ExecutorAccess::instance()->
                               start_process(pid))
                        {
                            LOG(error, "useragent", "process_connection_event",
                                "Unable to start user agent process for " +
                                connect_event_ptr->get_entity_id().to_string(true));
                        }
                    }

                    break;
                }

                case events::ConnectionEvent::ACTION_DISCONNECTED:
                {
                    // User disconnected; kill all their processes.
                    //
                    executor::ExecutorAccess::instance()->cleanup_processes(
                        connect_event_ptr->get_entity_id());

                    break;
                }

                default:
                {
                    LOG(error, "useragent", "process_connection_event",
                        "Unknown event type: " + text::to_string(
                            connect_event_ptr->get_action()));
                }
            }
        }
    }
}
}
