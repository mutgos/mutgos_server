/*
 * events_SiteSubscriptionParams.cpp
 */

#include <string>
#include <ostream>
#include <sstream>

#include "events_SiteSubscriptionParams.h"
#include "events_SiteEvent.h"

namespace mutgos
{
namespace events
{
    // ----------------------------------------------------------------------
    SiteSubscriptionParams::SiteSubscriptionParams(void)
        : SubscriptionParams(SubscriptionParams::SUBSCRIPTION_SITE)
    {
    }

    // ----------------------------------------------------------------------
    SiteSubscriptionParams::SiteSubscriptionParams(
        const SiteSubscriptionParams &rhs)
        : SubscriptionParams(rhs)
    {
    }

    // ----------------------------------------------------------------------
    SiteSubscriptionParams::~SiteSubscriptionParams()
    {
    }

    // ----------------------------------------------------------------------
    SiteSubscriptionParams &SiteSubscriptionParams::operator=(
        const SiteSubscriptionParams &rhs)
    {
        SubscriptionParams::params_copy_from(rhs);
        return *this;
    }

    // ----------------------------------------------------------------------
    bool SiteSubscriptionParams::operator==(
        const SiteSubscriptionParams &rhs) const
    {
        return SubscriptionParams::params_equal_to(rhs);
    }

    // ----------------------------------------------------------------------
    bool SiteSubscriptionParams::validate(void) const
    {
        return true;
    }

    // ----------------------------------------------------------------------
    SubscriptionParams *SiteSubscriptionParams::clone(void) const
    {
        return new SiteSubscriptionParams(*this);
    }

    // ----------------------------------------------------------------------
    bool SiteSubscriptionParams::references_id(const dbtype::Id &id) const
    {
        return false;
    }

    // ----------------------------------------------------------------------
    bool SiteSubscriptionParams::references_site(
        const dbtype::Id::SiteIdType site_id) const
    {
        return false;
    }

    // ----------------------------------------------------------------------
    std::string SiteSubscriptionParams::to_string(void) const
    {
        std::ostringstream strstream;

        strstream << "SiteSubscriptionParams" << std::endl
                  << SubscriptionParams::to_string()
                  << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool SiteSubscriptionParams::is_match(const SiteEvent *event_ptr) const
    {
        return true;
    }
}
}
