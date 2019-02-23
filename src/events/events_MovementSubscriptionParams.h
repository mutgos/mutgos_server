/*
 * events_MovementSubscriptionParams.h
 */

#ifndef MUTGOS_EVENTS_MOVEMENTSUBSCRIPTIONPARAMS_H
#define MUTGOS_EVENTS_MOVEMENTSUBSCRIPTIONPARAMS_H

#include <set>

#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_Entity.h"

#include "events/events_SubscriptionParams.h"

namespace mutgos
{
namespace events
{
    // Forward declarations
    //
    class MovementEvent;

    /**
     * A movement subscription.  This allows the subscriber to get notified
     * when an Entity has been moved from one Container to another, which
     * also includes when an Entity is newly created.
     *
     * Fields that are left at defaults (or empty) are considered wildcards.
     * For an example, not filling in who moved will match all movers.
     *
     * The movement site may only be filled in if the from and to are not
     * filled in.
     *
     * Note this is not a general purpose container.  Attributes, once set,
     * may not always be unsettable.
     */
    class MovementSubscriptionParams : public SubscriptionParams
    {
    public:
        /**
         * Interested types for the cause of movement.
         */
        enum MovementType
        {
            /** Movement was due to a program moving the Entity*/
            MOVEMENT_TYPE_PROGRAM,
            /** Movement was due to an Entity going through an Exit */
            MOVEMENT_TYPE_EXIT,
            /** Any movement type */
            MOVEMENT_TYPE_ALL
        };

        /**
         * Constructor with nothing set.
         */
        MovementSubscriptionParams(void);

        /**
         * Constructor that sets everything.
         * Refer to individual setters for more details about each parameter.
         * @param who[in] The Entities interested in knowing when they move.
         * @param from[in] The interested originating location of the movement.
         * @param to[in] The destination location of the movement.
         * @param site[in] The site ID of interest, if who, from, and to are
         * not set.
         * @param type[in] The type of the cause of movement.
         * @param how[in] The interested cause / how the movement occurred.
         */
        MovementSubscriptionParams(
            const dbtype::Entity::IdVector &who,
            const dbtype::Entity::IdVector &from,
            const dbtype::Entity::IdVector &to,
            const dbtype::Id::SiteIdType site,
            const MovementType type,
            const dbtype::Id &how);

        /**
         * Copy constructor.
         * @param rhs[in] The source for the copy.
         */
        MovementSubscriptionParams(const MovementSubscriptionParams &rhs);

        /**
         * Virtual destructor.
         */
        virtual ~MovementSubscriptionParams();

        /**
         * Assignment operator.
         * @param rhs[in] The source for the copy.
         * @return The updated destination.
         */
        MovementSubscriptionParams &operator=(
            const MovementSubscriptionParams &rhs);

        /**
         * Equals operator.
         * @param rhs[in] The class instance to check.
         * @return True if both instances are equal.
         */
        bool operator==(const MovementSubscriptionParams &rhs) const;

        /**
         * Adds an entity ID which we want to know if it moves.
         * Cannot be called if setting an interested site.
         * @param entity_id[in] The entity ID that we want to know if it moves.
         */
        void add_who(const dbtype::Id &entity_id)
          { movement_who.push_back(entity_id); }

        /**
         * @return The entity IDs interested in knowing if they move.
         */
        const dbtype::Entity::IdVector &get_who(void) const
          { return movement_who; }

        /**
         * Adds an entity ID of a Container which we want to know if anything
         * moves from it.
         * Cannot be called if setting an interested site.
         * @param entity_id[in] The entity ID that we want to know if anything
         * moves from it.
         */
        void add_from(const dbtype::Id &entity_id)
          { movement_from.push_back(entity_id); }

        /**
         * @return The entity IDs interested in knowing if anything moves from
         * them.
         */
        const dbtype::Entity::IdVector &get_from(void) const
          { return movement_from; }

        /**
         * Adds an entity ID of a Container which we want to know if anything
         * moves to it.
         * Cannot be called if setting an interested site.
         * @param entity_id[in] The entity ID that we want to know if anything
         * moves to it.
         */
        void add_to(const dbtype::Id &entity_id)
          { movement_to.push_back(entity_id); }

        /**
         * @return The entity IDs interested in knowing if anything moves to
         * them.
         */
        const dbtype::Entity::IdVector &get_to(void) const
          { return movement_to;}

        /**
         * Sets a site ID for the site we are interested in all movement in.
         * If this is set, cannot add 'who', 'from', or 'to' Entities.
         * You can still set 'how', but it will be of limited use unless
         * the programs are global.
         * @param site_id[in] The site ID of interest.
         */
        void set_site(const dbtype::Id::SiteIdType site_id)
          { movement_site = site_id; }

        /**
         * @return The site ID we are interested in knowing about any movement
         * in.
         */
        const dbtype::Id::SiteIdType get_site(void) const
          { return movement_site; }

        /**
         * Sets the interested type of how the entity moved (for instance,
         * via a program, or an exit).  The default is interested in every
         * type.
         * @param type[in] The type of movement of interest.
         */
        void set_movement_type(const MovementType type)
          { movement_type = type;}

        /**
         * @return The type of movement of interest.
         */
        const MovementType get_movement_type(void) const
          { return movement_type; }

        /**
         * Sets the interested cause of the movement.  It could be the entity
         * ID of a program, entity ID of an exit, etc, based on the
         * movement type.
         * @param entity_id[in] The cause of the movement.
         */
        void set_movement_how(const dbtype::Id &entity_id)
          { movement_how = entity_id; }

        /**
         * @return The cause of the movement.
         */
        const dbtype::Id &get_movement_how(void) const
          { return movement_how; }

        /**
         * Validates that the subscription is valid (has all needed fields
         * filled in and that they are properly filled in.
         * @return True if subscription is valid, false if there is a problem.
         */
        virtual bool validate(void) const;

        /**
         * @return A copy of this subscription.  Caller is responsible for
         * managing the pointer.
         */
        virtual SubscriptionParams *clone(void) const;

        /**
         * @param id[in] The ID to check.
         * @return True if the subscription parameters specifically reference
         * the given ID anywhere.
         */
        virtual bool references_id(const dbtype::Id &id) const;

        /**
         * @param site_id[in] The site ID to check.
         * @return True if the subscription parameters specifically reference
         * the given site ID anywhere, including in entity IDs.
         */
        virtual bool references_site(
            const dbtype::Id::SiteIdType site_id) const;

        /**
         * @return The subscription as a string, for diagnostic/logging
         * purposes.
         */
        virtual std::string to_string(void) const;

        /**
         * Evaluates the event and determine if it matches this subscription.
         * @param event_ptr[in] The event to evaluate.
         * @return True if subscription matches event.
         */
        virtual bool is_match(const MovementEvent *event_ptr) const;

    private:
        dbtype::Entity::IdVector movement_who; ///< Entities we want to know if moved
        dbtype::Entity::IdVector movement_from; ///< Interested sources of movement
        dbtype::Entity::IdVector movement_to; ///< Interested destinations of movement
        dbtype::Id::SiteIdType movement_site; ///< Site ID of interest
        MovementType movement_type; ///< Interested movement cause type
        dbtype::Id movement_how; ///< Interested cause of the movement
    };
}
}

#endif //MUTGOS_EVENTS_MOVEMENTSUBSCRIPTIONPARAMS_H
