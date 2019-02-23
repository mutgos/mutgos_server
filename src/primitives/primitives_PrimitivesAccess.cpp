/*
 * primitives_PrimitivesAccess.cpp
 */

#include "primitives_PrimitivesAccess.h"

namespace mutgos
{
namespace primitives
{
    // Statics
    //
    PrimitivesAccess *PrimitivesAccess::singleton_ptr = 0;

    // ----------------------------------------------------------------------
    PrimitivesAccess *PrimitivesAccess::make_singleton(void)
    {
        if (not singleton_ptr)
        {
            singleton_ptr = new PrimitivesAccess();
        }

        return singleton_ptr;
    }

    // ----------------------------------------------------------------------
    void PrimitivesAccess::destroy_singleton(void)
    {
        delete singleton_ptr;
        singleton_ptr = 0;
    }

    // ----------------------------------------------------------------------
    bool PrimitivesAccess::startup(void)
    {
        // Nothing to do for now.
        return true;
    }

    // ----------------------------------------------------------------------
    void PrimitivesAccess::shutdown(void)
    {
        // Nothing to do for now.
    }

    // ----------------------------------------------------------------------
    PrimitivesAccess::PrimitivesAccess(void)
    {
    }

    // ----------------------------------------------------------------------
    PrimitivesAccess::~PrimitivesAccess()
    {
    }
}
}