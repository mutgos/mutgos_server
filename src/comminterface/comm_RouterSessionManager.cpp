/*
 * comm_RouterSessionManager.cpp
 */

#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "osinterface/osinterface_TimeUtils.h"
#include "osinterface/osinterface_ThreadUtils.h"

#include "logging/log_Logger.h"
#include "utilities/mutgos_config.h"
#include "text/text_ExternalText.h"

#include "comminterface/comm_RouterSessionManager.h"
#include "comminterface/comm_ConnectionDriver.h"
#include "comminterface/comm_SessionStats.h"

#include "dbinterface/dbinterface_DatabaseAccess.h"
#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Player.h"

#include "events/events_ConnectionEvent.h"
#include "events/events_EventAccess.h"
#include "channels/events_Channel.h"

// Defines
//
#define DEFAULT_SLEEP_TIME_NANOSEC  100000000
#define DEFAULT_IDLE_CHECK_SEC 60

namespace mutgos
{
namespace comm
{
    // ----------------------------------------------------------------------
    RouterSessionManager::RouterSessionManager(void)
        : thread_ptr(0),
          shutdown_thread_flag(false)
    {
    }

    // ----------------------------------------------------------------------
    RouterSessionManager::~RouterSessionManager()
    {
        shutdown();

        for (ConnectionDrivers::iterator driver_iter =
            connection_drivers.begin();
             driver_iter != connection_drivers.end();
             ++driver_iter)
        {
            delete *driver_iter;
        }

        connection_drivers.clear();
    }

    // ----------------------------------------------------------------------
    bool RouterSessionManager::startup(void)
    {
        if (not thread_ptr)
        {
            LOG(info, "comm", "startup", "Starting up...");

            for (ConnectionDrivers::iterator driver_iter =
                connection_drivers.begin();
                 driver_iter != connection_drivers.end();
                 ++driver_iter)
            {
                (*driver_iter)->start();
            }

            shutdown_thread_flag.store(false);
            thread_ptr = new boost::thread(boost::ref(*this));
        }

        return true;
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::shutdown(void)
    {
        if (thread_ptr)
        {
            LOG(info, "comm", "shutdown", "Shutting down...");

            shutdown_thread_flag.store(true);

            thread_ptr->join();
            delete thread_ptr;
            thread_ptr = 0;

            // Now that the thread is gone, cleanup everything.
            //
            for (ConnectionDrivers::iterator driver_iter =
                connection_drivers.begin();
                 driver_iter != connection_drivers.end();
                 ++driver_iter)
            {
                (*driver_iter)->stop(this);
            }
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::operator()()
    {
        thread_main();
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::add_connection_driver(ConnectionDriver *driver_ptr)
    {
        if (driver_ptr)
        {
            if (std::find(
                connection_drivers.begin(),
                connection_drivers.end(),
                driver_ptr) == connection_drivers.end())
            {
                connection_drivers.push_back(driver_ptr);
            }
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector RouterSessionManager::get_entity_site_ids()
    {
        dbtype::Id::SiteIdVector result;
        boost::lock_guard<boost::recursive_mutex> read_lock(router_lock);

        for (SiteSessionsMap::const_iterator iter = site_to_sessions.begin();
             iter != site_to_sessions.end();
             ++iter)
        {
            result.push_back(iter->first);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    RouterSessionManager::SessionStatsVector RouterSessionManager::get_session_stats(
        const dbtype::Id::SiteIdType site_id)
    {
        SessionStatsVector result;
        boost::lock_guard<boost::recursive_mutex> read_lock(router_lock);

        SiteSessionsMap::iterator site_iter = site_to_sessions.find(site_id);

        if (site_iter != site_to_sessions.end())
        {
            for (EntitySessionMap::iterator entity_iter =
                    site_iter->second.begin();
                entity_iter != site_iter->second.end();
                ++entity_iter)
            {
                result.push_back(entity_iter->second->get_stats());
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector RouterSessionManager::get_online_ids(
        const dbtype::Id::SiteIdType site_id)
    {
        dbtype::Entity::IdVector result;
        boost::lock_guard<boost::recursive_mutex> read_lock(router_lock);

        SiteSessionsMap::iterator site_iter = site_to_sessions.find(site_id);

        if (site_iter != site_to_sessions.end())
        {
            result.reserve(site_iter->second.size());

            for (EntitySessionMap::iterator entity_iter =
                site_iter->second.begin();
                 entity_iter != site_iter->second.end();
                 ++entity_iter)
            {
                result.push_back(entity_iter->first);
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt RouterSessionManager::get_session_online_count(
        const mutgos::dbtype::Id::SiteIdType site_id)
    {
        MG_UnsignedInt result = 0;
        boost::lock_guard<boost::recursive_mutex> read_lock(router_lock);

        SiteSessionsMap::iterator site_iter = site_to_sessions.find(site_id);

        if (site_iter != site_to_sessions.end())
        {
            result = (MG_UnsignedInt) site_iter->second.size();
        }

        return result;
    }

    // ----------------------------------------------------------------------
    SessionStats RouterSessionManager::get_session_stats(
        const dbtype::Id &entity_id)
    {
        boost::lock_guard<boost::recursive_mutex> read_lock(router_lock);

        ClientSession * const session_ptr = get_entity_session(entity_id);

        if (session_ptr)
        {
            return session_ptr->get_stats();
        }
        else
        {
            return SessionStats();
        }
    }

    // ----------------------------------------------------------------------
    bool RouterSessionManager::add_channel(
        const dbtype::Id &id,
        events::Channel *channel_ptr,
        const bool to_client)
    {
        bool success = false;

        if (not channel_ptr)
        {
            LOG(error, "comm", "add_channel", "channel_ptr is null!");
        }
        else
        {
            boost::lock_guard<boost::recursive_mutex> callback_mutex(
                callback_lock);
            boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
            ClientSession * const session_ptr = get_entity_session(id);

            if (session_ptr)
            {
                // A nonzero channel ID indicates success.
                success = session_ptr->channel_added(channel_ptr, to_client);
            }
        }

        return success;
    }

    // ----------------------------------------------------------------------
    bool RouterSessionManager::disconnect_session(const dbtype::Id &entity_id)
    {
        ClientSession *session_ptr = 0;

        {
            boost::lock_guard<boost::recursive_mutex> read_lock(
                router_lock);

            session_ptr = get_entity_session(entity_id);
        }

        if (session_ptr)
        {
            remove_session(session_ptr);
        }

        return session_ptr;
    }

    // ----------------------------------------------------------------------
    bool RouterSessionManager::add_session(
        ConnectionDriver *connection_driver_ptr,
        ClientConnection *connection_ptr,
        ClientSession *session_ptr,
        const dbtype::Id &id)
    {
        bool updated = false;

        boost::lock_guard<boost::recursive_mutex> callback_mutex(callback_lock);
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);

        if (not connection_ptr)
        {
            LOG(error, "comm", "add_session", "connection is null!");
        }
        else if (not session_ptr)
        {
            LOG(error, "comm", "add_session", "session is null!");
        }
        else if (id.is_default())
        {
            LOG(error, "comm", "add_session", "Entity ID is invalid!");
        }
        else
        {
            // Update connection time.
            //
            dbinterface::EntityRef entity =
                dbinterface::DatabaseAccess::instance()->get_entity(id);

            if (entity.valid() and dynamic_cast<dbtype::Player *>(entity.get()))
            {
                static_cast<dbtype::Player *>(entity.get())->
                    set_last_connect_to_now();
            }
            else
            {
                LOG(error, "comm", "add_session",
                    "Player " + id.to_string(true) + " does not exist or "
                    "wrong type!  Cannot update last connect time.");
            }

            SessionConnectionMap::iterator session_iter =
                session_to_connection.find(session_ptr);

            if (session_iter == session_to_connection.end())
            {
                // New session
                //
                session_to_connection[session_ptr] = connection_ptr;
                connection_to_session[connection_ptr] =
                    std::make_pair(connection_driver_ptr, session_ptr);
                add_entity_session(id, session_ptr);

                connection_ptr->client_set_entity_id(id);

                events::EventAccess::instance()->publish_event(
                    new events::ConnectionEvent(
                        events::ConnectionEvent::ACTION_CONNECTED,
                        id,
                        connection_ptr->client_get_source()));
            }
            else
            {
                // Update session
                //
                ClientConnection * const old_connection_ptr =
                    session_iter->second;

                session_iter->second = connection_ptr;
                connection_to_session.erase(old_connection_ptr);
                connection_to_session[connection_ptr] =
                    std::make_pair(connection_driver_ptr, session_ptr);
                connection_ptr->client_set_entity_id(id);

                updated = true;
            }
        }

        return updated;
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::remove_session(ClientSession *session_ptr)
    {
        if (session_ptr)
        {
            boost::lock_guard<boost::recursive_mutex> callback_mutex(
                callback_lock);
            boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);

            SessionConnectionMap::iterator session_iter =
                session_to_connection.find(session_ptr);

            if (session_iter != session_to_connection.end())
            {
                events::EventAccess::instance()->publish_event(
                    new events::ConnectionEvent(
                        events::ConnectionEvent::ACTION_DISCONNECTED,
                        session_iter->second->client_get_entity_id(),
                        session_iter->second->client_get_source()));

                remove_entity_session(
                    session_iter->second->client_get_entity_id());

                // Found session.  Just destruct it to initiate cleanup.
                delete session_ptr;

                connection_to_session.erase(session_iter->second);
                session_to_connection.erase(session_iter);
            }
        }
    }

    // ----------------------------------------------------------------------
    ClientSession *RouterSessionManager::authorize_client(
        const std::string &name,
        const std::string &password,
        ConnectionDriver *connection_driver_ptr,
        ClientConnection *connection_ptr)
    {
        ClientSession *new_session_ptr = 0;
        dbtype::Id entity_id;

        if (connection_ptr and connection_driver_ptr)
        {
            entity_id = check_password(
                connection_ptr->client_get_site_id(),
                name,
                password);

            if (not entity_id.is_default())
            {
                boost::lock_guard<boost::recursive_mutex> write_lock(
                    router_lock);

                // Good password, now confirm we don't already have a session.
                //
                ClientSession * const session_ptr =
                    get_entity_session(entity_id);

                if (session_ptr)
                {
                    // Can't use this call when session already exists,
                    // so disconnect and cleanup the old one first.
                    disconnect_session(entity_id);
                }
            }
        }

        if (not entity_id.is_default())
        {
            // Make session and add it.
            // Currently the session ID is not used, and is defaulted.
            //

            // Need to set this in advance of adding the session because
            // ClientSession needs it.
            connection_ptr->client_set_entity_id(entity_id);

            new_session_ptr = new ClientSession(0, this, connection_ptr);

            add_session(
                connection_driver_ptr,
                connection_ptr,
                new_session_ptr,
                entity_id);
        }

        return new_session_ptr;
    }

    // ----------------------------------------------------------------------
    ClientSession *RouterSessionManager::reauthorize_client(
        const std::string &name,
        const std::string &password,
        ConnectionDriver *connection_driver_ptr,
        ClientConnection *connection_ptr,
        const bool make_new_if_not_found)
    {
        ClientSession *session_ptr = 0;
        dbtype::Id entity_id;

        if (connection_ptr and connection_driver_ptr)
        {
            entity_id = check_password(
                connection_ptr->client_get_site_id(),
                name,
                password);

            if (not entity_id.is_default())
            {
                boost::lock_guard<boost::recursive_mutex> callback_mutex(
                    callback_lock);
                boost::lock_guard<boost::recursive_mutex> write_lock(
                    router_lock);

                // Good password, now confirm we have an existing session.
                //
                session_ptr = get_entity_session(entity_id);

                if (session_ptr)
                {
                    // Finish reconnection
                    session_ptr->set_client_connection(connection_ptr);

                    add_session(
                        connection_driver_ptr,
                        connection_ptr,
                        session_ptr,
                        entity_id);
                }
                else
                {
                    // Can't use this call when session doesn't exists.
                    entity_id = dbtype::Id();

                    LOG(warning, "comm", "reauthorize_client",
                        "Connection from " + connection_ptr->client_get_source()
                        + " attempted to reauthorize when no session.");
                }
            }
        }

        if (entity_id.is_default() and make_new_if_not_found)
        {
            return authorize_client(
                name,
                password,
                connection_driver_ptr,
                connection_ptr);
        }
        else
        {
            return session_ptr;
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::client_disconnected(
        ClientConnection *connection_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "client_disconnected",
                "Null connection pointer!");
        }
        else
        {
            session_ptr->client_disconnected();
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::data_acknowledge(
        ClientConnection *connection_ptr,
        const MessageSerialId ser_id)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "data_acknowledge",
                "Null connection pointer!");
        }
        else
        {
            session_ptr->client_data_acknowledge(ser_id);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::data_acknowledge_reconnect(
        ClientConnection *connection_ptr,
        const MessageSerialId ser_id)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "data_acknowledge_reconnect",
                "Null connection pointer!");
        }
        else
        {
            session_ptr->client_data_acknowledge_reconnect(ser_id);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::client_unblocked(
        ClientConnection *connection_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "client_unblocked",
                "Null connection pointer!");
        }
        else
        {
            session_ptr->client_unblocked();
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::data_from_client(
        ClientConnection *connection_ptr,
        const ChannelId channel_id,
        const MessageSerialId ser_id,
        text::ExternalTextLine *text_line_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "data_from_client(text)",
                "Null connection pointer!");

            text::ExternalText::clear_text_line(*text_line_ptr);
            delete text_line_ptr;
        }
        else
        {
            session_ptr->client_data(channel_id, ser_id, text_line_ptr);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::data_from_client(
        ClientConnection *connection_ptr,
        const ChannelId channel_id,
        const MessageSerialId ser_id,
        message::ClientMessage *client_message_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);
        ClientSession * const session_ptr = get_session(connection_ptr);

        if (not session_ptr)
        {
            LOG(error, "comm", "data_from_client(clientmessage)",
                "Null connection pointer!");

            delete client_message_ptr;
        }
        else
        {
            session_ptr->client_data(channel_id, ser_id, client_message_ptr);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::session_has_pending_actions(
        ClientSession *session_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);

        if (session_ptr)
        {
            pending_actions.push_back(session_ptr);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::release_connection(
        ClientConnection *connection_ptr)
    {
        boost::lock_guard<boost::recursive_mutex> callback_mutex(callback_lock);
        boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);

        ConnectionSessionMap::iterator find_iter =
            connection_to_session.find(connection_ptr);

        if (find_iter != connection_to_session.end())
        {
            find_iter->second.first->release(connection_ptr);
            connection_to_session.erase(find_iter);
        }
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::thread_main(void)
    {
        bool do_shutdown = false;
        bool more_work = false;
        timeval current_idle_check_time;
        timeval prev_idle_check_time;
        timespec start_time;
        timespec end_time;
        timespec time_diff;
        timespec time_to_wait;

        time_to_wait.tv_sec = 0;
        time_to_wait.tv_nsec = 0;
        prev_idle_check_time.tv_sec = 0;
        prev_idle_check_time.tv_usec = 0;

        if (clock_gettime(CLOCK_MONOTONIC, &start_time))
        {
            LOG(fatal, "comm", "thread_main",
                "clock_gettime(CLOCK_MONOTONIC) not supported!  "
                "Cannot service connections.");
            return;
        }

        // The main loop basically services all drivers and connections that
        // need it, and then sleeps for some nanoseconds.  The idea is to
        // loop at a constant rate per second, regardless of how long the loop
        // may take each time.
        //

        while (not do_shutdown)
        {
            if (not more_work)
            {
                clock_gettime(CLOCK_MONOTONIC, &start_time);
            }

            more_work = false;

            // Call everything with pending actions
            service_sessions();

            // Call all connection drivers so they can service connections
            //
            for (ConnectionDrivers::iterator driver_iter =
                    connection_drivers.begin();
                driver_iter != connection_drivers.end();
                ++driver_iter)
            {
                more_work = (not (*driver_iter)->do_work(this)) or more_work;
            }

            // Call everything with pending actions
            service_sessions();

            // Check idle and remove sessions that aren't active.
            //
            gettimeofday(&current_idle_check_time, 0);

            if (labs(current_idle_check_time.tv_sec -
                prev_idle_check_time.tv_sec) > DEFAULT_IDLE_CHECK_SEC)
            {
                // Time to check for idlers.  Find out which need to be
                // disconnected, then handle the disconnects after the fact
                // to avoid messing up iterators.
                //
                prev_idle_check_time = current_idle_check_time;
                dbtype::Entity::IdVector ids_to_disconnect;
                ClientSession *session_ptr = 0;

                // Scope for read_lock
                {
                    boost::lock_guard<boost::recursive_mutex> read_lock(
                        router_lock);

                    for (SessionConnectionMap::iterator session_iter =
                            session_to_connection.begin();
                        session_iter != session_to_connection.end();
                        ++session_iter)
                    {
                        session_ptr = session_iter->first;

                        if (session_ptr->is_connected())
                        {
                            // Still connected, use inactivity timeout.
                            //
                            if (session_ptr->get_session_activity_time().
                                get_relative_seconds() > config::comm::idle_time())
                            {
                                // Idle too long.  Disconnect.
                                ids_to_disconnect.push_back(
                                    session_ptr->get_entity_id());
                            }
                        }
                        else
                        {
                            // Not connected, use reconnect timeout.
                            //
                            if (session_ptr->get_session_activity_time().
                                get_relative_seconds() >
                                    config::comm::reconnect_wait_time())
                            {
                                // Idle too long.  Disconnect.
                                ids_to_disconnect.push_back(
                                    session_ptr->get_entity_id());
                            }
                        }
                    }
                }

                if (not ids_to_disconnect.empty())
                {
                    // Inactive sessions need to be disconnected.
                    //
                    for (dbtype::Entity::IdVector::const_iterator id_iter =
                            ids_to_disconnect.begin();
                        id_iter != ids_to_disconnect.end();
                        ++id_iter)
                    {
                        disconnect_session(*id_iter);
                    }
                }
            }

            if (not more_work)
            {
                clock_gettime(CLOCK_MONOTONIC, &end_time);

                // Determine how much more time to wait based on how long the
                // last calls took.  This cannot be negative due to the clock
                // type we are using.
                //
                osinterface::TimeUtils::timespec_substract(
                    end_time,
                    start_time,
                    time_diff);

                if ((not time_diff.tv_sec) and
                    (time_diff.tv_nsec < DEFAULT_SLEEP_TIME_NANOSEC))
                {
                    // Did not take more time than the periodic sleep, so
                    // calculate how long to sleep and then do the sleep.
                    //
                    time_to_wait.tv_nsec =
                        DEFAULT_SLEEP_TIME_NANOSEC - time_diff.tv_nsec;

                    nanosleep(&time_to_wait, 0);
                }
                else
                {
                    // We're running late, but should let others get in
                    // so we don't starve other threads needing locks
                    //
                    osinterface::ThreadUtils::yield();
                }

                do_shutdown = shutdown_thread_flag.load();
            }
        }
    }

    // ----------------------------------------------------------------------
    ClientConnection* RouterSessionManager::get_connection(
        ClientSession *session_ptr)
    {
        SessionConnectionMap::iterator find_iter =
            session_to_connection.find(session_ptr);

        if (find_iter != session_to_connection.end())
        {
            return find_iter->second;
        }

        return 0;
    }

    // ----------------------------------------------------------------------
    ClientSession* RouterSessionManager::get_session(
        ClientConnection *connection_ptr)
    {
        ConnectionSessionMap::iterator find_iter =
            connection_to_session.find(connection_ptr);

        if (find_iter != connection_to_session.end())
        {
            return find_iter->second.second;
        }

        return 0;
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::service_sessions(void)
    {
        boost::lock_guard<boost::recursive_mutex> callback_mutex(callback_lock);

        SessionQueue sessions;

        // Scope for lock
        {
            boost::lock_guard<boost::recursive_mutex> write_lock(router_lock);

            // Swap the queues, to efficiently make the sessions to process
            // local so additional locks don't have to be held or constantly
            // reacquired.
            sessions.swap(pending_actions);
        }

        while (not sessions.empty())
        {
            sessions.front()->process_pending();
            sessions.pop_front();
        }
    }

    // ----------------------------------------------------------------------
    dbtype::Id RouterSessionManager::check_password(
        const dbtype::Id::SiteIdType site_id,
        const std::string &name,
        const std::string &password)
    {
        dbtype::Id validated_player;

        dbinterface::DatabaseAccess * const database_ptr =
            dbinterface::DatabaseAccess::instance();

        const dbtype::Entity::IdVector result_ids = database_ptr->find(
            site_id,
            dbtype::ENTITYTYPE_player,
            name,
            true);

        if (not result_ids.empty())
        {
            // Retrieve database item and check password
            dbinterface::EntityRef entity =
                database_ptr->get_entity(result_ids.front());

            if (entity.valid() and
                dynamic_cast<dbtype::Player *>(entity.get()))
            {
                if (static_cast<dbtype::Player *>(entity.get())->
                    check_password(password))
                {
                    // Valid password, so set the ID.
                    validated_player = entity.id();
                }
            }
        }

        return validated_player;
    }

    // ----------------------------------------------------------------------
    void RouterSessionManager::add_entity_session(
        const dbtype::Id &id,
        ClientSession *session_ptr)
    {
        if (id.is_default())
        {
            LOG(error, "comm", "add_entity_session",
                "id is default!");
        }
        else if (not session_ptr)
        {
            LOG(error, "comm", "add_entity_session",
                "session_ptr is null!");
        }
        else
        {
            site_to_sessions[id.get_site_id()][id] = session_ptr;
        }
    }

    // ----------------------------------------------------------------------
    ClientSession *RouterSessionManager::get_entity_session(
        const dbtype::Id &id) const
    {
        ClientSession *result = 0;

        SiteSessionsMap::const_iterator site_iter =
            site_to_sessions.find(id.get_site_id());

        if (site_iter != site_to_sessions.end())
        {
            EntitySessionMap::const_iterator entity_iter =
                site_iter->second.find(id);

            if (entity_iter != site_iter->second.end())
            {
                result = entity_iter->second;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool RouterSessionManager::remove_entity_session(const dbtype::Id &id)
    {
        bool found = false;

        SiteSessionsMap::iterator site_iter =
            site_to_sessions.find(id.get_site_id());

        if (site_iter != site_to_sessions.end())
        {
            EntitySessionMap::iterator entity_iter =
                site_iter->second.find(id);

            if (entity_iter != site_iter->second.end())
            {
                found = true;
                site_iter->second.erase(entity_iter);

                // Also remove the site if nothing else is there.
                //
                if (site_iter->second.empty())
                {
                    site_to_sessions.erase(site_iter);
                }
            }
        }

        return found;
    }
}
}
