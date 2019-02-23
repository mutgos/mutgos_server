/*
 * angelscript_SimpleGCObject.cpp
 */

#include <angelscript.h>

#include "logging/log_Logger.h"
#include "angelscript_SimpleGCObject.h"

namespace mutgos
{
namespace angelscript
{
    // ----------------------------------------------------------------------
    SimpleGCObject::~SimpleGCObject()
    {
    }

    // ----------------------------------------------------------------------
    void SimpleGCObject::add_ref(void)
    {
        ++ref_count;
        gc_flag = false;
    }

    // ----------------------------------------------------------------------
    void SimpleGCObject::release_ref(void)
    {
        --ref_count;
        gc_flag = false;

        if (ref_count <= 0)
        {
            delete this;
        }
    }

    // ----------------------------------------------------------------------
    int SimpleGCObject::get_ref_count(void)
    {
        return ref_count;
    }

    // ----------------------------------------------------------------------
    void SimpleGCObject::set_gc_flag(void)
    {
        gc_flag = true;
    }

    // ----------------------------------------------------------------------
    bool SimpleGCObject::get_gc_flag(void)
    {
        return gc_flag;
    }

    // ----------------------------------------------------------------------
    void SimpleGCObject::enum_references(asIScriptEngine *engine_ptr)
    {
        // Currently we do not reference other objects.
    }

    // ----------------------------------------------------------------------
    void SimpleGCObject::release_all_references(asIScriptEngine *engine_ptr)
    {
        // Currently we do not reference other objects.
    }

    // ----------------------------------------------------------------------
    SimpleGCObject::SimpleGCObject(
        asIScriptEngine *engine,
        const std::string &type,
        bool register_with_gc)
        : engine_ptr(engine),
          ref_count(1),
          gc_flag(false)
    {
        if (not engine_ptr)
        {
            LOG(fatal, "angelscript", "SimpleGCObject",
                "engine is null!  Crash will follow.");
        }

        if (register_with_gc)
        {
            // Notify the garbage collector of this object's creation.
            //
            engine_ptr->NotifyGarbageCollectorOfNewObject(
                this,
                engine_ptr->GetTypeInfoByName(type.c_str()));
        }
    }

    // ----------------------------------------------------------------------
    SimpleGCObject::SimpleGCObject(
        const SimpleGCObject &rhs,
        const std::string &type,
        bool register_with_gc)
        : engine_ptr(rhs.engine_ptr),
          ref_count(1),
          gc_flag(false)
    {
        if (not engine_ptr)
        {
            LOG(fatal, "angelscript", "SimpleGCObject",
                "engine is null!  Crash will follow.");
        }

        if (register_with_gc)
        {
            // Notify the garbage collector of this object's creation.
            //
            engine_ptr->NotifyGarbageCollectorOfNewObject(
                this,
                engine_ptr->GetTypeInfoByName(type.c_str()));
        }
    }
}
}
