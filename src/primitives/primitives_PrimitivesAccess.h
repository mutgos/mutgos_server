/*
 * primitives_PrimitivesAccess.h
 */

#ifndef MUTGOS_PRIMITIVES_PRIMITIVESACCESS_H
#define MUTGOS_PRIMITIVES_PRIMITIVESACCESS_H

#include "primitives_DatabasePrims.h"
#include "primitives_EventPrims.h"
#include "primitives_MovementPrims.h"
#include "primitives_SystemPrims.h"

namespace mutgos
{
namespace primitives
{
    // TODO Make all primitive classes no-copy

    /**
     * Provides a way to access all the primitives in this module.
     * Classes outside of this namespace shall use this class to access
     * anything they need.
     *
     * Do not attempt to hold onto or make copies of any returned primitives.
     */
    class PrimitivesAccess
    {
    public:
        /**
         * Creates the singleton if it doesn't already exist.
         * @return The singleton instance.
         */
        static PrimitivesAccess *make_singleton(void);

        /**
         * Will NOT create singleton if it doesn't already exist.
         * This is thread safe.
         * @return The singleton instance, or null if not created.
         */
        static inline PrimitivesAccess *instance(void)
        { return singleton_ptr; }

        /**
         * Destroys the singleton instance if it exists, calling shutdown()
         * as needed.
         */
        static void destroy_singleton(void);

        /**
         * Initializes the singleton instance; called once as MUTGOS is coming
         * up and before any methods below are called.
         * Attempting to use other methods prior to calling startup() may
         * result in a crash.
         * @return True if success.  If false is returned, MUTGOS should
         * fail initialization completely.
         */
        bool startup(void);

        /**
         * Shuts down the singleton instance; called when MUTGOS is coming down.
         * Basically the opposite of what startup() does.
         */
        void shutdown(void);

        /**
         * @return The database primitives.
         */
        DatabasePrims &database_prims(void)
        { return singleton_database_prims; }

        /**
         * @return The event primitives.
         */
        EventPrims &event_prims(void)
        { return singleton_event_prims; }

        /**
         * @return The movement primitives.
         */
        MovementPrims &movement_prims(void)
        { return singleton_movement_prims; }

        /**
         * @return The system primitives.
         */
        SystemPrims & system_prims(void)
        { return singleton_system_prims; }

    private:

        /**
         * Singleton constructor.
         */
        PrimitivesAccess(void);

        /**
         * Singleton destructor.
         */
        virtual ~PrimitivesAccess();


        static PrimitivesAccess *singleton_ptr; ///< Singleton pointer.

        DatabasePrims singleton_database_prims;  ///< Database primitives
        EventPrims singleton_event_prims;        ///< Event primitives
        MovementPrims singleton_movement_prims;  ///< Movement primitives
        SystemPrims singleton_system_prims;      ///< System primitives
    };
}
}

#endif //MUTGOS_PRIMITIVES_PRIMITIVESACCESS_H
