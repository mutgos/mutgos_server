/*
 * primitives_CommonTypes.h
 */

#ifndef MUTGOS_PRIMITIVES_COMMONTYPES_H
#define MUTGOS_PRIMITIVES_COMMONTYPES_H

#include <vector>

#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace primitives
{
    /** Vector of IDs */
    typedef std::vector<dbtype::Id> IdVector;
}
}

#endif //MUTGOS_PRIMITIVES_COMMONTYPES_H
