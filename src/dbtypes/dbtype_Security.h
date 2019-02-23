/*
 * dbtype_Security.h
 */

#ifndef MUTGOS_DBTYPE_SECURITY_H_
#define MUTGOS_DBTYPE_SECURITY_H_

#include <string>
#include <bitset>
#include <vector>
#include <stddef.h>

#include "dbtype_Id.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>


namespace mutgos
{
namespace dbtype
{
    /**
     * Represents the allowed security flags.
     * @see SECURITY_FLAGS_STRING in implementation for character equivalents.
     */
    enum SecurityFlag
    {
        SECURITYFLAG_read = 0,  ///< Read everything permission
        SECURITYFLAG_write,     ///< Write everything permission
        SECURITYFLAG_basic,     ///< Read certain basic attributes permission
        SECURITYFLAG_chown,     ///< Chown permission
        SECURITYFLAG_invalid    ///< Also size of flag enum
    };

    // TODO Need to limit IDs stored here to 10(?) or less, to prevent large arrays, lots of copying

    /**
     * Represents the Security type, a field used throughout the database
     * to represent permissions.
     * <p><p>
     * Security is composed of the owner/admins, list of IDs
     * with less than admin permissions (if any), and other
     * permissions which everyone not part of the first two permissions falls
     * into.  This class does not check permissions, but simply provides a way
     * to access the values.
     */
    class Security
    {
    public:
        /** Represents the IDs in the List field */
        typedef std::vector<Id> SecurityIds;

        /** First is the IDs removed from a security instance, second are
         *  the IDs added.   Generally used as a result from diffing two
         *  instances. */
        typedef std::pair<SecurityIds, SecurityIds> RemoveAddPair;

        /**
         * @param flag[in] The flag in string format.
         * @return The flag as an enum
         */
        static SecurityFlag security_flag_from_string(const std::string &flag);

        /**
         * Standard constructor.
         */
        Security(void);

        /**
         * Copy constructor.
         * @param[in] rhs The source to copy from.
         */
        Security(const Security &rhs);

        /**
         * Standard destructor.
         */
        virtual ~Security();

        /**
         * Assignment operator.
         * @param[in] rhs The source to copy from.
         * @return The copy.
         */
        virtual Security &operator=(const Security &rhs);

        /**
         * Compares against another Security instance.
         * @param[in] rhs The Security instance to compare against.
         * @return True if the two instances are the same.
         */
        virtual bool operator==(const Security &rhs) const;

        /**
         * Compares against another Security instance, providing what IDs
         * have been added and removed between this one, and the one provided.
         * Compares this -> RHS.  The result indicates the changes to the IDs
         * which occurred to create rhs.
         * @param rhs The Security instance to compare against.
         * @return The IDs removed or added between this instance and rhs.
         */
        virtual RemoveAddPair diff_ids(const Security &rhs) const;

        /**
         * @return This Security instance as a string.
         */
        virtual std::string to_string(void) const;

        /**
         * Gets a security flag from the list field.
         * @param[in] flag The flag to get.
         * @return The value of the flag (true set, false unset).
         */
        virtual bool get_list_security_flag(const SecurityFlag flag) const;

        /**
         * Gets a security flag from the other field.
         * @param[in] flag The flag to get.
         * @return The value of the flag (true set, false unset).
         */
        virtual bool get_other_security_flag(const SecurityFlag flag) const;

        /**
         * Sets a security flag on the list field.
         * @param[in] flag The security flag to set.
         * @param[in] value The value of the flag to set.
         * @return True if success.
         */
        virtual bool set_list_security_flag(
            const SecurityFlag flag,
            const bool value);

        /**
         * Sets a security flag on the other field.
         * @param[in] flag The security flag to set.
         * @param[in] value The value of the flag to set.
         * @return True if success.
         */
        virtual bool set_other_security_flag(
            const SecurityFlag flag,
            const bool value);


        /**
         * @return A read-only version of the IDs contained in the admin field.
         */
        virtual const SecurityIds &get_admin_ids(void) const;

        /**
         * Determines if the provided ID is in the admin field.
         * @param[in] id The ID to check.
         * @return True if id is in admin field.
         */
        virtual bool is_admin(const Id &id) const;

        /**
         * Adds an ID to the admin field.  Please note if ID exists in
         * the list field, it will be removed there and added here.
         * @param[in] id The ID to add.
         * @return True if ID was added, false if not due to being a duplicate.
         */
        virtual bool add_admin(const Id &id);

        /**
         * Removes an ID from the admin field.
         * @param id[in] The ID to remove.
         * @return True if the ID was found and removed, false if ID was not
         * found.
         */
        virtual bool remove_admin(const Id &id);

        /**
         * Removes all IDs from the admin field.
         */
        virtual void clear_admins(void);


        /**
         * @return A read-only version of the IDs contained in the list field.
         */
        virtual const SecurityIds &get_list_ids(void) const;

        /**
         * Determines if the provided ID is in the list field.
         * @param[in] id The ID to check.
         * @return True if id is in list field.
         */
        virtual bool is_in_list(const Id &id) const;

        /**
         * Adds an ID to the list field.  Please note if the ID is in the admin
         * list, it cannot be added here.
         * @param[in] id The ID to add.
         * @return True if ID was added, false if not due to being a duplicate
         * or in admin list.
         */
        virtual bool add_to_list(const Id &id);

        /**
         * Removes an ID from the list field.
         * @param id[in] The ID to remove.
         * @return True if the ID was found and removed, false if ID was not
         * found.
         */
        virtual bool remove_from_list(const Id &id);

        /**
         * Removes all IDs from the list field.
         */
        virtual void clear_list(void);


        /**
         * @return Approximate memory used by this class instance, in bytes.
         */
        inline size_t mem_used(void) const
        {
            size_t memory =
                sizeof(Security) // Overall class
                  + (sizeof(SecurityFlagContainer) * 3) // High level container
                  + sizeof(SecurityIds) // High level IDs
                  + 3 // user, list, other as compressed bools
                  + admin_ids.size()  // Number of IDs in admin list
                  + list_ids.size();  // Number of IDs in list

            // Add up the individual ID sizes
            for (SecurityIds::const_iterator admin_id_iter = admin_ids.begin();
                    admin_id_iter != admin_ids.end();
                 ++admin_id_iter)
            {
                memory += admin_id_iter->mem_used();
            }

            // Add up the individual ID sizes
            for (SecurityIds::const_iterator id_iter = list_ids.begin();
                 id_iter != list_ids.end();
                 ++id_iter)
            {
                memory += id_iter->mem_used();
            }

            return memory;
        }

    protected:
        /**
         * Determines if a flag is allowed to be used.  This method is
         * overridden by subclasses, generally.
         * @param flag[in] The flag to check.
         * @return True if the flag can be used.
         */
        virtual bool allow_flag(const SecurityFlag flag) const;

        /**
         * Provides a textual representation of this Security instance,
         * suitable for display.  Error checking is not performed as this
         * is for internal use only.
         * @param flag_count[in] How many flag fields to display.
         * @param output_string[out] This instance in string form.
         */
        virtual void to_string_internal(
            const unsigned int flag_count,
            std::string &output_string) const;

        /**
         * Performs a diff of the two ID sets, putting the differences into
         * diffs.
         * @param lhs[in] The left hand side IDs of the diff.
         * @param rhs[in] The right hand side IDs of the diff.
         * @param diffs[out] Adds any difference information to this.
         */
        void diff_id_set(
            const SecurityIds &lhs,
            const SecurityIds &rhs,
            RemoveAddPair &result) const;

    private:
        /** Container for security flags */
        typedef std::bitset<SECURITYFLAG_invalid> SecurityFlagContainer;

        /**
         * Accounts for possible out of bounds issues when getting a flag.
         * @param[in] container The container to get the flag from.
         * @param[in] flag The flag to get.
         * @return The flag value, or false if out of bounds.
         */
        bool secure_get_flag(
            const SecurityFlagContainer &container,
            const SecurityFlag flag) const;

        /**
         * Accounts for possible out of bounds issues when setting a flag.
         * @param[in,out] container The container to set the flag on.
         * @param[in] flag The flag to set.
         * @param[in] value The value of the flag to set.
         * @return True if success.
         */
        bool secure_set_flag(
            SecurityFlagContainer &container,
            const SecurityFlag flag,
            const bool value);

        // Serialization using Boost Serialization
        //
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & list_flags;
            ar & other_flags;
            ar & admin_ids;
            ar & list_ids;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & list_flags;
            ar & other_flags;
            ar & admin_ids;
            ar & list_ids;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

        SecurityFlagContainer list_flags;  ///< Flags for list of IDs
        SecurityFlagContainer other_flags; ///< Flags for others
        SecurityIds admin_ids;             ///< Has all IDs that are admins
        SecurityIds list_ids;              ///< Has all IDs in 'list' field.
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_SECURITY_H_ */
