/*
 * dbtype_id.h
 */

#ifndef MUTGOS_DBTYPE_ID_H_
#define MUTGOS_DBTYPE_ID_H_

#include <string>
#include <vector>
#include <set>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"
#include "utilities/json_JsonUtilities.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

namespace mutgos
{
namespace dbtype
{
    // TODO Need some way to easily detect if either site or entity are default.

    /**
     * Represents an entity ID, the fundamental way to refer to anything
     * in the game database.
     */
    class Id
    {
        // Most of this is inlined for performance reasons
    public:
        /** The type for a Site ID */
        typedef osinterface::OsTypes::UnsignedInt SiteIdType;
        /** The type for an Entity ID */
        typedef osinterface::OsTypes::VeryLongUnsignedInt EntityIdType;
        /** A vector of site IDs */
        typedef std::vector<SiteIdType> SiteIdVector;
        /** Set of site IDs */
        typedef std::set<SiteIdType> SiteIdSet;

        /**
         * Constructs an id.
         * @param[in] site The site ID to use.
         * @param[in] entity The entity ID to use.
         */
        inline Id(const SiteIdType &site, const EntityIdType &entity)
          : site_id(site),
            entity_id(entity)
        {
        }

        /**
         * Copy constructor.
         * @param[in] rhs The source to copy from.
         */
        inline Id(const Id &rhs)
            : site_id(rhs.site_id),
              entity_id(rhs.entity_id)
        {
        }

        /**
         * Deserialization/Default constructor.  The Id is considered invalid
         * or nonexistant.
         */
        inline Id()
          : site_id(0),
            entity_id(0)
        {
        }

        /**
         * Standard destructor.
         */
        ~Id()
        {
            // Nothing to do here yet
        }

        /**
         * @param include_site[in] If true, the site ID is included in the
         * string.
         * @return This ID as a string.
         */
        std::string to_string(const bool include_site = true) const;

        /**
         * Assignment operator.
         * @param[in] rhs The source to copy from.
         * @return The copy.
         */
        inline Id &operator=(const Id &rhs)
        {
            if (&rhs != this)
            {
                site_id = rhs.site_id;
                entity_id = rhs.entity_id;
            }

            return *this;
        }

        /**
         * Compares against another Id.
         * @param[in] rhs The Id to compare against.
         * @return True if the two instances are the same.
         */
        inline bool operator==(const Id &rhs) const
        {
            return (site_id == rhs.site_id) and (entity_id == rhs.entity_id);
        }

        /**
         * Compares against another Id.
         * @param[in] rhs The Id to compare against.
         * @return True if the two instances are not the same.
         */
        inline bool operator!=(const Id &rhs) const
        {
            return not operator==(rhs);
        }

        /**
         * Compares against another Id.
         * @param[in] rhs The Id to compare against.
         * @return True if this ID is less than rhs.
         */
        inline bool operator<(const Id &rhs) const
        {
            if (site_id < rhs.site_id)
            {
                return true;
            }
            else if (site_id > rhs.site_id)
            {
                return false;
            }
            else
            {
                return entity_id < rhs.entity_id;
            }
        }

        /**
         * @return The site ID.
         */
        inline SiteIdType get_site_id(void) const
        {
            return site_id;
        }

        /**
         * @return True if the site ID is defaulted (invalid).
         */
        inline bool is_site_default(void) const
        {
            return (site_id == 0);
        }

        /**
         * @return The entity ID.
         */
        inline EntityIdType get_entity_id(void) const
        {
            return entity_id;
        }

        /**
         * @return True if the entity ID is defaulted (invalid).
         */
        inline bool is_entity_default(void) const
        {
            return (entity_id == 0);
        }

        /**
         * @return True if Id was made with the default (invalid) constructor.
         */
        inline bool is_default(void) const
        {
            return (site_id == 0) and (entity_id == 0);
        }

        /**
         * @return Approximate memory used by this class instance, in bytes.
         */
        inline size_t mem_used(void) const
        {
            return sizeof(Id);
        }

        /**
         * Saves this to the provided JSON node.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        bool restore(const json::JSONNode &node);

    private:
        // Serialization using Boost Serialization
        //
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & site_id;
            ar & entity_id;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & site_id;
            ar & entity_id;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////

        SiteIdType site_id;     ///< The Site ID
        EntityIdType entity_id; ///< The Entity ID
    };

} /* namespace dbtype */
} /* namespace mutgos */

#endif /* DBTYPE_ID_H_ */
