/*
 * primitives_NameRegistry.cpp
 */

#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include "primitives/primitives_NameRegistry.h"



#include "logging/log_Logger.h"
#include "text/text_StringConversion.h"

namespace mutgos
{
namespace primitives
{
    // Statics
    //
    NameRegistry *NameRegistry::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    NameRegistry *NameRegistry::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new NameRegistry();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void NameRegistry::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    NameRegistry::ResultVector NameRegistry::search_by_prefix(
        const dbtype::Id::SiteIdType site,
        const std::string &prefix,
        const dbtype::EntityType type)
    {
        ResultVector result;

        if (prefix.empty())
        {
            return result;
        }

        if ((type != dbtype::ENTITYTYPE_entity) and
            (type != dbtype::ENTITYTYPE_player) and
            (type != dbtype::ENTITYTYPE_puppet))
        {
            LOG(error, "primitives", "search_by_prefix",
                "Invalid search type: " + text::to_string(type));
            return result;
        }

        const std::string prefix_lower = text::to_lower_copy(prefix);

        boost::shared_lock<boost::shared_mutex> read_lock(mutex);

        SiteToNamesMap::const_iterator site_iter = names.find(site);

        if (site_iter != names.end())
        {
            if ((type != dbtype::ENTITYTYPE_entity) or
                (type != dbtype::ENTITYTYPE_player))
            {
                // Search players
                //
                for (NameInfoVector::const_iterator info_iter =
                        site_iter->second.first.begin();
                     info_iter != site_iter->second.first.end();
                     ++info_iter)
                {
                    if (boost::algorithm::starts_with(
                        info_iter->normalized_name,
                        prefix_lower))
                    {
                        // Found a match.
                        result.push_back(NameRegistryResult(
                            info_iter->name,
                            info_iter->id,
                            dbtype::ENTITYTYPE_player,
                            prefix.size() == info_iter->name.size()));
                    }
                }
            }

            if ((type != dbtype::ENTITYTYPE_entity) or
                (type != dbtype::ENTITYTYPE_puppet))
            {
                // Search puppets
                //
                for (NameInfoVector::const_iterator info_iter =
                    site_iter->second.second.begin();
                     info_iter != site_iter->second.second.end();
                     ++info_iter)
                {
                    if (boost::algorithm::starts_with(
                        info_iter->normalized_name,
                        prefix_lower))
                    {
                        // Found a match.
                        result.push_back(NameRegistryResult(
                            info_iter->name,
                            info_iter->id,
                            dbtype::ENTITYTYPE_puppet,
                            prefix.size() == info_iter->name.size()));
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    NameRegistry::ResultVector NameRegistry::search_by_exact(
        const dbtype::Id::SiteIdType site,
        const std::string &prefix,
        const dbtype::EntityType type)
    {
        ResultVector result;

        if (prefix.empty())
        {
            return result;
        }

        if ((type != dbtype::ENTITYTYPE_entity) and
            (type != dbtype::ENTITYTYPE_player) and
            (type != dbtype::ENTITYTYPE_puppet))
        {
            LOG(error, "primitives", "search_by_prefix",
                "Invalid search type: " + text::to_string(type));
            return result;
        }

        const std::string prefix_lower = text::to_lower_copy(prefix);

        boost::shared_lock<boost::shared_mutex> read_lock(mutex);

        SiteToNamesMap::const_iterator site_iter = names.find(site);

        if (site_iter != names.end())
        {
            if ((type != dbtype::ENTITYTYPE_entity) or
                (type != dbtype::ENTITYTYPE_player))
            {
                // Search players
                //
                for (NameInfoVector::const_iterator info_iter =
                    site_iter->second.first.begin();
                     info_iter != site_iter->second.first.end();
                     ++info_iter)
                {
                    if (info_iter->normalized_name == prefix_lower)
                    {
                        // Found a match.
                        result.push_back(NameRegistryResult(
                            info_iter->name,
                            info_iter->id,
                            dbtype::ENTITYTYPE_player,
                            prefix.size() == info_iter->name.size()));
                    }
                }
            }

            if ((type != dbtype::ENTITYTYPE_entity) or
                (type != dbtype::ENTITYTYPE_puppet))
            {
                // Search puppets
                //
                for (NameInfoVector::const_iterator info_iter =
                    site_iter->second.second.begin();
                     info_iter != site_iter->second.second.end();
                     ++info_iter)
                {
                    if (info_iter->normalized_name == prefix_lower)
                    {
                        // Found a match.
                        result.push_back(NameRegistryResult(
                            info_iter->name,
                            info_iter->id,
                            dbtype::ENTITYTYPE_puppet,
                            prefix.size() == info_iter->name.size()));
                    }
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool NameRegistry::add_entity(const NameRegistryInfo &entity_info)
    {
        if (entity_info.name.empty() or entity_info.id.is_default() or
            ((entity_info.type != dbtype::ENTITYTYPE_player) and
            (entity_info.type != dbtype::ENTITYTYPE_puppet)))
        {
            LOG(error, "primitives", "add_entity(info)",
                "Not all entity info fields filled out.  Cannot add.");
            return false;
        }

        boost::unique_lock<boost::shared_mutex> write_lock(mutex);

        PlayersPuppetsPair &players_puppets =
            names[entity_info.id.get_site_id()];

        if (entity_info.type == dbtype::ENTITYTYPE_player)
        {
            players_puppets.first.push_back(NameInfo(
                entity_info.id,
                entity_info.name,
                text::to_lower_copy(entity_info.name)));
        }
        else
        {
            // Must be a puppet, then
            players_puppets.second.push_back(NameInfo(
                entity_info.id,
                entity_info.name,
                text::to_lower_copy(entity_info.name)));
        }

        return true;
    }

    // ----------------------------------------------------------------------
    bool NameRegistry::add_entity(const std::vector<NameRegistryInfo> &entities)
    {
        bool result = true;

        if (entities.empty())
        {
            return result;
        }

        dbtype::Id::SiteIdType site_id = 0;
        PlayersPuppetsPair *players_puppets = 0;

        boost::unique_lock<boost::shared_mutex> write_lock(mutex);

        for (std::vector<NameRegistryInfo>::const_iterator iter = entities.begin();
            iter != entities.end();
            ++iter)
        {
            if (iter->name.empty() or iter->id.is_default() or
                ((iter->type != dbtype::ENTITYTYPE_player) and
                 (iter->type != dbtype::ENTITYTYPE_puppet)))
            {
                LOG(error, "primitives", "add_entity(vector)",
                    "Not all entity info fields filled out for entry.  Cannot add.");
                result = false;
                continue;
            }

            if (not site_id)
            {
                site_id = iter->id.get_site_id();
                players_puppets = &names[site_id];
            }
            else if (site_id != iter->id.get_site_id())
            {
                LOG(error, "primitives", "add_entity(vector)",
                    "Site mismatch for ID " + iter->id.to_string(true)
                    + ", expected site " + text::to_string(site_id));
                result = false;
                continue;
            }

            // All good to add.
            //
            if (iter->type == dbtype::ENTITYTYPE_player)
            {
                players_puppets->first.push_back(NameInfo(
                    iter->id,
                    iter->name,
                    text::to_lower_copy(iter->name)));
            }
            else
            {
                // Must be a puppet, then
                players_puppets->second.push_back(NameInfo(
                    iter->id,
                    iter->name,
                    text::to_lower_copy(iter->name)));
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool NameRegistry::update_entity_name(
        const dbtype::Id &id,
        const dbtype::EntityType type,
        const std::string &new_name)
    {
        bool result = false;

        if (new_name.empty() or id.is_default() or
            ((type != dbtype::ENTITYTYPE_player) and
             (type != dbtype::ENTITYTYPE_puppet)))
        {
            LOG(error, "primitives", "update_entity_name()",
                "Not all arguments properly filled out.  Cannot update "
                + id.to_string((true)));
            return result;
        }

        boost::unique_lock<boost::shared_mutex> write_lock(mutex);

        SiteToNamesMap::iterator site_iter = names.find(id.get_site_id());

        if (site_iter != names.end())
        {
            NameInfoVector * const name_infos =
                type == dbtype::ENTITYTYPE_player ? &site_iter->second.first :
                &site_iter->second.second;

            // Search for the entry
            //
            for (NameInfoVector::iterator name_iter = name_infos->begin();
                name_iter != name_infos->end();
                ++name_iter)
            {
                if (name_iter->id == id)
                {
                    // Found it
                    name_iter->name = new_name;
                    name_iter->normalized_name = text::to_lower_copy(new_name);
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool NameRegistry::remove_entity(
        const dbtype::Entity::IdSet &ids,
        const dbtype::EntityType type)
    {
        bool result = false;

        if (ids.empty())
        {
            return true;
        }

        if ((type != dbtype::ENTITYTYPE_player) and
             (type != dbtype::ENTITYTYPE_puppet))
        {
            LOG(error, "primitives", "remove_entity()",
                "Bad Entity type passed in: " + text::to_string(type));
            return result;
        }

        dbtype::Entity::IdSet::size_type ids_remaining = ids.size();

        boost::unique_lock<boost::shared_mutex> write_lock(mutex);

        SiteToNamesMap::iterator site_iter = names.find(ids.begin()->get_site_id());

        if (site_iter != names.end())
        {
            result = true;

            NameInfoVector * const name_infos =
                type == dbtype::ENTITYTYPE_player ? &site_iter->second.first :
                    &site_iter->second.second;

            if (name_infos->empty())
            {
                // Empty, nothing more to do.
                return result;
            }

            NameInfoVector::iterator iter = --name_infos->end();

            // Go through the vector of IDs once, removing any found in
            // the set.  Start in reverse so we are not copying around name info
            // for stuff that is actually about to be removed, and to simplify
            // the logic.  If enough removals have been done that equal
            // the set count, then we've removed everything and can finish
            // early.
            //
            do
            {
                if (not ids.count(iter->id))
                {
                    if (iter == name_infos->begin())
                    {
                        // Finished checking all of them.
                        break;
                    }
                    else
                    {
                        --iter;
                    }
                }
                else
                {
                    if (iter == --name_infos->end())
                    {
                        // At end or only one element.  Just pop it off.
                        //
                        name_infos->pop_back();

                        if (name_infos->empty())
                        {
                            // Out of elements to check.
                            break;
                        }
                        else
                        {
                            iter = --name_infos->end();
                        }
                    }
                    else
                    {
                        // Not at end. Swap last one in
                        *iter = name_infos->back();
                        name_infos->pop_back();

                        if (iter == name_infos->begin())
                        {
                            // Finished checking all of them.
                            break;
                        }
                        else
                        {
                            --iter;
                        }
                    }

                    --ids_remaining;
                }
            }
            while (ids_remaining);
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool NameRegistry::remove_entity(
        const dbtype::Id &id,
        const dbtype::EntityType type)
    {
        bool result = false;

        if (id.is_default() or
            ((type != dbtype::ENTITYTYPE_player) and
             (type != dbtype::ENTITYTYPE_puppet)))
        {
            LOG(error, "primitives", "remove_entity()",
                "Not all arguments  properly filled out.  Cannot remove "
                + id.to_string((true)));
            return result;
        }

        boost::unique_lock<boost::shared_mutex> write_lock(mutex);

        SiteToNamesMap::iterator site_iter = names.find(id.get_site_id());

        if (site_iter != names.end())
        {
            NameInfoVector * const name_infos =
                type == dbtype::ENTITYTYPE_player ? &site_iter->second.first :
                    &site_iter->second.second;

            // Search for the entry
            //
            for (NameInfoVector::iterator name_iter = name_infos->begin();
                 name_iter != name_infos->end();
                 ++name_iter)
            {
                if (name_iter->id == id)
                {
                    result = true;

                    // Found it; remove by copying the last one on top of
                    // this one, or remove the last entry if the very last
                    // one.
                    //
                    if (name_iter == --name_infos->end())
                    {
                        // This one is actually the very last entry
                        name_infos->pop_back();
                    }
                    else
                    {
                        // Swap last one in.
                        *name_iter = name_infos->back();
                        name_infos->pop_back();
                    }

                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------
    NameRegistry::NameRegistry(void)
    {
    }
}
}
