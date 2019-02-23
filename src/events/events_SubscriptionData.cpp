/*
 * events_SubscriptionData.cpp
 */

#include <string>
#include <string.h>
#include <limits>

#include "osinterface/osinterface_OsTypes.h"

#include <boost/thread/shared_mutex.hpp>
#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

#include "events/events_SubscriptionData.h"
#include "events/events_SubscriptionProcessor.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    SubscriptionData::SubscriptionData(void)
      : empty_subscription(0, SubscriptionCallback()),
        next_unique_subscription_id(1),
        max_subscription_id(std::numeric_limits<SubscriptionId>::max() - 1)
    {
        memset(subscription_processors, 0, sizeof(subscription_processors));
    }

    // ----------------------------------------------------------------------
    SubscriptionData::~SubscriptionData()
    {
        // Cleanup pointers.
        //
        for (MG_UnsignedInt index = 0; index < Event::EVENT_END_INVALID; ++index)
        {
            if (subscription_processors[index])
            {
                delete subscription_processors[index];
                subscription_processors[index] = 0;
            }
        }

        for (SubscriptionIdToData::iterator iter = subscription_data.begin();
            iter != subscription_data.end();
            ++iter)
        {
            delete iter->second.params_ptr;
            delete iter->second.callback_ptr;
        }

        subscription_data.clear();
        next_unique_subscription_id = 0;
    }

    // ----------------------------------------------------------------------
    SubscriptionId SubscriptionData::add_subscription(
        SubscriptionParams * const subscription_ptr,
        const Event::EventType subscription_type,
        SubscriptionCallback * const callback_ptr)
    {
        SubscriptionId id = 0;

        if (not subscription_ptr)
        {
            LOG(error, "events", "add_subscription",
                "subscription_ptr is null!");
        }
        else if (not callback_ptr)
        {
            LOG(error, "events", "add_subscription",
                "callback_ptr is null!");
        }
        else if (not callback_ptr->valid())
        {
            LOG(error, "events", "add_subscription",
                "callback not valid!");

            delete subscription_ptr;
            delete callback_ptr;
        }
        else
        {
            // Everything looks valid, put it into the data structures.
            //
            boost::unique_lock<boost::shared_mutex> write_lock(
                subscription_lock);

            id = get_next_subscription_id();

            if (not id)
            {
                LOG(error, "events", "add_subscription",
                    "Unable to get unique ID!");

                delete subscription_ptr;
                delete callback_ptr;
            }
            else
            {
                const executor::PID pid = callback_ptr->get_pid();

                subscription_data[id] = SubscriptionDetails(
                    pid,
                    subscription_type,
                    subscription_ptr,
                    callback_ptr);

                if (pid)
                {
                    // Callback is to a PID, so add to PID data structures
                    //
                    SubscriptionIdList &subscriptions = pid_subscriptions[pid];
                    subscriptions.push_back(id);
                }
            }
        }

        return id;
    }

    // ----------------------------------------------------------------------
    bool SubscriptionData::remove_subscription(const SubscriptionId id)
    {
        bool success = true;

        if (not id)
        {
            LOG(error, "events", "remove_subscription",
                "subscription is 0!");
            success = false;
        }
        else
        {
            boost::unique_lock<boost::shared_mutex> write_lock(
                subscription_lock);

            SubscriptionIdToData::iterator data_iter =
                subscription_data.find(id);

            if (data_iter == subscription_data.end())
            {
                LOG(error, "events", "remove_subscription",
                    "subscription not found: "
                    + text::to_string(id));
                success = false;
            }
            else
            {
                if (data_iter->second.pid)
                {
                    // Also found it is from a Process.  Remove it there
                    // too.
                    PidToSubscriptions::iterator pid_iter =
                        pid_subscriptions.find(data_iter->second.pid);

                    if (pid_iter != pid_subscriptions.end())
                    {
                        for (SubscriptionIdList::iterator list_iter =
                                pid_iter->second.begin();
                            list_iter != pid_iter->second.end();
                            ++list_iter)
                        {
                            if (*list_iter == id)
                            {
                                // Found it.  Erase and exit since iter is
                                // now invalid.
                                pid_iter->second.erase(list_iter);
                                break;
                            }
                        }

                        if (pid_iter->second.empty())
                        {
                            // No more subscrpitions for PID.  Remove.
                            pid_subscriptions.erase(pid_iter);
                        }
                    }
                }

                // Clean up pointers and remove from data map.
                //
                delete data_iter->second.params_ptr;
                delete data_iter->second.callback_ptr;
                subscription_data.erase(data_iter);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool SubscriptionData::subscription_exists(const SubscriptionId id)
    {
        boost::shared_lock<boost::shared_mutex> read_lock(
            subscription_lock);

        const bool exists =
            (subscription_data.find(id) != subscription_data.end());

        return exists;
    }

    // ----------------------------------------------------------------------
    SubscriptionIdList SubscriptionData::
        get_subscriptions_for_process(const executor::PID pid)
    {
        if (not pid)
        {
            LOG(error, "events", "get_subscriptions_for_process",
                "PID is 0!");
            return empty_id_list;
        }
        else
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            PidToSubscriptions::const_iterator pid_iter =
                pid_subscriptions.find(pid);

            if (pid_iter == pid_subscriptions.end())
            {
                // Not found.  Return empty.
                return empty_id_list;
            }
            else
            {
                // Found it!
                return pid_iter->second;
            }
        }

        return empty_id_list;
    }

    // ----------------------------------------------------------------------
    SubscriptionData::SubscriptionParamCallback SubscriptionData::
        get_subscription_info(const SubscriptionId id)
    {
        if (not id)
        {
            LOG(error, "events", "get_subscription_info",
                "subscription id is 0!");
            return empty_subscription;
        }
        else
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            SubscriptionIdToData::const_iterator data_iter =
                subscription_data.find(id);

            if (data_iter == subscription_data.end())
            {
                // Not found.  Return invalid.
                return empty_subscription;
            }
            else
            {
                return std::make_pair(
                    data_iter->second.params_ptr,
                    *(data_iter->second.callback_ptr));
            }
        }

        return empty_subscription;
    }

    // ----------------------------------------------------------------------
    Event::EventType SubscriptionData::get_subscription_type(
        const SubscriptionId id)
    {
        if (not id)
        {
            LOG(error, "events", "get_subscription_type",
                "subscription id is 0!");
            return Event::EVENT_END_INVALID;
        }
        else
        {
            boost::shared_lock<boost::shared_mutex> read_lock(
                subscription_lock);

            SubscriptionIdToData::const_iterator data_iter =
                subscription_data.find(id);

            if (data_iter == subscription_data.end())
            {
                // Not found.  Return invalid.
                return Event::EVENT_END_INVALID;
            }
            else
            {
                return data_iter->second.event_type;
            }
        }

        return Event::EVENT_END_INVALID;
    }

    // ----------------------------------------------------------------------
    void SubscriptionData::register_subscription_processor(
        SubscriptionProcessor *const processor_ptr)
    {
        if (not processor_ptr)
        {
            LOG(error, "events", "register_subscription_processor",
                "Null pointer for event processor.  Ignored.");
        }
        else
        {
            const Event::EventType event_type =
                processor_ptr->get_event_type_handled();

            if (subscription_processors[event_type])
            {
                // Overwrite existing - should never happen.
                delete subscription_processors[event_type];

                LOG(warning, "events", "register_subscription_processor",
                    "Overwriting subscription processor of type " +
                        text::to_string(event_type));
            }

            subscription_processors[event_type] = processor_ptr;
        }
    }

    // ----------------------------------------------------------------------
    SubscriptionId SubscriptionData::get_next_subscription_id(void)
    {
        SubscriptionId id = next_unique_subscription_id;

        if (subscription_data.find(id) == subscription_data.end())
        {
            // Found next ID.
            ++next_unique_subscription_id;

            if (next_unique_subscription_id >= max_subscription_id)
            {
                next_unique_subscription_id = 1;
            }
        }
        else if (subscription_data.size() >= max_subscription_id)
        {
            // Next ID already in use and there's no more available.
            //
            LOG(error, "events", "get_next_subscription_id",
                "No more IDs available!");
        }
        else
        {
            // Next ID already in use.  Find the next available...
            //
            id = 0;

            while (not id)
            {
                if (subscription_data.find(next_unique_subscription_id) ==
                    subscription_data.end())
                {
                    id = next_unique_subscription_id;
                }

                ++next_unique_subscription_id;

                if (next_unique_subscription_id >= max_subscription_id)
                {
                    next_unique_subscription_id = 1;
                }
            }
        }

        return id;
    }
}
}
