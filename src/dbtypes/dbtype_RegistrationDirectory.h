/*
 * dbtype_RegistrationDirectory.h
 */

#ifndef MUTGOS_DBTYPE_REGISTRATIONDIRECTORY_H_
#define MUTGOS_DBTYPE_REGISTRATIONDIRECTORY_H_

#include <map>
#include <string>
#include <stddef.h>

#include "logging/log_Logger.h"
#include "osinterface/osinterface_OsTypes.h"
#include "dbtypes/dbtype_Id.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include "text/text_StringConversion.h"

namespace mutgos
{
namespace dbtype
{
    /**
     * Represents a registration directory, which is composed of entries that
     * can either be leaf entry or another directory.  A leaf item must
     * have an ID associated with it.
     *
     * The getters are NOT const because they have a 'last used' cache to
     * make repeated lookups faster.  This means there can only be one
     * user of the RegistrationDirectory (and children) at one time.  This
     * is an obvious pain point and will need to be resolved in the future.
     *
     * Directories are traversed using the '/' character, much like a
     * filesystem.
     *
     * This class is not thread safe and cannot be subclassed.
     *
     * Directories do not keep a pointer to their parent to save space.  The
     * parent can be calculated as needed.
     *
     * This class is not designed to be inherited from or overridden.
     *
     * This class is very similar to PropertyDirectory, but was
     * reimplemented so it could be optimized for registrations, and
     * so that future behavior changes won't impact both properties
     * and registrations, which may not want every behavior shared.
     */
    class RegistrationDirectory
    {
    public:
        // Represents a directory path
        typedef std::string PathString;

        /**
         * Creates an empty registration directory.
         */
        RegistrationDirectory();

        /**
         * Copy constructor.
         * @param rhs The RegistrationDirectory to clone.
         */
        RegistrationDirectory(const RegistrationDirectory &rhs);

        /**
         * Destructor.
         */
        ~RegistrationDirectory();

        /**
         * Assignment operator.
         * @param rhs Source to copy.
         * @return Itself.
         */
        RegistrationDirectory &operator=(const RegistrationDirectory &rhs);

        /**
         * Clones this registration directory, and all subdirectories.
         * @return A pointer to the cloned RegistrationDirectory.  Caller must
         * manage this pointer!
         */
        RegistrationDirectory *clone(void) const;

        /**
         * Equals operator.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        bool operator==(const RegistrationDirectory &rhs) const;

        /**
         * Not equals operator, for convenience.
         * This just calls operator== and inverts the result.
         * Do not use this call as part of an operator== implementation because
         * of this!
         * @param rhs The source to compare.
         * @return True if contents are not equal.
         */
        bool operator!=(const RegistrationDirectory &rhs) const;

        /**
         * This call has a length limit.  If the size of the resulting string
         * is too big, it will truncate the listings.
         * @return A recursive listing starting from this RegistrationDirectory
         * instance.  The data fields are shown in 'short' form.
         */
        std::string to_string(void) const;

        /**
         * Uses the provided path to get the ID associated with the
         * registration.
         * @param path[in] The registration path to retrieve.
         * @return The ID pointer for a given registration, or null if the
         * registration is a directory or not found.  Do not delete the
         * pointer.
         */
        Id *get_registered_id(const PathString &path);

        /**
         * Uses the provided path to get the actual registration directory
         * entry.
         * Do not keep this pointer; other calls may delete it.
         * @param path[in] The registration path to retrieve.
         * @return The registration directory entry, or null if not found or
         * not a directory.  Do not delete the pointer.
         */
        RegistrationDirectory *get_registration_directory(
            const PathString &path);

        /**
         * Returns the full path for the next entry in the deepest
         * registration directory, or empty string if not found or at the end.
         * This allows "walking" a directory.
         * @param path[in] The full path to iterate with.
         * @return The full path to the next entry, or empty if not found.
         */
        PathString get_next_registration_entry(const PathString &path);

        /**
         * Returns the full path for the previous entry in the deepest
         * registration directory, or empty string if not found or at the
         * beginning.  This allows "walking" a directory.
         * @param path[in] The full path to iterate with.
         * @return The full path to the previous entry, or empty if not found.
         */
        PathString get_previous_registration_entry(const PathString &path);

        /**
         * Returns the first entry in the given registration directory.
         * @param path[in] The full path of the registration directory to get
         * the first entry of.
         * @return The full path of the first registration entry in the
         * directory given by path.  If there are no entries or the path is
         * invalid, an empty string is returned.
         */
        PathString get_first_registration_entry(const PathString &path);

        /**
         * Returns the last entry in the given registration directory.
         * @param path[in] The full path of the registration directory to get
         * the last entry of.
         * @return The full path of the last registration entry in the
         * directory given by path.  If there are no entries or the path is
         * invalid, an empty string is returned.
         */
        PathString get_last_registration_entry(const PathString &path);

        /**
         * Deletes the given registration entry.  If the entry is a directory,
         * everything underneath it will be recursively removed.
         * @param path[in] The path whose registration data is to be deleted.
         * @return True if found and deleted.
         */
        bool delete_registration(const PathString &path);

        /**
         * Adds or updates the registration entry.  If the directories
         * in between do not exist, they will be created.
         * @param path[in] The path to set the registration entry.
         * @param id[in] The ID to be associated with the registration.
         * @return True if success, false if error.
         */
        bool add_registration(const PathString &path, const Id &id);

        /**
         * @param path[in] The registration path to check.
         * @return True if the registration exists (and is not a directory),
         * false if registration does not exist or is a directory.
         */
        bool does_registration_exist(const PathString &path);

        /**
         * @param path[in] The registration path to check.
         * @return True if path is a valid directory, false otherwise.
         */
        bool is_path_directory(const PathString &path);

        /**
         * @return True if there are no registrations.
         */
        bool is_empty(void) const;

        /**
         * Remove everything in this registration directory.
         */
        void clear(void);

        /**
         * @return The approximate amount of memory used by this
         * RegistrationDirectory, including data and subdirectories.
         */
        size_t mem_used(void) const;

    private:
        /** Null indicates the entry does not contain that type of item.
            Only one may be populated. */
        typedef std::pair<Id *, RegistrationDirectory *> RegistrationEntry;

        typedef std::map<std::string, RegistrationEntry> RegistrationDirectoryMap;
        typedef std::list<RegistrationDirectory *> DirectoryPath;

        /**
         * Container class for use while iterating depth-first through
         * all registrations when doing a to_string() (or other similar
         * algorithms).
         */
        class DepthPosition
        {
        public:
          inline DepthPosition(
            const std::string &prefix,
            const RegistrationDirectoryMap::const_iterator &iter,
            const RegistrationDirectoryMap *iter_map_ptr);

          std::string path_prefix; ///< Path prefix for this depth
          RegistrationDirectoryMap::const_iterator path_iter; ///< Iter for entries
          const RegistrationDirectoryMap *dir_ptr; ///< Directory map iter is for
        };

        /**
         * Given a name, get the directory entry.  From there, the registration
         * or another directory can be accessed.  This method will make use
         * of the cache.
         * @param name[in] The name of the entry to get.
         * @param create[in] If true, create the entry if not found.
         * @return A pointer to the entry, or null if not found.
         */
        RegistrationEntry *get_directory_entry(
            const PathString &name,
            const bool create);

        /**
         * Given a directory path (such as "path/to/reg", traverse
         * the path and return the RegistrationEntry that corresponds to the
         * end of the path.
         * @param path[in] The directory path to traverse.
         * @param create[in] If true, create entries if they are not found.
         * @param path_ptr[out] If pointer provided, the path as a series
         * of pointers will be added.  Do not delete pointers!  This
         * will have no data in it if the path is not found.  The
         * first entry is the topmost parent, the last is the parent to
         * what was found.
         * @return The DirectoryEntry that corresponds to the path, or null
         * if any part of the path is not found.
         */
        RegistrationEntry *parse_directory_path(
            const PathString &path,
            const bool create,
            DirectoryPath *path_ptr = 0);

        /**
         * Gets the first or last (edges) registration (or directory) entry
         * in the given directory.
         * @param path[in] The full path to check.
         * @param last[in] If true, gets the last entry.  If false, gets the
         * first.
         * @param edge_path[out] The full path of the desired entry.
         */
        void get_registration_edge(
            const PathString &path,
            const bool last,
            PathString &edge_path);

        /**
         * Used during serialization to determine which part of the pair are
         * to be restored.
         */
        enum DirectoryContents
        {
            DIR_NONE,
            DIR_REGISTRATION,
            DIR_DIRECTORY
        };

        RegistrationDirectoryMap registration_map; ///< The registrations in this dir
        const std::string *last_accessed_name_ptr;  ///< Ptr to last accessed key
        RegistrationEntry *last_accessed_entry_ptr; ///< Ptr to last accessed val

        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            // First save off how many items exist
            //
            const MG_UnsignedInt regsize = registration_map.size();

            ar & regsize;

            // If anything exists, save it, dereferencing pointers as needed.
            //
            DirectoryContents contents_type = DIR_NONE;

            for (RegistrationDirectoryMap::const_iterator
                 save_iter = registration_map.begin();
                save_iter != registration_map.end();
                ++save_iter)
            {
                // Save the name
                ar & save_iter->first;

                if (save_iter->second.first)
                {
                    // Has registration data
                    //
                    contents_type = DIR_REGISTRATION;
                    ar & contents_type;
                    ar & (*(save_iter->second.first));
                }
                else if (save_iter->second.second)
                {
                    // Has directory
                    //
                    contents_type = DIR_DIRECTORY;
                    ar & contents_type;
                    ar & (*(save_iter->second.second));
                }
                else
                {
                    // Has nothing (??)
                    //
                    contents_type = DIR_NONE;
                    ar & contents_type;
                }
            }
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            // Get the size...
            //
            MG_UnsignedInt regsize = 0;

            ar & regsize;

            clear();

            // Then restore the entries
            //
            if (regsize)
            {
                DirectoryContents contents_type = DIR_NONE;
                std::string reg_name;
                Id *id_ptr = 0;
                RegistrationDirectory *regdir_ptr = 0;

                for (MG_UnsignedInt index = 0; index < regsize; ++index)
                {
                    ar & reg_name;
                    ar & contents_type;

                    regdir_ptr = 0;
                    id_ptr = 0;

                    switch (contents_type)
                    {
                        case DIR_REGISTRATION:
                        {
                            id_ptr = new Id();
                            ar & (*id_ptr);
                            break;
                        }

                        case DIR_DIRECTORY:
                        {
                            regdir_ptr = new RegistrationDirectory();
                            ar & (*regdir_ptr);
                            break;
                        }

                        case DIR_NONE:
                        {
                            break;
                        }

                        default:
                        {
                            std::string enum_string = "-1";

                            enum_string = text::to_string(
                                contents_type);

                            LOG(fatal, "dbtype", "load()",
                                "Type is unknown:  " + enum_string +
                                "for reg " + reg_name +
                                ".  A crash is likely to follow.");
                        }
                    }

                    // Add the deserialized entry to the map.
                    //
                    registration_map[reg_name] =
                        std::make_pair(id_ptr, regdir_ptr);
                }
            }
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ///
    };

    // ----------------------------------------------------------------------
    RegistrationDirectory::DepthPosition::DepthPosition(
        const std::string &prefix,
        const RegistrationDirectoryMap::const_iterator &iter,
        const RegistrationDirectoryMap *iter_map_ptr)
      : path_prefix(prefix) ,
        path_iter(iter),
        dir_ptr(iter_map_ptr)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_REGISTRATIONDIRECTORY_H_ */
