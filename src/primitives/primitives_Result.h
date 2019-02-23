/*
 * primitives_Result.h
 */

#ifndef MUTGOS_PRIMITIVES_RESULT_H
#define MUTGOS_PRIMITIVES_RESULT_H

#include <string>

namespace mutgos
{
namespace primitives
{
    /**
     * A simple class that indicates if a primitive succeeded or failed, and
     * if it failed the general reason why.
     */
    class Result
    {
    public:
        // Keep the CPP string array and method updated when these are changed
        /** Indicates why a primnitive failed. */
        enum Status
        {
            /** SUCCESS: Indicates primitive succeeded with no errors */
            STATUS_OK,
            /** ERROR: Indicates primitive failed due to a security violation */
            STATUS_SECURITY_VIOLATION,
            /** ERROR: Indicates invalid arguments (default IDs, empty strings,
                invalid IDs, etc) passed to primitive. */
            STATUS_BAD_ARGUMENTS,
            /** ERROR: Indicates incorrect entity types provided (example:
                providing a Room when the primitive expected a player) */
            STATUS_BAD_ENTITY_TYPE,
            /** ERROR: The arguments are valid but the operation cannot be
                performed on them (example: Putting a player inside itself) */
            STATUS_IMPOSSIBLE
        };

        /**
         * Default constructor.  Sets status to 'OK'.
         */
        Result(void)
          : status(STATUS_OK)
        { }

        /**
         * Destructor.
         */
        ~Result()
        { }

        /**
         * @return True if status indicates success, false otherwise.
         */
        bool is_success(void) const
        { return (status == STATUS_OK); }

        /**
         * @return True if status indicates a security violation, false
         * otherwise.
         */
        bool is_security_violation(void) const
        { return (status == STATUS_SECURITY_VIOLATION); }

        /**
         * @return The status.
         */
        Status get_status(void) const
        { return status; }

        /**
         * Sets the status.  Only used by code within the primitives module.
         * @param new_status[in] The new status.
         */
        void set_status(const Status new_status)
        { status = new_status; }

        /**
         * @return The status in string form.
         */
        const std::string &status_to_string(void) const;

    private:
        Status status; ///< The status of the operation
    };
}
}

#endif //MUTGOS_PRIMITIVES_RESULT_H
