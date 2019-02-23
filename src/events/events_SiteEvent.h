/*
 * events_SiteEvent.h
 */

#ifndef MUTGOS_EVENTS_SITEEVENT_H
#define MUTGOS_EVENTS_SITEEVENT_H

#include <string>

#include "events/events_Event.h"
#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace events
{
    /**
     * Represents when a site is created or deleted.
     *
     * Future versions of this class may contain other types of site changes
     * as well.
     */
    class SiteEvent : public Event
    {
    public:
        /** The possible actions (changes) for sites */
        enum SiteAction
        {
            /** Site was just created */
            SITE_ACTION_CREATE,
            /** Site was just deleted */
            SITE_ACTION_DELETE
        };

        /**
         * Constructor that sets everything.
         * @param action[in] The site action (what changed on the site).
         * @param site[in] The site ID this event is about.
         * @param name[in] The name of the site.
         */
        SiteEvent(
            const SiteAction action,
            const dbtype::Id::SiteIdType site,
            const std::string name)
          : Event(Event::EVENT_SITE),
            site_action(action),
            site_id(site),
            site_name(name)
          { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        SiteEvent(const SiteEvent &rhs)
            : Event(rhs),
              site_action(rhs.site_action),
              site_id(rhs.site_id),
              site_name(rhs.site_name)
          { }

        /**
         * Required virtual destructor.
         */
        virtual ~SiteEvent()
          { }

        /**
         * @return The event as a string, for diagnostic/logging purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * @return The site action (what changed about the site).
         */
        SiteAction get_site_action(void) const
          { return site_action; }

        /**
         * @return The site ID this event is about.
         */
        dbtype::Id::SiteIdType get_site_id(void) const
          { return site_id; }

        /**
         * @return The site name this event is about.
         */
        const std::string &get_site_name(void) const
          { return site_name; }

    private:
        const SiteAction site_action; ///< What change occurred to the site
        const dbtype::Id::SiteIdType site_id; ///< The site ID the event is about
        const std::string site_name; ///< The site name the event is about
    };
}
}

#endif //MUTGOS_EVENTS_SITEEVENT_H
