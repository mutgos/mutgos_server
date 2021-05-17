/*
 * dbtype_RegistrationDirectory.cpp
 */

#include <map>
#include <string>
#include <stddef.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "logging/log_Logger.h"
#include "osinterface/osinterface_OsTypes.h"
#include "dbtypes/dbtype_Id.h"
#include "text/text_Utf8Tools.h"
#include "utilities/mutgos_config.h"

#include "dbtype_RegistrationDirectory.h"

namespace
{
    // @see RegistrationEntity
    /** Currently this can only be one character */
    static const std::string PATH_SEPARATOR = "/";
    static const std::string LISTING_SEPARATOR = ": ";
    static const MG_UnsignedInt MAX_TO_STRING_BYTES = 1024000;
}

// TODO Add depth limit?

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    RegistrationDirectory::RegistrationDirectory()
        : last_accessed_name_ptr(0),
          last_accessed_entry_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::RegistrationDirectory(
        const RegistrationDirectory &rhs)
        : last_accessed_name_ptr(0),
          last_accessed_entry_ptr(0)
    {
        operator=(rhs);
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::~RegistrationDirectory()
    {
        clear();
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory &RegistrationDirectory::operator=(
        const RegistrationDirectory &rhs)
    {
        if (&rhs != this)
        {
            clear();

            RegistrationDirectoryMap::iterator insert_iter;

            for (RegistrationDirectoryMap::const_iterator
                     copy_iter = rhs.registration_map.begin();
                 copy_iter != rhs.registration_map.end();
                 ++copy_iter)
            {
                insert_iter = registration_map.insert(
                    std::make_pair(copy_iter->first, RegistrationEntry(0, 0)))
                                              .first;

                if (copy_iter->second.first)
                {
                    insert_iter->second.first = new Id(
                        *copy_iter->second.first);
                }
                else if (copy_iter->second.second)
                {
                    insert_iter->second.second = copy_iter->second.second
                                                          ->clone();
                }
            }
        }

        return *this;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory *RegistrationDirectory::clone(void) const
    {
        return new RegistrationDirectory(*this);
    }

    // ----------------------------------------------------------------------
    // TODO Rewrite later so not recursive
    bool
    RegistrationDirectory::operator==(const RegistrationDirectory &rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        if (registration_map.size() != rhs.registration_map.size())
        {
            return false;
        }

        // Exactly the same size, so do a entry-by-entry deep comparison.
        //
        RegistrationDirectoryMap::const_iterator equal_iter =
            registration_map.begin();
        RegistrationDirectoryMap::const_iterator rhs_equal_iter =
            rhs.registration_map.begin();

        for (; (equal_iter != registration_map.end()) and
               (rhs_equal_iter != rhs.registration_map.end());
               ++equal_iter, ++rhs_equal_iter)
        {
            // Entry name
            //
            if (equal_iter->first != rhs_equal_iter->first)
            {
                // Directory entry names are not equal.
                return false;
            }

            // Contents of entry
            //
            if (equal_iter->second.first)
            {
                if (rhs_equal_iter->second.first)
                {
                    if (*(equal_iter->second.first) !=
                        *(rhs_equal_iter->second.first))
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else if (rhs_equal_iter->second.first)
            {
                return false;
            }

            // Contents of subdirectory, if any
            //
            if (equal_iter->second.second)
            {
                if (rhs_equal_iter->second.second)
                {
                    if (*(equal_iter->second.second) !=
                        *(rhs_equal_iter->second.second))
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else if (rhs_equal_iter->second.second)
            {
                return false;
            }
        }

        // Nothing triggered a mismatch, so we must be equal.
        return true;
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::operator!=(
        const RegistrationDirectory &rhs) const
    {
        return not operator==(rhs);
    }

    // ----------------------------------------------------------------------
    std::string RegistrationDirectory::to_string(void) const
    {
        std::ostringstream result;

        std::vector<DepthPosition> dir_stack;

        // Prime the loop to start here.
        //
        dir_stack.push_back(DepthPosition(
            std::string(),
            registration_map.begin(),
            &registration_map));

        while (not dir_stack.empty())
        {
            DepthPosition
                &current_position = dir_stack.back();

            if (current_position.path_iter == current_position.dir_ptr->end())
            {
                // No more entries this deep, so pop it and loop back for the
                // next.
                dir_stack.pop_back();
            }
            else
            {
                // Print entry path and value
                //
                if (current_position.path_iter->second.first)
                {
                    result << "  "
                           << current_position.path_prefix
                           << current_position.path_iter->first
                           << LISTING_SEPARATOR
                           << current_position.path_iter->second.first->
                               to_string(true)
                           << std::endl;
                }

                // If entry is a directory, push back iterator and updated path
                // to back.
                //
                if (current_position.path_iter->second.second and
                    (not current_position.path_iter->second.second->
                        registration_map.empty()))
                {
                    dir_stack.push_back(
                        DepthPosition(
                            current_position.path_prefix +
                            current_position.path_iter->first +
                            PATH_SEPARATOR,
                            current_position.path_iter->second.second->
                                registration_map.begin(),
                            &current_position.path_iter->second.second->
                                registration_map));
                }

                ++current_position.path_iter;

                // If result >= limit, then append '...' at bottom and exit.
                if (result.tellp() >= MAX_TO_STRING_BYTES)
                {
                    result << "..." << std::endl;
                    break;
                }
            }
        }

        return result.str();
    }

    // ----------------------------------------------------------------------
    Id *RegistrationDirectory::get_registered_id(
        const RegistrationDirectory::PathString &path)
    {
        Id *result_ptr = 0;
        RegistrationEntry *entry_ptr = parse_directory_path(path, false);

        if (entry_ptr)
        {
            result_ptr = entry_ptr->first;
        }

        return result_ptr;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory *RegistrationDirectory::get_registration_directory(
        const RegistrationDirectory::PathString &path)
    {
        RegistrationDirectory *result_ptr = 0;
        RegistrationEntry *entry_ptr = parse_directory_path(path, false);

        if (entry_ptr)
        {
            result_ptr = entry_ptr->second;
        }

        return result_ptr;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::PathString
    RegistrationDirectory::get_next_registration_entry(
        const RegistrationDirectory::PathString &path)
    {
        std::string result;
        DirectoryPath search_path;
        RegistrationEntry *entry_ptr =
            parse_directory_path(path, false, &search_path);

        if (entry_ptr)
        {
            // Found something, so find it again in the parent's map, and go
            // forward one to find what's next.  Cache the result in case
            // the caller plans to look at the contents.
            //
            RegistrationDirectory *parent_ptr = search_path.back();
            RegistrationDirectoryMap::iterator parent_iter =
                parent_ptr->registration_map.find(
                    *parent_ptr->last_accessed_name_ptr);

            if (parent_iter != parent_ptr->registration_map.end())
            {
                ++parent_iter;

                if (parent_iter != parent_ptr->registration_map.end())
                {
                    // Not at the end, so cache it and build the return path.
                    //
                    parent_ptr->last_accessed_name_ptr = &(parent_iter->first);
                    parent_ptr->last_accessed_entry_ptr =
                        &(parent_iter->second);

                    // Build the path by using the last accessed cache.
                    //
                    for (DirectoryPath::iterator path_iter =
                        search_path.begin();
                         path_iter != search_path.end();
                         ++path_iter)
                    {
                        result += PATH_SEPARATOR +
                                  *(*path_iter)->last_accessed_name_ptr;
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::PathString
        RegistrationDirectory::get_previous_registration_entry(
        const RegistrationDirectory::PathString &path)
    {
        std::ostringstream result;
        DirectoryPath search_path;
        RegistrationEntry *entry_ptr =
            parse_directory_path(path, false, &search_path);

        if (entry_ptr)
        {
            // Found something, so find it again in the parent's map, and go
            // forward one to find what's next.  Cache the result in case
            // the caller plans to look at the contents.
            //
            RegistrationDirectory *parent_ptr = search_path.back();
            RegistrationDirectoryMap::iterator parent_iter =
                parent_ptr->registration_map.find(
                    *parent_ptr->last_accessed_name_ptr);

            // Make sure the entry was found and not at the beginning.
            // If it's at the beginning, we can't go backwards any further so
            // we can just stop.
            //
            if ((parent_iter != parent_ptr->registration_map.end()) and
                (parent_iter != parent_ptr->registration_map.begin()))
            {
                --parent_iter;

                // Not at the beginning, so cache it and build the return path.
                //
                parent_ptr->last_accessed_name_ptr = &(parent_iter->first);
                parent_ptr->last_accessed_entry_ptr =
                    &(parent_iter->second);

                // Build the path by using the last accessed cache.
                //
                for (DirectoryPath::iterator path_iter = search_path.begin();
                     path_iter != search_path.end();
                     ++path_iter)
                {
                    result << PATH_SEPARATOR
                           << *((*path_iter)->last_accessed_name_ptr);
                }
            }
        }

        return result.str();
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::PathString
        RegistrationDirectory::get_first_registration_entry(
            const RegistrationDirectory::PathString &path)
    {
        std::string result;

        get_registration_edge(path, false, result);

        return result;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::PathString
        RegistrationDirectory::get_last_registration_entry(
            const RegistrationDirectory::PathString &path)
    {
        std::string result;

        get_registration_edge(path, true, result);

        return result;
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::delete_registration(
        const RegistrationDirectory::PathString &path)
    {
        bool result = false;
        DirectoryPath search_path;
        RegistrationEntry *entry_ptr =
            parse_directory_path(path, false, &search_path);

        if (entry_ptr)
        {
            // Delete the registration
            //
            delete entry_ptr->first;
            entry_ptr->first = 0;

            // Delete anything inside the directory, if it is one
            delete entry_ptr->second;
            entry_ptr->second = 0;

            // Remove it from the map and cache.
            // A trick here: The registration/directory we need to delete is
            // always the last accessed one in the parent.  So we use that
            // for the registration name.
            //
            RegistrationDirectory *parent_ptr = search_path.back();

            if (not parent_ptr->last_accessed_name_ptr)
            {
                LOG(fatal, "dbtype", "delete_registration",
                    "Cache is null!  Cannot delete " + path);
            }
            else
            {
                parent_ptr->registration_map.erase(
                    *(parent_ptr->last_accessed_name_ptr));
                parent_ptr->last_accessed_name_ptr = 0;
                parent_ptr->last_accessed_entry_ptr = 0;
            }

            result = true;
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::add_registration(
        const RegistrationDirectory::PathString &path,
        const Id &id)
    {
        RegistrationEntry *entry_ptr = parse_directory_path(path, true);

        if (entry_ptr)
        {
            if (entry_ptr->first)
            {
                // Delete what's currently there.
                //
                delete entry_ptr->first;
                entry_ptr->first = 0;
            }

            // Copy the new value in.
            //
            entry_ptr->first = new Id(id);
        }

        return entry_ptr;
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::does_registration_exist(
        const RegistrationDirectory::PathString &path)
    {
        return parse_directory_path(path, false);
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::is_path_directory(
        const RegistrationDirectory::PathString &path)
    {
        RegistrationEntry *entry_ptr = parse_directory_path(path, false);

        return (entry_ptr ? entry_ptr->second : 0);
    }

    // ----------------------------------------------------------------------
    bool RegistrationDirectory::is_empty(void) const
    {
        return registration_map.empty();
    }

    // ----------------------------------------------------------------------
    // TODO Rewrite later so not recursive
    void RegistrationDirectory::clear(void)
    {
        for (RegistrationDirectoryMap::iterator delete_iter =
                registration_map.begin();
             delete_iter != registration_map.end();
             ++delete_iter)
        {
            delete delete_iter->second.first;
            delete delete_iter->second.second;
            delete_iter->second.first = 0;
            delete_iter->second.second = 0;
        }

        registration_map.clear();
        last_accessed_entry_ptr = 0;
        last_accessed_name_ptr = 0;
    }

    // ----------------------------------------------------------------------
    // TODO Rewrite later so not recursive
    size_t RegistrationDirectory::mem_used(void) const
    {
        size_t memory_used = registration_map.size();

        for (RegistrationDirectoryMap::const_iterator
                 mem_iter = registration_map.begin();
             mem_iter != registration_map.end();
             ++mem_iter)
        {
            memory_used += mem_iter->first.size();
            memory_used += sizeof(*mem_iter);

            if (mem_iter->second.first)
            {
                memory_used += mem_iter->second.first->mem_used();
            }

            if (mem_iter->second.second)
            {
                memory_used += mem_iter->second.second->mem_used();
            }
        }

        return memory_used;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::RegistrationEntry *RegistrationDirectory::get_directory_entry(
        const RegistrationDirectory::PathString &name,
        const bool create)
    {
        if (last_accessed_name_ptr)
        {
            // Try the cache first.
            //
            if ((*last_accessed_name_ptr) == name)
            {
                // Found in cache.
                return last_accessed_entry_ptr;
            }
        }

        RegistrationDirectoryMap::iterator reg_iter =
            registration_map.find(name);

        if (reg_iter == registration_map.end())
        {
            // Not found.  See if we need to create it.
            if (not create)
            {
                return 0;
            }
            else
            {
                // Create the entry, cache it, and return.
                reg_iter = registration_map.insert(
                    std::make_pair(name, RegistrationEntry(0,0))).first;
                last_accessed_name_ptr = &(reg_iter->first);
                last_accessed_entry_ptr = &(reg_iter->second);

                return last_accessed_entry_ptr;
            }
        }
        else
        {
            // Found something.  Cache it and return.
            //
            last_accessed_name_ptr = &(reg_iter->first);
            last_accessed_entry_ptr = &(reg_iter->second);

            return last_accessed_entry_ptr;
        }

        return 0;
    }

    // ----------------------------------------------------------------------
    RegistrationDirectory::RegistrationEntry *RegistrationDirectory::parse_directory_path(
        const RegistrationDirectory::PathString &path,
        const bool create,
        RegistrationDirectory::DirectoryPath *path_ptr)
    {
        RegistrationEntry *current_entry_ptr = 0;
        RegistrationDirectory *current_regdir_ptr = this;

        std::string trimmed_path = boost::trim_copy(path);

        if (trimmed_path.empty())
        {
            // Empty paths are not valid.
            return 0;
        }

        // Remove any prefixed separators, since they are not needed.
        //
        const size_t trim_index =
            trimmed_path.find_first_not_of(PATH_SEPARATOR);

        if (trim_index and (trim_index != std::string::npos))
        {
            // Get rid of prefix separator since it's not needed.
            trimmed_path = (trimmed_path.size() == (trim_index + 1)) ?
                "" : trimmed_path.substr(trim_index + 1);
        }

        boost::char_separator<char> sep(PATH_SEPARATOR.c_str());
        boost::tokenizer<boost::char_separator<char> >
            tokens(trimmed_path, sep);

        // Check sizes
        //
        if (create)
        {
            for (boost::tokenizer<boost::char_separator<char> >::iterator
                     tok_iter = tokens.begin();
                 tok_iter != tokens.end(); ++tok_iter)
            {
                if (text::utf8_size(*tok_iter) > config::db::limits_property_name())
                {
                    // Too long. Abort.
                    return 0;
                }
            }
        }

        // Go through the path one segment at a time, traversing the
        // registration directories until either the end is found, or a
        // segment cannot be located.
        //
        for (boost::tokenizer<boost::char_separator<char> >::iterator
                 tok_iter = tokens.begin();
             tok_iter != tokens.end(); ++tok_iter)
        {
            // Skip empty tokens.  This might happen if there are multiple
            // separators in a row, or if it ends in a separator.
            //
            if (not (*tok_iter).empty())
            {
                if (not current_regdir_ptr)
                {
                    current_entry_ptr = 0;
                    break;
                }

                if (path_ptr)
                {
                    path_ptr->push_back(current_regdir_ptr);
                }

                current_entry_ptr = current_regdir_ptr->get_directory_entry(
                    *tok_iter, create);

                if (not current_entry_ptr)
                {
                    // Couldn't find a segment.
                    break;
                }
                else
                {
                    current_regdir_ptr = current_entry_ptr->second;
                }
            }
        }

        if (path_ptr and (not current_entry_ptr))
        {
            // Nothing found, so leave the path empty.
            path_ptr->clear();
        }

        return current_entry_ptr;
    }

    // ----------------------------------------------------------------------
    void RegistrationDirectory::get_registration_edge(
        const RegistrationDirectory::PathString &path,
        const bool last,
        RegistrationDirectory::PathString &edge_path)
    {
        std::string trimmed_path = boost::trim_copy(path);

        edge_path.clear();

        if (not trimmed_path.empty())
        {
            bool add_separator = true;

            // If they already have a slash at the end, no need to add another
            // one
            //
            if (trimmed_path[trimmed_path.size() - 1] == PATH_SEPARATOR[0])
            {
                add_separator = false;
            }

            // Parse the path, then simply append the first entry at the end.
            //
            RegistrationEntry *entry_ptr = parse_directory_path(path, false);

            if (entry_ptr and entry_ptr->second and
                (not entry_ptr->second->registration_map.empty()))
            {
                edge_path = trimmed_path;

                if (add_separator)
                {
                    edge_path += PATH_SEPARATOR;
                }

                edge_path += (last ?
                    entry_ptr->second->registration_map.rbegin()->first :
                    entry_ptr->second->registration_map.begin()->first);
            }
        }
    }
}
}
