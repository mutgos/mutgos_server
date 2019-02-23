/*
 * events_MovementSubscriptionParams.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include <boost/foreach.hpp>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

#include "events/events_MovementSubscriptionParams.h"
#include "events/events_MovementEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    MovementSubscriptionParams::MovementSubscriptionParams(void)
        : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_MOVEMENT),
          movement_site(0),
          movement_type(MovementSubscriptionParams::MOVEMENT_TYPE_ALL)
    {
    }

    // ----------------------------------------------------------------------
    MovementSubscriptionParams::MovementSubscriptionParams(
        const dbtype::Entity::IdVector &who,
        const dbtype::Entity::IdVector &from,
        const dbtype::Entity::IdVector &to,
        const dbtype::Id::SiteIdType site,
        const MovementType type,
        const dbtype::Id &how)
        : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_MOVEMENT),
          movement_who(who),
          movement_from(from),
          movement_to(to),
          movement_site(site),
          movement_type(type),
          movement_how(how)
    {
    }

    // ----------------------------------------------------------------------
    MovementSubscriptionParams::MovementSubscriptionParams(
        const MovementSubscriptionParams &rhs)
        : SubscriptionParams(rhs),
          movement_who(rhs.movement_who),
          movement_from(rhs.movement_from),
          movement_to(rhs.movement_to),
          movement_site(rhs.movement_site),
          movement_type(rhs.movement_type),
          movement_how(rhs.movement_how)
    {
    }

    // ----------------------------------------------------------------------
    MovementSubscriptionParams::~MovementSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    MovementSubscriptionParams &MovementSubscriptionParams::operator=(
        const MovementSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);

        movement_who = rhs.movement_who;
        movement_from = rhs.movement_from;
        movement_to = rhs.movement_to;
        movement_site = rhs.movement_site;
        movement_type = rhs.movement_type;
        movement_how = rhs.movement_how;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool MovementSubscriptionParams::operator==(
        const MovementSubscriptionParams &rhs) const
    {
        bool equals = false;

        // This is an exact equal in that the lists must be in the same
        // order.  This is primarily to detect if rhs is an exact clone of
        // lhs.
        //
        if (SubscriptionParams::params_equal_to(rhs))
        {
            equals = (movement_who == rhs.movement_who) and
                (movement_from == rhs.movement_from) and
                (movement_to == rhs.movement_to) and
                (movement_site == rhs.movement_site) and
                (movement_type == rhs.movement_type) and
                (movement_how == rhs.movement_how);
        }

        return equals;
    }

    // ----------------------------------------------------------------------
    bool MovementSubscriptionParams::validate(void) const
    {
        bool valid = true;

        if (movement_site)
        {
            valid = movement_who.empty() and movement_from.empty() and
                movement_to.empty();
        }

        return valid;
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *MovementSubscriptionParams::clone(void) const
    {
        return new MovementSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool MovementSubscriptionParams::references_id(const dbtype::Id &id) const
    {
        return has_entity_id(id, movement_who) or
               has_entity_id(id, movement_from) or
               has_entity_id(id, movement_to) or
               (id == movement_how);
    }

    // ----------------------------------------------------------------------
    bool MovementSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return (movement_site == site_id) or
               has_site_id(site_id, movement_who) or
               has_site_id(site_id, movement_from) or
               has_site_id(site_id, movement_to) or
               has_site_id(site_id, movement_how);
    }

    // ----------------------------------------------------------------------
    std::string MovementSubscriptionParams::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "MovementSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << "who:       ";

        BOOST_FOREACH(const dbtype::Id &id, movement_who)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "from:      ";

        BOOST_FOREACH(const dbtype::Id &id, movement_from)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "to:        ";

        BOOST_FOREACH(const dbtype::Id &id, movement_to)
        {
            strstream << id.to_string(true) << "  ";
        }

        strstream << std::endl
                  << "site:      " << movement_site << std::endl
                  << "type:      " << movement_type << std::endl
                  << "how:       " << movement_how.to_string(true) << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool MovementSubscriptionParams::is_match(
        const MovementEvent *event_ptr) const
    {
        if (not event_ptr)
        {
            return false;
        }

        bool match = true;

        // Movement type
        //
        if (movement_type != MOVEMENT_TYPE_ALL)
        {
            if (event_ptr->get_program_flag())
            {
                match = (movement_type == MOVEMENT_TYPE_PROGRAM);
            }
            else
            {
                match = (movement_type == MOVEMENT_TYPE_EXIT);
            }
        }

        if (match)
        {
            if (movement_site)
            {
                // Movement site
                //
                match = (event_ptr->get_who().get_site_id() == movement_site) or
                    (event_ptr->get_from().get_site_id() == movement_site) or
                    (event_ptr->get_to().get_site_id() == movement_site);
            }
            else
            {
                // No site, so match who, from, to
                //
                if (not movement_who.empty())
                {
                    match = has_entity_id(event_ptr->get_who(), movement_who);
                }

                if (match and (not movement_from.empty()))
                {
                    match = has_entity_id(event_ptr->get_from(), movement_from);
                }

                if (match and (not movement_to.empty()))
                {
                    match = has_entity_id(event_ptr->get_to(), movement_to);
                }
            }
        }

        // Movement 'how'
        //
        if (match and (not movement_how.is_default()))
        {
            match = (movement_how == event_ptr->get_how());
        }

        return match;
    }
}
}
