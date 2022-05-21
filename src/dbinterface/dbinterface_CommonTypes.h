/*
 * dbinterface_CommonTypes.h
 */

#ifndef MUTGOS_DBINTERFACE_COMMONTYPES_H
#define MUTGOS_DBINTERFACE_COMMONTYPES_H

#include <vector>

#include "dbtypes/dbtype_Id.h"

#include "dbinterface/dbinterface_EntityMetadata.h"

namespace mutgos
{
namespace dbinterface
{
    static const dbtype::Id::SiteIdType GLOBAL_SITE_ID = 1;

    typedef std::vector<EntityMetadata> MetadataVector;
}
}

#endif //MUTGOS_DBINTERFACE_COMMONTYPES_H
