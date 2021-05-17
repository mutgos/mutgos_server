/*
 * dbtype_DatabaseEntityChangeListener.cpp
 */

#include "dbtypes/dbtype_DatabaseEntityChangeListener.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    DatabaseEntityChangeListener::DatabaseEntityChangeListener(void)
    {
    }

    // ----------------------------------------------------------------------
    DatabaseEntityChangeListener::~DatabaseEntityChangeListener()
    {
    }

    // ----------------------------------------------------------------------
    bool DatabaseEntityChangeListener::check_program_registration_name(
        Entity *entity,
        concurrency::WriterLockToken &token,
        const std::string &old_name,
        const std::string &new_name)
    {
        return true;
    }
} /* namespace dbtype */
} /* namespace mutgos */
