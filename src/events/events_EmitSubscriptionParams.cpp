/*
 * events_EmitSubscriptionParams.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "events/events_EmitSubscriptionParams.h"
#include "events/events_EmitEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    EmitSubscriptionParams::EmitSubscriptionParams(void)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_EMIT)
    {
    }

    // ----------------------------------------------------------------------
    EmitSubscriptionParams::EmitSubscriptionParams(
        const dbtype::Id &source,
        const dbtype::Id &target,
        const dbtype::Id &my_id)
      : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_EMIT),
        emit_source(source),
        emit_target(target),
        emit_my_id(my_id)
    {
    }

    // ----------------------------------------------------------------------
    EmitSubscriptionParams::EmitSubscriptionParams(
        const EmitSubscriptionParams &rhs)
        : SubscriptionParams(rhs),
          emit_source(rhs.emit_source),
          emit_target(rhs.emit_target),
          emit_my_id(rhs.emit_my_id)
    {
    }

    // ----------------------------------------------------------------------
    EmitSubscriptionParams::~EmitSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    EmitSubscriptionParams& EmitSubscriptionParams::operator=(
        const EmitSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);

        emit_source = rhs.emit_source;
        emit_target = rhs.emit_target;
        emit_my_id = rhs.emit_my_id;

        return *this;
    }

    // ----------------------------------------------------------------------
    bool EmitSubscriptionParams::operator==(
        const EmitSubscriptionParams &rhs) const
    {
        bool equals = false;

        // This is an exact equal in that the lists must be in the same
        // order.  This is primarily to detect if rhs is an exact clone of
        // lhs.
        //
        if (SubscriptionParams::params_equal_to(rhs))
        {
            equals = (emit_source == rhs.emit_source) and
                (emit_target == rhs.emit_target) and
                (emit_my_id == rhs.emit_my_id);
        }

        return equals;
    }

    // ----------------------------------------------------------------------
    bool EmitSubscriptionParams::validate(void) const
    {
        return (not emit_source.is_default()) or (not emit_target.is_default());
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *EmitSubscriptionParams::clone(void) const
    {
        return new EmitSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool EmitSubscriptionParams::references_id(const dbtype::Id &id) const
    {
        return (emit_source == id) or (emit_target == id) or (emit_my_id == id);
    }

    // ----------------------------------------------------------------------
    bool EmitSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return has_site_id(site_id, emit_source) or
            has_site_id(site_id, emit_target) or
            has_site_id(site_id, emit_my_id);
    }

    // ----------------------------------------------------------------------
    std::string EmitSubscriptionParams::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "EmitSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << "source:  " << emit_source.to_string(true) << std::endl
                  << "target:  " << emit_target.to_string(true) << std::endl
                  << "my ID:   " << emit_my_id.to_string(true) << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool EmitSubscriptionParams::is_match(const EmitEvent *event_ptr) const
    {
        if (not event_ptr)
        {
            return false;
        }

        bool match = true;

        if (not emit_source.is_default())
        {
            match = (emit_source == event_ptr->get_source());
        }

        if (match and (not emit_target.is_default()))
        {
            match = (emit_target == event_ptr->get_target());
        }

        if (match and (not emit_my_id.is_default()) and
            (not event_ptr->get_exclude().is_default()))
        {
            match = (emit_my_id != event_ptr->get_exclude());
        }

        return match;
    }
}
}
