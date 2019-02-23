#include <string>

#include "dbinterface_CachedEntity.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbinterface_EntityRef.h"
#include "dbinterface_EntityRefCounter.h"

#include "logging/log_Logger.h"

namespace mutgos
{
namespace dbinterface
{
    // ----------------------------------------------------------------------
    CachedEntity::CachedEntity(dbtype::Entity *entity)
      : ref_count(0),
        entity_ptr(entity)
    {
        if (not entity_ptr)
        {
            LOG(fatal, "dbinterface", "CachedEntity()",
                "entity_ptr is null!");
        }
        else
        {
            LOG(debug, "dbinterface", "CachedEntity()",
                "Creating CachedEntity for ID "
                  + entity_ptr->get_entity_id().to_string(true));
        }
    }

    // ----------------------------------------------------------------------
    CachedEntity::~CachedEntity()
    {
        if (ref_count)
        {
            LOG(fatal, "dbinterface", "~CachedEntity()",
                "Being destructed when there are still references!  ID: "
                  + entity_ptr->get_entity_id().to_string(true));
        }
        else
        {
            LOG(debug, "dbinterface", "~CachedEntity()",
                "Destructing CachedEntity for ID "
                + entity_ptr->get_entity_id().to_string(true));
        }

        entity_ptr = 0;
    }

    // ----------------------------------------------------------------------
    void CachedEntity::mem_reference_added(const dbtype::Entity *entity)
    {
        if (entity != entity_ptr)
        {
            LOG(fatal, "dbinterface", "mem_reference_added()",
                "Mismatched entity pointers!");
        }
        else
        {
            boost::lock_guard<boost::mutex> guard(mutex);
            ++ref_count;
        }
    }

    // ----------------------------------------------------------------------
    void CachedEntity::mem_reference_removed(const dbtype::Entity *entity)
    {
        if (entity != entity_ptr)
        {
            LOG(fatal, "dbinterface", "mem_reference_removed()",
                "Mismatched entity pointers!");
        }
        else
        {
            boost::lock_guard<boost::mutex> guard(mutex);

            if (ref_count)
            {
                --ref_count;
            }
            else
            {
                LOG(fatal, "dbinterface", "mem_reference_removed()",
                    "More references than were counted!");
            }
        }
    }

    // ----------------------------------------------------------------------
    void CachedEntity::get_reference(EntityRef &ref)
    {
        ref.set_reference(entity_ptr, this);
    }

    // ----------------------------------------------------------------------
    bool CachedEntity::is_referenced()
    {
        boost::lock_guard<boost::mutex> guard(mutex);

        return ref_count;
    }

    // ----------------------------------------------------------------------
    const dbtype::Id &CachedEntity::get_id() const
    {
        return entity_ptr->get_entity_id();
    }
}
}
