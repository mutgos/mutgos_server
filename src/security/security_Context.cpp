/*
 * security_Context.cpp
 */

#include <string>
#include <sstream>
#include <algorithm>

#include "text/text_StringConversion.h"

#include "security_Context.h"

#define CONTEXT_CACHE_SIZE 64

namespace mutgos
{
namespace security
{
    // TODO Maybe get rid of or expand cache?  May be too small to be useful and be constantly overwritten

    // -----------------------------------------------------------------------
    Context::Context(
        const mutgos::dbtype::Id requesting_entity,
        const mutgos::dbtype::Id requesting_program)
          : requester(requesting_entity),
            program(requesting_program),
            pid(0),
            populated_capabilities(false),
            admin(false),
            run_as_requester(false),
            context_cache_index(0)
    {
    }

    // -----------------------------------------------------------------------
    Context::~Context()
    {
    }

    // -----------------------------------------------------------------------
    void Context::reset_capabilities(void)
    {
        populated_capabilities = false;
        admin = false;
        capabilities.clear();
        context_cache.clear();
        context_cache_index = 0;
    }

    // -----------------------------------------------------------------------
    std::string Context::to_string() const
    {
        std::ostringstream strstream;

        strstream << "[Requester: " << requester.to_string(true)
                  << "  Program: " << program.to_string(true)
                  << "  PID: " << pid
                  << "  Populated: " << populated_capabilities
                  << "  IsAdmin: " << admin
                  << "  RunAsRequester: " << run_as_requester
                  << "  Capabilities:";

        for (Capabilities::const_iterator iter = capabilities.begin();
            iter != capabilities.end();
            ++iter)
        {
            strstream << "  " << *iter;
        }

        strstream << "]";

        return strstream.str();
    }

    // -----------------------------------------------------------------------
    void Context::add_capability(const security::Capability capability)
    {
        // Confirm capability doesn't already exist before adding.
        //
        if (std::find(capabilities.begin(), capabilities.end(), capability) ==
            capabilities.end())
        {
            capabilities.push_back(capability);
        }
    }

    // -----------------------------------------------------------------------
    bool Context::has_capability(const security::Capability capability) const
    {
        // If has admin rights, then it has all capabilities.
        return admin ? true : std::find(
            capabilities.begin(),
            capabilities.end(),
            capability) != capabilities.end();
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
            cache_iter != context_cache.end();
            ++cache_iter)
        {
            if (cache_iter->operation == operation)
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const security::Operation operation,
        const security::Result result)
    {
        add_cache_entry(
            result,
            operation,
            dbtype::ENTITYTYPE_invalid,
            dbtype::ENTITYFIELD_invalid,
            std::string(),
            dbtype::Id(),   // Target
            dbtype::Id());  // Source
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        const dbtype::EntityType entity_type,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
             cache_iter != context_cache.end();
             ++cache_iter)
        {
            if ((cache_iter->operation == operation) and
                (cache_iter->entity_type == entity_type))
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const Operation operation,
        const dbtype::EntityType entity_type,
        const Result result)
    {
        add_cache_entry(
            result,
            operation,
            entity_type,
            dbtype::ENTITYFIELD_invalid,
            std::string(),
            dbtype::Id(),   // Target
            dbtype::Id());  // Source
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        const dbtype::Id &entity_target,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
             cache_iter != context_cache.end();
             ++cache_iter)
        {
            if ((cache_iter->operation == operation) and
                (cache_iter->entity_target == entity_target))
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const Operation operation,
        const dbtype::Id &entity_target,
        const Result result)
    {
        add_cache_entry(
            result,
            operation,
            dbtype::ENTITYTYPE_invalid,
            dbtype::ENTITYFIELD_invalid,
            std::string(),
            entity_target,  // Target
            dbtype::Id());  // Source
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        const dbtype::Id &entity_target,
        const std::string &application,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
             cache_iter != context_cache.end();
             ++cache_iter)
        {
            if ((cache_iter->operation == operation) and
                (cache_iter->entity_target == entity_target) and
                (cache_iter->application_name == application))
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const Operation operation,
        const dbtype::Id &entity_target,
        const std::string &application,
        const Result result)
    {
        add_cache_entry(
            result,
            operation,
            dbtype::ENTITYTYPE_invalid,
            dbtype::ENTITYFIELD_invalid,
            application,
            entity_target,  // Target
            dbtype::Id());  // Source
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        const dbtype::Id &entity_target,
        const dbtype::EntityField entity_field,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
             cache_iter != context_cache.end();
             ++cache_iter)
        {
            if ((cache_iter->operation == operation) and
                (cache_iter->entity_target == entity_target) and
                (cache_iter->entity_field == entity_field))
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const Operation operation,
        const dbtype::Id &entity_target,
        const dbtype::EntityField entity_field,
        const Result result)
    {
        add_cache_entry(
            result,
            operation,
            dbtype::ENTITYTYPE_invalid,
            entity_field,
            std::string(),
            entity_target,  // Target
            dbtype::Id());  // Source
    }

    // -----------------------------------------------------------------------
    bool Context::security_check_cache(
        const Operation operation,
        const dbtype::Id &entity_target,
        const dbtype::Id &entity_source,
        Result &result) const
    {
        for (ContextCache::const_iterator cache_iter = context_cache.begin();
             cache_iter != context_cache.end();
             ++cache_iter)
        {
            if ((cache_iter->operation == operation) and
                (cache_iter->entity_target == entity_target) and
                (cache_iter->entity_source == entity_source))
            {
                result = cache_iter->operation_result;
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    void Context::save_security_check_result(
        const Operation operation,
        const dbtype::Id &entity_target,
        const dbtype::Id &entity_source,
        const Result result)
    {
        add_cache_entry(
            result,
            operation,
            dbtype::ENTITYTYPE_invalid,
            dbtype::ENTITYFIELD_invalid,
            std::string(),
            entity_target,  // Target
            entity_source); // Source
    }

    // -----------------------------------------------------------------------
    void Context::clear_security_check_cache(void)
    {
        context_cache_index = 0;
        context_cache.clear();
    }

    // -----------------------------------------------------------------------
    void Context::add_cache_entry(
        const security::Result operation_result,
        const security::Operation operation,
        const dbtype::EntityType entity_type,
        const dbtype::EntityField entity_field,
        const std::string &application_name,
        const dbtype::Id &entity_target,
        const dbtype::Id &entity_source)
    {
        ContextCacheEntry *entry = 0;

        if (context_cache.size() < CONTEXT_CACHE_SIZE)
        {
            // Cache too small, just insert at the end.
            //
            context_cache.push_back(ContextCacheEntry());
            entry = &context_cache.back();
        }
        else
        {
            // Cache is 'full', so overwrite older entry.
            //
            entry = &context_cache[context_cache_index];
            ++context_cache_index;

            if (context_cache_index >= CONTEXT_CACHE_SIZE)
            {
                context_cache_index = 0;
            }
        }

        entry->operation_result = operation_result;
        entry->operation = operation;
        entry->entity_type = entity_type;
        entry->entity_field = entity_field;
        entry->application_name = application_name;
        entry->entity_target = entity_target;
        entry->entity_source = entity_source;
    }

    // -----------------------------------------------------------------------
    Context::ContextCacheEntry::ContextCacheEntry(void)
        : operation_result(RESULT_SKIP),
          operation(OPERATION_END_INVALID),
          entity_type(dbtype::ENTITYTYPE_invalid),
          entity_field(dbtype::ENTITYFIELD_invalid)
    {
    }
}
}
