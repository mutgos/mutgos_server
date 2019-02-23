/*
 * events_SubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_SUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_SUBSCRIPTIONPARAMS_H

#include <string>
#include <algorithm>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

namespace mutgos
{
namespace events
{
    /**
     * Base class for all subscription parameter classes, which specify
     * parameters for an event-based subscription.
     *
     * Since the (sub)class's parameters are not modified after being
     * accepted by the events subsystem, thread safety is not needed at this
     * time.  Therefore, the class is not thread safe.
     *
     * Currently this is a pure abstract class, but this should not be
     * depended on.
     */
    class SubscriptionParams
    {
    public:
        /** Type of subscription subclass */
        enum SubscriptionType
        {
            /** MovementSubscriptionParams class */
            SUBSCRIPTION_MOVEMENT,
            /** EmitSubscriptionParams class */
            SUBSCRIPTION_EMIT,
            /** ConnectionSubscriptionParams class */
            SUBSCRIPTION_CONNECTION,
            /** EntityChangedSubscriptionParams class */
            SUBSCRIPTION_ENTITY_CHANGED,
            /** ProcessExecutionSubscriptionParams class */
            SUBSCRIPTION_PROCESS_EXECUTION,
            /** SiteEventSubscriptionParams class */
            SUBSCRIPTION_SITE
        };

        /**
         * Required virtual destructor.
         */
        virtual ~SubscriptionParams()
          { }

        /**
         * @return The type of subscription.
         */
        SubscriptionType get_type(void) const
          { return subscription_type; }

        /**
         * Validates that the subscription is valid (has all needed fields
         * filled in and that they are properly filled in.
         * @return True if subscription is valid, false if there is a problem.
         */
        virtual bool validate(void) const =0;

        /**
         * @return A copy of this subscription.  Caller is responsible for
         * managing the pointer.
         */
        virtual SubscriptionParams *clone(void) const =0;

        /**
         * @param id[in] The ID to check.
         * @return True if the subscription parameters specifically reference
         * the given ID anywhere.
         */
        virtual bool references_id(const dbtype::Id &id) const =0;

        /**
         * @param site_id[in] The site ID to check.
         * @return True if the subscription parameters specifically reference
         * the given site ID anywhere, including in entity IDs.
         */
        virtual bool references_site(
            const dbtype::Id::SiteIdType site_id) const =0;

        /**
         * Subclasses must override this, but also call parent.
         * @return The subscription as a string, for diagnostic/logging
         * purposes.
         */
        virtual std::string to_string(void) const
          { return "\n"; }

    protected:
        /**
         * Abstract class constructor.
         */
        SubscriptionParams(const SubscriptionType type)
            : subscription_type(type)
          { }

        /**
         * Abstract class copy constuctor.
         * @param rhs[in]  Source of the data to copy.
         */
        SubscriptionParams(const SubscriptionParams &rhs)
            : subscription_type(rhs.subscription_type)
          { }

        /**
         * Copies any data at this class level from the source into this
         * instance.
         * For future expansion.
         * @param source[in]  Source of the data to copy.
         */
        void params_copy_from(const SubscriptionParams &source)
          { }

        /**
         * Determines if the given SubscriptionParams is equal to this one.
         * For future expansion.
         * @param rhs[in] The class instance to check.
         * @return True (for now).
         */
        bool params_equal_to(const SubscriptionParams &rhs) const
          { return (subscription_type == rhs.subscription_type); }

        /**
         * Determines if the Entity ID set has the given entity ID.
         * @param id[in] The Entity ID to look for.
         * @param id_set[in] The ID set to search in.
         * @return True if id_set conrtains id.
         */
        bool has_entity_id(
            const dbtype::Id &id,
            const dbtype::Entity::IdSet &id_set) const
          { return id_set.find(id) != id_set.end(); }

        /**
         * Determines if the Entity ID vector has the given entity ID.
         * @param id[in] The Entity ID to look for.
         * @param id_vector[in] The ID vector to search in.
         * @return True if id_vector conrtains id.
         */
        bool has_entity_id(
            const dbtype::Id &id,
            const dbtype::Entity::IdVector &id_vector) const
        {
            return std::find(
                id_vector.begin(),
                id_vector.end(),
                id) != id_vector.end();
        }

        /**
         * Determines if the Entity ID set has any Entities from the given
         * site.
         * @param site_id[in] The site ID to look for.
         * @param id_set[in] The ID set to search in.
         * @return True if id_set conrtains an Entity ID from the given site.
         */
        bool has_site_id(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Entity::IdSet &id_set) const
        {
            // Have to check every Id to see if it has the site.
            //
            for (dbtype::Entity::IdSet::const_iterator iter = id_set.begin();
                iter != id_set.end();
                ++iter)
            {
                if (iter->get_site_id() == site_id)
                {
                    return true;
                }
            }

            return false;
        }

        /**
         * Determines if the Entity ID vector has any Entities from the given
         * site.
         * @param site_id[in] The site ID to look for.
         * @param id_vector[in] The ID vector to search in.
         * @return True if id_vector conrtains an Entity ID from the given site.
         */
        bool has_site_id(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Entity::IdVector &id_vector) const
        {
            // Have to check every Id to see if it has the site.
            //
            for (dbtype::Entity::IdVector::const_iterator iter =
                    id_vector.begin();
                 iter != id_vector.end();
                 ++iter)
            {
                if (iter->get_site_id() == site_id)
                {
                    return true;
                }
            }

            return false;
        }

        /**
         * Determines if the Entity ID is from the given site.
         * @param site_id[in] The site ID to look for.
         * @param id_to_check[in] The ID to check.
         * @return True if the ID is from the given site.
         */
        bool has_site_id(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Id &id_to_check) const
          { return (id_to_check.get_site_id() == site_id); }

        /**
         * Determines if the site set has the given site.
         * @param site_id[in] The site ID to look for.
         * @param site_set[in] The set of IDs
         * @return True if site_set has the given site ID.
         */
        bool has_site_id(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Id::SiteIdSet &site_set) const
          { return (site_set.find(site_id) != site_set.end()); }

        /**
         * Determines if the site vector has the given site.
         * @param site_id[in] The site ID to look for.
         * @param site_vector[in] The vector of IDs
         * @return True if vector has the given site ID.
         */
        bool has_site_id(
            const dbtype::Id::SiteIdType site_id,
            const dbtype::Id::SiteIdVector &site_vector) const
        {
            return std::find(
                site_vector.begin(),
                site_vector.end(),
                site_id) != site_vector.end();
        }

    private:
        // Base class can't copy or compare directly.
        //
        SubscriptionParams &operator=(const SubscriptionParams &rhs);
        bool operator==(const SubscriptionParams &rhs) const;

        const SubscriptionType subscription_type; ///< Type of subclass

    };
}
}

#endif //MUTGOS_EVENTS_SUBSCRIPTIONPARAMS_H
