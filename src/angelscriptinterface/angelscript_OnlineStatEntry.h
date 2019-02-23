/*
 * angelscript_OnlineStatEntry.h
 */

#ifndef MUTGOS_ANGELSCRIPT_ONLINESTATENTRY_H
#define MUTGOS_ANGELSCRIPT_ONLINESTATENTRY_H

#include <angelscript.h>

#include "dbtypes/dbtype_Id.h"

#include "angelscript_SimpleGCObject.h"
#include "osinterface/osinterface_OsTypes.h"
#include "utilities/memory_VirtualHeapAllocator.h"

#include "comminterface/comm_SessionStats.h"


namespace mutgos
{
namespace angelscript
{
    // Forward declarations
    //
    class AEntity;

    /**
     * A container class that holds selected data from a comm::SessionStats
     * class instance.
     *
     * Note this class is meant exclusively to interface with AngelScript.
     * It is designed to be a wrapper and not reusable.  When instantiating,
     * always do it on the heap.
     */
    class OnlineStatEntry : public SimpleGCObject
    {
    public:
        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * A factory used to create a new instance of an OnlineStatEntry,
         * equivalent to the default constructor.  The created
         * OnlineStatEntry will have default data.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void online_stat_entry_factory(asIScriptGeneric *gen_ptr);

        /**
         * A factory used to create a copy of an OnlineStatEntry, equivalent
         * to the copy constructor.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void online_stat_entry_factory_copy(asIScriptGeneric *gen_ptr);

        /**
         * Constructor that creates an instance with default data.
         * @param engine[in] The pointer to the script engine.
         */
        OnlineStatEntry(asIScriptEngine *engine);

        /**
         * Constructor that copies an existing instance.
         * @param rhs[in] The OnlineStatEntry to copy from.
         */
        OnlineStatEntry(const OnlineStatEntry &rhs);

        /**
         * Constructor that creates an instance with stats.
         * @param engine[in] The pointer to the script engine.
         * @param stats[in] The comm SessionStats object to copy from.
         */
        OnlineStatEntry(
            asIScriptEngine *engine,
            const comm::SessionStats &stats);

        /**
         * Required virtual destructor.
         */
        virtual ~OnlineStatEntry();

        /**
         * Assignment operator.
         * @param rhs[in] The OnlineStatEntry to copy from.
         * @return This.
         */
        OnlineStatEntry &operator=(const OnlineStatEntry &rhs);

        /**
         * Equality operator.
         * @param rhs[in] The OnlineStatEntry to check against.
         * @return True if they are equal.
         */
        bool operator==(const OnlineStatEntry &rhs) const;

        /**
         * @return True if this entry has valid (non-default) data.
         */
        bool is_valid(void) const;

        /**
         * TODO will need to make this hold onto reference later
         * @return The Entity these online stats are about.
         */
        AEntity *get_entity(void);

        /**
         * @return True if Entity is currently connected, false if session
         * is waiting for a reconnect.
         */
        bool is_connected(void) const;

        /**
         * @return How many seconds the Entity has been online.
         */
        asINT64 get_time_online_secs(void) const;

        /**
         * @return How many seconds the Entity has been idle (calculated when
         * class instance was made).
         */
        asINT64 get_idle_time_secs(void) const;

    private:
        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);


        dbtype::Id entity_id;  ///< The Entity this entry is about.
        bool connected;   ///< True if currently connected.
        MG_VeryLongUnsignedInt elapsed_secs_connection; ///< How many seconds Entity has been connected.
        MG_VeryLongUnsignedInt elapsed_secs_idle; ///< How many seconds Entity has been idle.
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_ONLINESTATENTRY_H
