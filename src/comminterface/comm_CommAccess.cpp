/*
 * comm_CommAccess.cpp
 */

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/mutgos_config.h"

#include "comm_RouterSessionManager.h"
#include "comm_SessionStats.h"

#include "websocketcomm/websocket_WebsocketDriver.h"
#include "socketcomm/socket_SocketDriver.h"

#include "comm_CommAccess.h"

namespace mutgos
{
namespace comm
{
    // Static initialization
    //
    CommAccess *CommAccess::singleton_ptr = 0;


    // ----------------------------------------------------------------------
    CommAccess* CommAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new CommAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void CommAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool CommAccess::startup(void)
    {
        return router.startup();
    }

    // ----------------------------------------------------------------------
    void CommAccess::shutdown(void)
    {
        return router.shutdown();
    }

    // ----------------------------------------------------------------------
    bool CommAccess::add_channel(
        const dbtype::Id &id,
        events::Channel *channel_ptr,
        const bool to_client)
    {
        return router.add_channel(id, channel_ptr, to_client);
    }

    // ----------------------------------------------------------------------
    bool CommAccess::disconnect_session(const mutgos::dbtype::Id &entity_id)
    {
        return router.disconnect_session(entity_id);
    }

    // ----------------------------------------------------------------------
    dbtype::Id::SiteIdVector CommAccess::get_entity_site_ids(void)
    {
        return router.get_entity_site_ids();
    }

    // ----------------------------------------------------------------------
    MG_UnsignedInt CommAccess::get_session_online_count(
        const dbtype::Id::SiteIdType site_id)
    {
        return router.get_session_online_count(site_id);
    }

    // ----------------------------------------------------------------------
    CommAccess::SessionStatsVector CommAccess::get_session_stats(
        const dbtype::Id::SiteIdType site_id)
    {
        return router.get_session_stats(site_id);
    }

    // ----------------------------------------------------------------------
    dbtype::Entity::IdVector CommAccess::get_online_ids(
        const dbtype::Id::SiteIdType site_id)
    {
        return router.get_online_ids(site_id);
    }

    // ----------------------------------------------------------------------
    SessionStats CommAccess::get_session_stats(const dbtype::Id &entity_id)
    {
        return router.get_session_stats(entity_id);
    }

    // ----------------------------------------------------------------------
    CommAccess::CommAccess(void)
    {
        add_comm_modules();
    }

    // ----------------------------------------------------------------------
    CommAccess::~CommAccess(void)
    {
        shutdown();
    }

    // ----------------------------------------------------------------------
    bool CommAccess::add_comm_modules(void)
    {
        if (config::comm::ws_enable_plain())
        {
            router.add_connection_driver(
                new websocket::WebsocketDriver(&router));
        }

        if (config::comm::so_enable_plain() or config::comm::so_enable_ssl())
        {
            router.add_connection_driver(new socket::SocketDriver(&router));
        }

        return true;
    }
}
}
