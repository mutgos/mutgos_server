/*
 * dbtype_PropertyDirectory.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYDIRECTORY_H_
#define MUTGOS_DBTYPE_PROPERTYDIRECTORY_H_

#include <map>
#include <string>
#include <stddef.h>

#include "logging/log_Logger.h"
#include "osinterface/osinterface_OsTypes.h"
#include "dbtypes/dbtype_PropertyDataSerializer.h"

#include "dbtypes/dbtype_PropertyData.h"

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
     * Represents a property directory, which is composed of entries that
     * can either be leaf entry and/or another directory.  While a leaf item
     * entry must have a PropertyData associated with it, for the directory
     * entry it is optional.
     *
     * The getters are NOT const because they have a 'last used' cache to
     * make repeated lookups faster.  This means there can only be one
     * user of the PropertyDirectory (and children) at one time.  This is an
     * obvious pain point and will need to be resolved in the future.
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
     */
    class PropertyDirectory
    {
    public:
        // TODO Go back and make everyone use this
        // Represents a directory path
        typedef std::string PathString;

        /**
         * Creates an empty property directory.
         */
        PropertyDirectory();

        /**
         * Copy constructor.
         * @param rhs The PropertyDirectory to clone.
         */
        PropertyDirectory(const PropertyDirectory &rhs);

        ~PropertyDirectory();

        /**
         * Assignment operator.
         * @param rhs Source to copy.
         * @return Itself.
         */
        PropertyDirectory &operator=(const PropertyDirectory &rhs);

        /**
         * Clones this property directory, and all subdirectories.
         * @return A pointer to the cloned PropertyDirectory.  Caller must
         * manage this pointer!
         */
        PropertyDirectory *clone(void) const;

        /**
         * Equals operator.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        bool operator==(const PropertyDirectory &rhs) const;

        /**
         * Not equals operator, for convenience.
         * This just calls operator== and inverts the result.
         * Do not use this call as part of an operator== implementation because
         * of this!
         * @param rhs The source to compare.
         * @return True if contents are not equal.
         */
        bool operator!=(const PropertyDirectory &rhs) const;

        /**
         * This call has a length limit.  If the size of the resulting string
         * is too big, it will truncate the listings.
         * @return A recursive listing starting from this PropertyDirectory
         * instance.  The data fields are shown in 'short' form.
         */
        std::string to_string(void) const;

        /**
         * Uses the provided path to get the data of a property.  Not all
         * properties have data.  Do not keep this pointer; other calls
         * may delete it.
         * @param path[in] The property path to retrieve.
         * @return The data for a given property, or null if the property
         * does not have any data or not found.
         */
        PropertyData *get_property_data(const std::string &path);

        /**
         * Uses the provided path to get the actual property directory entry.
         * Do not keep this pointer; other calls may delete it.
         * @param path[in] The property path to retrieve.
         * @return The property directory entry, or null if not found or not a
         * directory.
         */
        PropertyDirectory *get_property_directory(const std::string &path);

        /**
         * Returns the full path for the next entry in the deepest directory,
         * or empty string if not found or at the end.  This allows "walking"
         * a directory.
         * @param path[in] The full path to iterate with.
         * @return The full path to the next entry.
         */
        std::string get_next_property(const std::string &path);

        /**
         * Returns the full path for the previous entry in the deepest
         * directory, or empty string if not found or at the beginning.  This
         * allows "walking" a directory.
         * @param path[in] The full path to iterate with.
         * @return The full path to the previous entry.
         */
        std::string get_previous_property(const std::string &path);

        /**
         * Returns the first property in the given directory.
         * @param path[in] The full path of the property directory to get the
         * first entry of.
         * @return The full path of the first property entry in the directory
         * given by path.  If there are no entries or the path is invalid,
         * an empty string is returned.
         */
        std::string get_first_property(const std::string &path);

        /**
         * Returns the last property in the given directory.
         * @param path[in] The full path of the property directory to get the
         * last entry of.
         * @return The full path of the last property entry in the directory
         * given by path.  If there are no entries or the path is invalid,
         * an empty string is returned.
         */
        std::string get_last_property(const std::string &path);

        /**
         * Deletes the data associated with a property entry.  If the property
         * is NOT a directory, the entire property entry will be removed.
         * @param path[in] The property path whose data is to be deleted.
         */
        void delete_property_data(const std::string &path);

        /**
         * Deletes the property entry.  If the property is a directory, all
         * entries beneath it will also be deleted.
         * @param path[in] The property path to be deleted.
         */
        void delete_property(const std::string &path);

        /**
         * Sets the property data at the path provided.  If it doesn't exist,
         * then the appropriate property directories will be created along
         * the way.  The data will be cloned.  If data already exists at the
         * location, it will be deleted first.
         * @param path[in] The path to set the property data.
         * @param data[in] The data to set.
         * @return True if success, false if error.
         */
        bool set_property(const std::string &path, const PropertyData &data);

        /**
         * @param path[in] The property path to check.
         * @return True if the property exists (may or may not be a directory),
         * false if property does not exist.
         */
        bool does_property_exist(const std::string &path);

        /**
         * @param path[in] The property path to check.
         * @return True if the property exists and is a directory,
         * false otherwise.
         */
        bool is_property_directory(const std::string &path);

        /**
         * Remove everything in this property directory.
         */
        void clear(void);

        /**
         * @return The approximate amount of memory used by this
         * PropertyDirectory, including data and subdirectories.
         */
        size_t mem_used(void) const;

    private:
        /** Null indicates the entry does not contain that type of item */
        typedef std::pair<PropertyData *, PropertyDirectory *> DirectoryEntry;

        typedef std::map<std::string, DirectoryEntry> PropertyDirectoryMap;
        typedef std::list<PropertyDirectory *> DirectoryPath;

        /**
         * Container class for use while iterating depth-first through
         * all properties when doing a to_string().
         */
        class ToStringPosition
        {
        public:
          inline ToStringPosition(
            const std::string &prefix,
            const PropertyDirectoryMap::const_iterator &iter,
            const PropertyDirectoryMap *iter_map_ptr);

          std::string path_prefix; ///< Path prefix for this depth
          PropertyDirectoryMap::const_iterator path_iter; ///< Iter for entries
          const PropertyDirectoryMap *dir_ptr; ///< Directory map iter is for
        };

        /**
         * Given a name, get the directory entry.  From there, the data or
         * another propdir can be accessed.  This method will make use
         * of the cache.
         * @param name[in] The name of the entry to get.
         * @param create[in] If true, create the entry if not found.
         * @return A pointer to the directory entry, or null if not found.
         */
        DirectoryEntry *get_directory_entry(
            const std::string &name,
            const bool create);

        /**
         * Given a directory path (such as "path/to/prop", traverse
         * the path and return the DirectoryEntry that corresponds to the end
         * of the path.
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
        DirectoryEntry *parse_directory_path(
            const std::string &path,
            const bool create,
            DirectoryPath *path_ptr = 0);

        /**
         * Gets the first or last (edges) property entry in the given directory.
         * @param path[in] The full path to check.
         * @param last[in] If true, gets the last entry.  If false, gets the
         * first.
         * @param edge_path[out] The full path of the desired entry.
         */
        void get_property_edge(
            const std::string &path,
            const bool last,
            std::string &edge_path);

        /**
         * Used during serialization to determine which parts of the pair are
         * to be restored.
         */
        enum DirectoryContents
        {
            DIR_NONE,
            DIR_DATA,
            DIR_PROPDIR,
            DIR_DATA_PROPDIR
        };

        PropertyDirectoryMap property_map; ///< The properties in this dir
        const std::string *last_accessed_name_ptr;  ///< Ptr to last accessed key
        DirectoryEntry *last_accessed_entry_ptr; ///< Ptr to last accessed val

        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            // First save off how many items exist
            //
            const MG_UnsignedInt propsize = property_map.size();

            ar & propsize;

            // If anything exists, save it, dereferencing pointers as needed.
            //
            DirectoryContents contents_type = DIR_NONE;

            for (PropertyDirectoryMap::const_iterator
                 save_iter = property_map.begin();
                save_iter != property_map.end();
                ++save_iter)
            {
                // Save the name
                ar & save_iter->first;

                if (save_iter->second.first and save_iter->second.second)
                {
                    // Has everything
                    //
                    contents_type = DIR_DATA_PROPDIR;
                    ar & contents_type;

                    PropertyDataSerializer::save(
                        save_iter->second.first,
                        ar,
                        version);

                    ar & (*(save_iter->second.second));
                }
                else if (save_iter->second.first)
                {
                    // Just has property data
                    //
                    contents_type = DIR_DATA;
                    ar & contents_type;

                    PropertyDataSerializer::save(
                        save_iter->second.first,
                        ar,
                        version);
                }
                else if (save_iter->second.second)
                {
                    // Just has propdirs
                    //
                    contents_type = DIR_PROPDIR;
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
            MG_UnsignedInt propsize = 0;

            ar & propsize;

            // Then restore the entries
            //
            if (propsize)
            {
                DirectoryContents contents_type = DIR_NONE;
                std::string prop_name;
                PropertyDirectory *propdir_ptr = 0;
                PropertyData *data_ptr = 0;

                for (MG_UnsignedInt index = 0; index < propsize; ++index)
                {
                    ar & prop_name;
                    ar & contents_type;

                    propdir_ptr = 0;
                    data_ptr = 0;

                    switch (contents_type)
                    {
                        case DIR_DATA:
                        {
                            data_ptr =
                                PropertyDataSerializer::load(ar, version);
                            break;
                        }

                        case DIR_PROPDIR:
                        {
                            propdir_ptr = new PropertyDirectory();
                            ar & (*propdir_ptr);
                            break;
                        }

                        case DIR_DATA_PROPDIR:
                        {
                            data_ptr =
                                PropertyDataSerializer::load(ar, version);
                            propdir_ptr = new PropertyDirectory();
                            ar & (*propdir_ptr);
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
                                "for prop " + prop_name +
                                ".  A crash is likely to follow.");
                        }
                    }

                    // Add the deserialized entry to the map.
                    //
                    property_map[prop_name] =
                        std::make_pair(data_ptr, propdir_ptr);
                }
            }
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ///
    };

    // ----------------------------------------------------------------------
    PropertyDirectory::ToStringPosition::ToStringPosition(
        const std::string &prefix,
        const PropertyDirectoryMap::const_iterator &iter,
        const PropertyDirectoryMap *iter_map_ptr)
      : path_prefix(prefix) ,
        path_iter(iter),
        dir_ptr(iter_map_ptr)
    {
    }
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_PROPERTYDIRECTORY_H_ */
