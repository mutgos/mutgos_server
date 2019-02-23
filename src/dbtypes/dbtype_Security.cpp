/*
 * dbtype_Security.cpp
 */

#include <string>
#include <ostream>
#include <sstream>
#include <algorithm>

#include <bitset>
#include <vector>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Security.h"

#include "logging/log_Logger.h"

namespace
{
    const static std::string SECURITY_FLAGS_STRING[] =
        {
            "r",
            "w",
            "b",
            "c",
            "?"
        };

    const static std::string SECURITY_FLAGS_LONG_STRING[] =
        {
            "read",
            "write",
            "base",
            "chown",
            "invalid"
        };
}

namespace mutgos
{
namespace dbtype
{
    // -----------------------------------------------------------------------
    SecurityFlag Security::security_flag_from_string(const std::string &flag)
    {
        SecurityFlag result = SECURITYFLAG_invalid;

        for (MG_UnsignedInt index = SECURITYFLAG_read; index < SECURITYFLAG_invalid;
             ++index)
        {
            if ((SECURITY_FLAGS_STRING[index] == flag) or
                SECURITY_FLAGS_LONG_STRING[index] == flag)
            {
                result = (SecurityFlag) index;
                break;
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    Security::Security(void)
      : list_flags(SECURITYFLAG_invalid),
        other_flags(SECURITYFLAG_invalid)
    {
    }

    // -----------------------------------------------------------------------
    Security::Security(const Security &rhs)
      : list_flags(rhs.list_flags),
        other_flags(rhs.other_flags),
        admin_ids(rhs.admin_ids),
        list_ids(rhs.list_ids)
    {
    }

    // -----------------------------------------------------------------------
    Security::~Security()
    {
        // Nothing extra to destruct
    }

    // -----------------------------------------------------------------------
    Security &Security::operator=(const Security &rhs)
    {
        if (&rhs != this)
        {
            list_flags = rhs.list_flags;
            other_flags = rhs.other_flags;
            admin_ids = rhs.admin_ids;
            list_ids = rhs.list_ids;
        }

        return *this;
    }

    // -----------------------------------------------------------------------
    bool Security::operator==(const Security &rhs) const
    {
        if (&rhs == this)
        {
            return true;
        }
        else
        {
            bool lists_equal = true;

            lists_equal = (admin_ids.size() == rhs.admin_ids.size()) and
                    (list_ids.size() == rhs.list_ids.size());

            lists_equal = lists_equal and std::equal(
                admin_ids.begin(), admin_ids.end(), rhs.admin_ids.begin());

            lists_equal = lists_equal and std::equal(
                list_ids.begin(), list_ids.end(), rhs.list_ids.begin());

            return lists_equal and (list_flags == rhs.list_flags) and
                    (other_flags == rhs.other_flags);
        }
    }

    // -----------------------------------------------------------------------
    Security::RemoveAddPair Security::diff_ids(const Security &rhs) const
    {
        RemoveAddPair result;

        // Do the diff, checking each ID set in turn.

        diff_id_set(admin_ids, rhs.admin_ids, result);
        diff_id_set(list_ids, rhs.list_ids, result);

        // This can cause an ID to be in the both add and remove list.  Remove
        // those instances...

        SecurityIds ids_to_remove;

        if ((not result.first.empty()) and (not result.second.empty()))
        {
            SecurityIds::iterator first_iter = result.first.begin();
            SecurityIds::iterator second_iter;
            bool erase = false;

            while (first_iter != result.first.end())
            {
                erase = false;
                second_iter = result.second.begin();

                while (second_iter != result.second.end())
                {
                    if (*first_iter == *second_iter)
                    {
                        erase = true;
                        break;
                    }

                    ++second_iter;
                }

                if (erase)
                {
                    first_iter = result.first.erase(first_iter);
                    result.second.erase(second_iter);
                }
                else
                {
                    ++first_iter;
                }
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    std::string Security::to_string(void) const
    {
        std::string output;

        to_string_internal(list_flags.size(), output);

        return output;
    }

    // -----------------------------------------------------------------------
    bool Security::get_list_security_flag(const SecurityFlag flag) const
    {
        return secure_get_flag(list_flags, flag);
    }

    // -----------------------------------------------------------------------
    bool Security::get_other_security_flag(const SecurityFlag flag) const
    {
        return secure_get_flag(other_flags, flag);
    }

    // -----------------------------------------------------------------------
    bool Security::set_list_security_flag(
        const SecurityFlag flag,
        const bool value)
    {
        return (allow_flag(flag) ?
                secure_set_flag(list_flags, flag, value) : false);
    }

    // -----------------------------------------------------------------------
    bool Security::set_other_security_flag(
        const SecurityFlag flag,
        const bool value)
    {
        return (allow_flag(flag) ?
                secure_set_flag(other_flags, flag, value) : false);
    }

    // -----------------------------------------------------------------------
    const Security::SecurityIds &Security::get_admin_ids(void) const
    {
        return admin_ids;
    }

    // -----------------------------------------------------------------------
    bool Security::is_admin(const Id &id) const
    {
        SecurityIds::const_iterator admin_iter =
            std::find(list_ids.begin(), list_ids.end(), id);

        return (admin_iter != admin_ids.end());
    }

    // -----------------------------------------------------------------------
    bool Security::add_admin(const Id &id)
    {
        if (id.is_default())
        {
            // Default IDs are not allowed to avoid confusion.
            return false;
        }

        // Remove from 'list' if there, since ID shouldn't be both
        // an admin and in the list
        //
        SecurityIds::iterator list_iter =
            std::find(list_ids.begin(), list_ids.end(), id);

        if (list_iter != list_ids.end())
        {
            list_ids.erase(list_iter);
        }

        // Add to admin list if not found
        //
        list_iter = std::find(admin_ids.begin(), admin_ids.end(), id);

        if (list_iter == admin_ids.end())
        {
            admin_ids.push_back(id);
            return true;
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Security::remove_admin(const Id &id)
    {
        SecurityIds::iterator admin_iter =
            std::find(admin_ids.begin(), admin_ids.end(), id);

        if (admin_iter != admin_ids.end())
        {
            if (admin_ids.size() == 1)
            {
                // Nothing left.
                admin_ids.clear();
            }
            else if (*admin_iter == admin_ids.back())
            {
                // Shortcut for removing last item
                admin_ids.pop_back();
            }
            else
            {
                // Put the last one in place of what's being erased,
                // then pop the last one.
                //
                *admin_iter = admin_ids.back();
                admin_ids.pop_back();
            }

            return true;
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Security::clear_admins(void)
    {
        admin_ids.clear();
    }

    // -----------------------------------------------------------------------
    const Security::SecurityIds &Security::get_list_ids(void) const
    {
        return list_ids;
    }

    // -----------------------------------------------------------------------
    bool Security::is_in_list(const Id &id) const
    {
        return (std::find(list_ids.begin(), list_ids.end(), id) !=
            list_ids.end());
    }

    // -----------------------------------------------------------------------
    bool Security::add_to_list(const Id &id)
    {
        if (id.is_default())
        {
            // Default IDs are not allowed to avoid confusion.
            return false;
        }

        SecurityIds::iterator admin_iter =
            std::find(admin_ids.begin(), admin_ids.end(), id);

        if (admin_iter == admin_ids.end())
        {
            SecurityIds::iterator list_iter =
                std::find(list_ids.begin(), list_ids.end(), id);

            if (list_iter == list_ids.end())
            {
                list_ids.push_back(id);
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    bool Security::remove_from_list(const Id &id)
    {
        SecurityIds::iterator list_iter =
            std::find(list_ids.begin(), list_ids.end(), id);

        if (list_iter != list_ids.end())
        {
            if (list_ids.size() == 1)
            {
                // Nothing left.
                list_ids.clear();
            }
            else if (*list_iter == list_ids.back())
            {
                // Shortcut for removing last item
                list_ids.pop_back();
            }
            else
            {
                // Put the last one in place of what's being erased,
                // then pop the last one.
                //
                *list_iter = list_ids.back();
                list_ids.pop_back();
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Security::clear_list(void)
    {
        list_ids.clear();
    }

    // -----------------------------------------------------------------------
    bool Security::allow_flag(const SecurityFlag flag) const
    {
        return flag < SECURITYFLAG_invalid;
    }

    // -----------------------------------------------------------------------
    void Security::to_string_internal(
        const MG_UnsignedInt flag_count,
        std::string &output_string) const
    {
        std::ostringstream strstream;

        for (MG_UnsignedInt index = 0; index < flag_count; ++index)
        {
            strstream
              << (list_flags[index] ? SECURITY_FLAGS_STRING[index] : "-");
        }

        strstream << ":";

        for (MG_UnsignedInt index = 0; index < flag_count; ++index)
        {
            strstream
              << (other_flags[index] ? SECURITY_FLAGS_STRING[index] : "-");
        }

        if (not admin_ids.empty())
        {
            bool first = true;

            // Also spit out the list of space separated admin IDs
            //
            strstream << ":(ADMIN_FIELD ";

            for (SecurityIds::const_iterator iter = admin_ids.begin();
                 iter != admin_ids.end();
                 ++iter)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    strstream << " ";
                }

                strstream << iter->to_string();
            }

            strstream << ")";
        }

        if (not list_ids.empty())
        {
            bool first = true;

            // Also spit out the list of space separated group IDs
            //
            strstream << ":(LIST_FIELD ";

            for (SecurityIds::const_iterator iter = list_ids.begin();
                 iter != list_ids.end();
                 ++iter)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    strstream << " ";
                }

                strstream << iter->to_string();
            }

            strstream << ")";
        }

        output_string = strstream.str();
    }

    // -----------------------------------------------------------------------
    void Security::diff_id_set(
        const SecurityIds &lhs,
        const SecurityIds &rhs,
        RemoveAddPair &result) const
    {
        // Do the diff, checking each list_id in turn.

        for (SecurityIds::const_iterator from_iter = lhs.begin();
             from_iter != lhs.end();
             ++from_iter)
        {
            if (std::find(rhs.begin(), rhs.end(), *from_iter) != rhs.end())
            {
                // Not in other, so it was removed
                result.first.push_back(*from_iter);
            }
        }

        for (SecurityIds::const_iterator to_iter = rhs.begin();
             to_iter != rhs.end();
             ++to_iter)
        {
            if (std::find(lhs.begin(), lhs.end(), *to_iter) != lhs.end())
            {
                // Not in original, so it was added
                result.second.push_back(*to_iter);
            }
        }
    }

    // -----------------------------------------------------------------------
    bool Security::secure_get_flag(
        const Security::SecurityFlagContainer &container,
        const SecurityFlag flag) const
    {
        if ((flag < 0) or (flag >= SECURITYFLAG_invalid))
        {
            LOG(error, "dbtype", "secure_get_flag",
                "Flag to get is not valid!");

            return false;
        }
        else
        {
            return container[flag];
        }
    }

    // -----------------------------------------------------------------------
    bool Security::secure_set_flag(
        Security::SecurityFlagContainer &container,
        const SecurityFlag flag,
        const bool value)
    {
        if ((flag < 0) or (flag >= SECURITYFLAG_invalid))
        {
            LOG(error, "dbtype", "secure_set_flag",
                "Flag to set is not valid!");

            return false;
        }
        else
        {
            container[flag] = value;
            return true;
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
