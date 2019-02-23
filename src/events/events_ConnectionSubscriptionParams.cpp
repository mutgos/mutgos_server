/*
 * events_ConnectionSubscriptionParams.cpp
 */

#include <string>
#include <algorithm>
#include <ostream>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

#include "events/events_ConnectionSubscriptionParams.h"
#include "events/events_ConnectionEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    ConnectionSubscriptionParams::ConnectionSubscriptionParams(void)
        : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_CONNECTION),
          connection_type(ConnectionSubscriptionParams::CONNECTION_TYPE_ALL)
    {
    }

    // ----------------------------------------------------------------------
    ConnectionSubscriptionParams::ConnectionSubscriptionParams(
        const ConnectionTypeSubscription type,
        const dbtype::Entity::IdVector &ids,
        const dbtype::Id::SiteIdVector &sites,
        const ConnectionSubscriptionParams::SourceList &sources)
        : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_CONNECTION),
          connection_type(type),
          connection_entity_ids(ids),
          connection_site_ids(sites)
    {
        // Sources need special processing.
        //
        for (SourceList::const_iterator source_iter = sources.begin();
            source_iter != sources.end();
            ++source_iter)
        {
            add_source(*source_iter);
        }
    }

    // ----------------------------------------------------------------------
    ConnectionSubscriptionParams::ConnectionSubscriptionParams(
        const ConnectionSubscriptionParams &rhs)
        : SubscriptionParams(rhs),
          connection_type(rhs.connection_type),
          connection_entity_ids(rhs.connection_entity_ids),
          connection_site_ids(rhs.connection_site_ids),
          connection_sources(rhs.connection_sources)
    {
    }

    // ----------------------------------------------------------------------
    ConnectionSubscriptionParams::~ConnectionSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    ConnectionSubscriptionParams &ConnectionSubscriptionParams::operator=(
        const ConnectionSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);

        connection_type = rhs.connection_type;
        connection_entity_ids = rhs.connection_entity_ids;
        connection_site_ids = rhs.connection_site_ids;
        connection_sources = rhs.connection_sources;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool ConnectionSubscriptionParams::operator==(
        const ConnectionSubscriptionParams &rhs) const
    {
        bool equals = false;

        // This is an exact equal in that the lists must be in the same
        // order.  This is primarily to detect if rhs is an exact clone of
        // lhs.
        //
        if (SubscriptionParams::params_equal_to(rhs))
        {
            equals = (connection_type == rhs.connection_type) and
                (connection_entity_ids == rhs.connection_entity_ids) and
                (connection_site_ids == rhs.connection_site_ids) and
                (connection_sources == rhs.connection_sources);
        }

        return equals;
    }

    // ----------------------------------------------------------------------
    void ConnectionSubscriptionParams::add_source(
        const std::string &source)
    {
        // Sources shall always be in lowercase to make matching easier.
        // The event will also make sure the source is lowercase on its side.
        //
        const std::string source_lower = boost::to_lower_copy(source);

        if ((not source_lower.empty()) and (std::find(
              connection_sources.begin(),
              connection_sources.end(),
              source_lower) == connection_sources.end()))
        {
            connection_sources.push_back(source_lower);
        }
    }

    // ----------------------------------------------------------------------
    bool ConnectionSubscriptionParams::validate(void) const
    {
        const bool valid = (connection_entity_ids.empty() and
            connection_site_ids.empty()) or
            (connection_entity_ids.empty() != connection_site_ids.empty());

        return valid;
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *ConnectionSubscriptionParams::clone(void) const
    {
        return new ConnectionSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool ConnectionSubscriptionParams::references_id(
        const dbtype::Id &id) const
    {
        return has_entity_id(id, connection_entity_ids);
    }

    // ----------------------------------------------------------------------
    bool ConnectionSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return has_site_id(site_id, connection_entity_ids) or
            has_site_id(site_id, connection_site_ids);
    }

    // ----------------------------------------------------------------------
    std::string ConnectionSubscriptionParams::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "ConnectionSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << "connection type:  " << connection_type << std::endl
                  << "entity IDs:       ";

        BOOST_FOREACH(const dbtype::Id &id, connection_entity_ids)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "site IDs:         ";

        BOOST_FOREACH(const dbtype::Id::SiteIdType &id, connection_site_ids)
        {
            strstream << id << "  ";
        }

        strstream << std::endl
                  << "sources:        ";

        BOOST_FOREACH(const std::string &source, connection_sources)
        {
            strstream << source << "  ";
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool ConnectionSubscriptionParams::is_match(
        const ConnectionEvent *event_ptr) const
    {
        if (not event_ptr)
        {
            return false;
        }

        bool match = true;

        // Connection type
        //
        if (connection_type != CONNECTION_TYPE_ALL)
        {
            switch (event_ptr->get_action())
            {
                case ConnectionEvent::ACTION_CONNECTED:
                {
                    match = (connection_type == CONNECTION_TYPE_CONNECT);
                    break;
                }

                case ConnectionEvent::ACTION_DISCONNECTED:
                {
                    match = (connection_type == CONNECTION_TYPE_DISCONNECT);
                    break;
                }

                default:
                {
                    LOG(error, "events", "is_match",
                        "Unknown connection action type.");

                    match = false;
                    break;
                }
            }
        }

        // Entity ID / Site ID
        //
        if (match)
        {
            if (not connection_entity_ids.empty())
            {
                match = std::find(
                    connection_entity_ids.begin(),
                    connection_entity_ids.end(),
                    event_ptr->get_entity_id()) != connection_entity_ids.end();
            }
            else if (not connection_site_ids.empty())
            {

                match = std::find(
                    connection_site_ids.begin(),
                    connection_site_ids.end(),
                    event_ptr->get_entity_id().get_site_id()) !=
                        connection_site_ids.end();
            }
        }

        // Connection hosts and addresses partial match
        //
        if (match and ((not connection_sources.empty())))
        {
            bool source_matched = false;
            const std::string &event_source = event_ptr->get_source();

            // Try matching host first.
            //
            for (SourceList::const_iterator source_iter =
                    connection_sources.begin();
                 source_iter != connection_sources.end();
                ++source_iter)
            {
                if (event_source.find(*source_iter) != std::string::npos)
                {
                    source_matched = true;
                    break;
                }
            }

            if (not source_matched)
            {
                match = false;
            }
        }

        return match;
    }
}
}
