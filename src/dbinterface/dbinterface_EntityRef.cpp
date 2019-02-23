
#include "dbinterface_EntityRef.h"
#include "dbinterface_EntityRefCounter.h"

namespace mutgos
{
namespace dbinterface
{
    // ----------------------------------------------------------------------
    EntityRef::EntityRef()
      : entity_ptr(0),
        ref_counter_ptr(0)
    {
    }

    // ----------------------------------------------------------------------
    EntityRef::EntityRef(dbtype::Entity *entity, EntityRefCounter *ref_counter)
      : entity_ptr(entity),
        ref_counter_ptr(ref_counter)
    {
        hold_ref();
    }

    // ----------------------------------------------------------------------
    EntityRef::EntityRef(const EntityRef &rhs)
      : entity_ptr(rhs.entity_ptr),
        ref_counter_ptr(rhs.ref_counter_ptr)
    {
        hold_ref();
    }

    // ----------------------------------------------------------------------
    EntityRef::~EntityRef()
    {
        release_ref();
        entity_ptr = 0;
        ref_counter_ptr = 0;
    }

    // ----------------------------------------------------------------------
    void EntityRef::set_reference(
        dbtype::Entity *entity,
        EntityRefCounter *ref_counter)
    {
        if (entity != entity_ptr)
        {
            release_ref();
            entity_ptr = entity;
            ref_counter_ptr = ref_counter;
            hold_ref();
        }
    }

    // ----------------------------------------------------------------------
    void EntityRef::clear()
    {
        release_ref();
        entity_ptr = 0;
        ref_counter_ptr = 0;
    }

    // ----------------------------------------------------------------------
    EntityRef &EntityRef::operator=(const EntityRef &rhs)
    {
        release_ref();
        entity_ptr = rhs.entity_ptr;
        ref_counter_ptr = rhs.ref_counter_ptr;
        hold_ref();

        return *this;
    }

    // ----------------------------------------------------------------------
    bool EntityRef::operator==(const EntityRef &rhs) const
    {
        return (entity_ptr == rhs.entity_ptr);
    }

    // ----------------------------------------------------------------------
    bool EntityRef::operator!=(const EntityRef &rhs) const
    {
        return (entity_ptr != rhs.entity_ptr);
    }

    // ----------------------------------------------------------------------
    void EntityRef::release_ref()
    {
        if (ref_counter_ptr and entity_ptr)
        {
            ref_counter_ptr->mem_reference_removed(entity_ptr);
        }
    }

    // ----------------------------------------------------------------------
    void EntityRef::hold_ref()
    {
        if (ref_counter_ptr and entity_ptr)
        {
            ref_counter_ptr->mem_reference_added(entity_ptr);
        }
    }
}
}