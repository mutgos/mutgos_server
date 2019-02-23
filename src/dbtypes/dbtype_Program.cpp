/*
 * dbtype_Program.cpp
 */

#include <string>
#include <string.h>
#include <iostream>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_Program.h"

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_PropertyEntity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DocumentProperty.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

#define DEFAULT_MAX_PROGRAM_STRING_LENGTH 2048
#define DEFAULT_MAX_PROGRAM_LINES 3192

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    Program::Program()
        : PropertyEntity(),
          program_runtime_sec(0)
    {
        program_source_code.set_max_line_length(
            DEFAULT_MAX_PROGRAM_STRING_LENGTH);
        program_source_code.set_max_lines(DEFAULT_MAX_PROGRAM_LINES);
    }

    // ----------------------------------------------------------------------
    Program::Program(const Id &id)
        : PropertyEntity(id, ENTITYTYPE_program, 0, 0),
          program_runtime_sec(0)
    {
        program_source_code.set_max_line_length(
            DEFAULT_MAX_PROGRAM_STRING_LENGTH);
        program_source_code.set_max_lines(DEFAULT_MAX_PROGRAM_LINES);
    }

    // ----------------------------------------------------------------------
    Program::~Program()
    {
    }

    // ----------------------------------------------------------------------
    Entity *Program::clone(
        const Id &id,
        const VersionType version,
        const InstanceType instance,
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            Entity *copy_ptr = new Program(
                id,
                ENTITYTYPE_program,
                version,
                instance);

            copy_fields(copy_ptr);

            return copy_ptr;
        }
        else
        {
            LOG(error, "dbtype", "clone",
                "Using the wrong lock token!");

            return 0;
        }
    }

    // ----------------------------------------------------------------------
    std::string Program::to_string(void)
    {
        concurrency::ReaderLockToken token(*this);

        std::ostringstream strstream;

        strstream << PropertyEntity::to_string()
                  << "Total runtime (secs): " << program_runtime_sec
                  << std::endl
                  << "Source code (lines): "
                  << program_source_code.get_number_lines()  << std::endl
                  << "Compiled code (bytes): " << program_compiled_code.size()
                  << std::endl
                  << "Program language: " << program_language
                  << std::endl
                  << "Program includes:";

        for (IdSet::const_iterator id_iter = program_includes.begin();
             id_iter != program_includes.end();
             ++id_iter)
        {
            strstream << " " << id_iter->to_string(true);
        }

        strstream << std::endl;

        return strstream.str();
    }

    // ----------------------------------------------------------------------
    bool Program::increment_runtime(
        const osinterface::OsTypes::Double seconds,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            program_runtime_sec += seconds;
            notify_field_changed(ENTITYFIELD_program_runtime_sec);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "increment_runtime",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::increment_runtime(const osinterface::OsTypes::Double seconds)
    {
        concurrency::WriterLockToken token(*this);

        return increment_runtime(seconds, token);
    }

    // ----------------------------------------------------------------------
    DocumentProperty Program::get_source_code(
        concurrency::ReaderLockToken &token)
    {
        if (token.has_lock(*this))
        {
            // operator= not supported
            return program_source_code;
        }
        else
        {
            LOG(error, "dbtype", "get_source_code",
                "Using the wrong lock token!");
        }

        // Default
        return DocumentProperty();
    }

    // ----------------------------------------------------------------------
    DocumentProperty Program::get_source_code(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_source_code(token);
    }

    // ----------------------------------------------------------------------
    bool Program::set_source_code(
        const DocumentProperty &source_code,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            result = program_source_code.set_from_string(
                source_code.get_as_string());
            notify_field_changed(ENTITYFIELD_program_source_code);
        }
        else
        {
            LOG(error, "dbtype", "set_source_code",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::set_source_code(const DocumentProperty &source_code)
    {
        concurrency::WriterLockToken token(*this);

        return set_source_code(source_code, token);
    }

    // ----------------------------------------------------------------------
    bool Program::has_compiled_code(void)
    {
        concurrency::ReaderLockToken token(*this);

        return has_compiled_code(token);
    }

    // ----------------------------------------------------------------------
    bool Program::has_compiled_code(concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            result = not program_compiled_code.empty();
        }
        else
        {
            LOG(error, "dbtype", "has_compiled_code",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::get_compiled_code(
        char *&data_ptr,
        size_t &data_size,
        concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if (program_compiled_code.empty())
            {
                data_ptr = 0;
                data_size = 0;
            }
            else
            {
                data_size = program_compiled_code.size();

                data_ptr = new char[data_size];
                memcpy(
                    data_ptr,
                    program_compiled_code.data(),
                    data_size);

                result = true;
            }
        }
        else
        {
            LOG(error, "dbtype", "get_compiled_code",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::get_compiled_code(char *&data_ptr, size_t &data_size)
    {
        concurrency::ReaderLockToken token(*this);

        return get_compiled_code(data_ptr, data_size, token);
    }

    // ----------------------------------------------------------------------
    bool Program::set_compiled_code(
        const char *data,
        const size_t data_size,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            if ((data_size <= 0) or (not data))
            {
                program_compiled_code.clear();
            }
            else
            {
                program_compiled_code.assign(data, data_size);
            }

            notify_field_changed(ENTITYFIELD_program_compiled_code);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_compiled_code",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::set_compiled_code(
        const char *data,
        const size_t data_size)
    {
        concurrency::WriterLockToken token(*this);

        return set_compiled_code(data, data_size, token);
    }

    // ----------------------------------------------------------------------
    std::string Program::get_program_language(
        concurrency::ReaderLockToken &token)
    {
        std::string result;

        if (token.has_lock(*this))
        {
            result = program_language;
        }
        else
        {
            LOG(error, "dbtype", "get_program_language",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    std::string Program::get_program_language(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_program_language(token);
    }

    // ----------------------------------------------------------------------
    bool Program::set_program_language(
        const std::string &language,
       concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            program_language = language;
            notify_field_changed(ENTITYFIELD_program_language);
            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_program_language",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::set_program_language(
        const std::string &language)
    {
        concurrency::WriterLockToken token(*this);

        return set_program_language(language, token);
    }

    // ----------------------------------------------------------------------
    bool Program::set_program_includes(
        const IdSet &includes,
        concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            // Mark all existing as removed.  If they weren't actually removed,
            // it will cancel out on its own...
            //
            for (IdSet::const_iterator id_remove_iter = program_includes.begin();
                id_remove_iter != program_includes.end();
                ++id_remove_iter)
            {
                removed_id(ENTITYFIELD_program_includes, *id_remove_iter);
            }

            program_includes.clear();

            // ...and add the ones in the new set
            //
            for (IdSet::const_iterator id_add_iter = includes.begin();
                id_add_iter != includes.end();
                ++id_add_iter)
            {
                program_includes.insert(*id_add_iter);
                added_id(ENTITYFIELD_program_includes, *id_add_iter);
            }

            notify_field_changed(ENTITYFIELD_program_includes);

            result = true;
        }
        else
        {
            LOG(error, "dbtype", "set_program_includes",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::set_program_includes(
        const IdSet &includes)
    {
        concurrency::WriterLockToken token(*this);

        return set_program_includes(includes, token);
    }

    // ----------------------------------------------------------------------
    bool Program::is_program_included(
        const Id &program_id,
        concurrency::ReaderLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            result =
                (program_includes.find(program_id) != program_includes.end());
        }
        else
        {
            LOG(error, "dbtype", "is_program_included",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::is_program_included(
        const Id &program_id)
    {
        concurrency::ReaderLockToken token(*this);

        return is_program_included(program_id, token);
    }

    // ----------------------------------------------------------------------
    bool Program::clear_included_programs(concurrency::WriterLockToken &token)
    {
        bool result = false;

        if (token.has_lock(*this))
        {
            result = set_program_includes(IdSet());
        }
        else
        {
            LOG(error, "dbtype", "clear_included_programs",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    bool Program::clear_included_programs(void)
    {
        concurrency::WriterLockToken token(*this);

        return clear_included_programs(token);
    }

    // ----------------------------------------------------------------------
    Entity::IdSet Program::get_program_includes(
        concurrency::ReaderLockToken &token)
    {
        IdSet result;

        if (token.has_lock(*this))
        {
            result = program_includes;
        }
        else
        {
            LOG(error, "dbtype", "get_program_includes",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Entity::IdSet Program::get_program_includes(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_program_includes(token);
    }

    // ----------------------------------------------------------------------
    Id Program::get_first_program_include(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not program_includes.empty())
            {
                result = *(program_includes.begin());
            }
        }
        else
        {
            LOG(error, "dbtype", "get_first_program_include",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Program::get_first_program_include(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_first_program_include();
    }

    // ----------------------------------------------------------------------
    Id Program::get_next_program_include(
        const Id &program_id,
        concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not program_includes.empty())
            {
                IdSet::const_iterator iter = program_includes.find(program_id);

                if (iter != program_includes.end())
                {
                    ++iter;

                    if (iter != program_includes.end())
                    {
                        result = *iter;
                    }
                }
            }
        }
        else
        {
            LOG(error, "dbtype", "get_next_program_include",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Program::get_next_program_include(const Id &program_id)
    {
        concurrency::ReaderLockToken token(*this);

        return get_next_program_include(program_id);
    }

    // ----------------------------------------------------------------------
    Id Program::get_last_program_include(concurrency::ReaderLockToken &token)
    {
        Id result;

        if (token.has_lock(*this))
        {
            if (not program_includes.empty())
            {
                result = *(--(program_includes.end()));
            }
        }
        else
        {
            LOG(error, "dbtype", "get_last_program_include",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    Id Program::get_last_program_include(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_last_program_include();
    }

    // ----------------------------------------------------------------------
    size_t Program::get_program_include_count(
        concurrency::ReaderLockToken &token)
    {
        size_t result = 0;

        if (token.has_lock(*this))
        {
            result = program_includes.size();
        }
        else
        {
            LOG(error, "dbtype", "get_program_include_count",
                "Using the wrong lock token!");
        }

        return result;
    }

    // ----------------------------------------------------------------------
    size_t Program::get_program_include_count(void)
    {
        concurrency::ReaderLockToken token(*this);

        return get_program_include_count(token);
    }

    // ----------------------------------------------------------------------
    Program::Program(
        const Id &id,
        const EntityType &type,
        const VersionType version,
        const InstanceType instance,
        const bool restoring)
      : PropertyEntity(id, type, version, instance, restoring),
        program_runtime_sec(0)
    {
    }

    // ----------------------------------------------------------------------
    size_t Program::mem_used_fields(void)
    {
        size_t field_sizes = PropertyEntity::mem_used_fields()
            + sizeof(program_runtime_sec)
            + program_source_code.mem_used()
            + program_compiled_code.size() + sizeof(program_compiled_code)
            + program_language.size() + sizeof(program_language)
            + sizeof(program_includes);

        // Add up IDs in the program includes
        for (Entity::IdSet::const_iterator iter = program_includes.begin();
            iter != program_includes.end();
            ++iter)
        {
            field_sizes += iter->mem_used();
        }

        return field_sizes;
    }

    // ----------------------------------------------------------------------
    void Program::copy_fields(Entity *entity_ptr)
    {
        PropertyEntity::copy_fields(entity_ptr);

        Program *cast_ptr = 0;

        if (entity_ptr and
            ((cast_ptr = (dynamic_cast<Program *>(entity_ptr))) != 0))
        {
            cast_ptr->program_runtime_sec = program_runtime_sec;
            cast_ptr->notify_field_changed(ENTITYFIELD_program_runtime_sec);

            cast_ptr->program_source_code.set(program_source_code.get());
            cast_ptr->notify_field_changed(ENTITYFIELD_program_source_code);

            cast_ptr->program_compiled_code = program_compiled_code;
            cast_ptr->notify_field_changed(ENTITYFIELD_program_compiled_code);

            cast_ptr->program_language = program_language;
            cast_ptr->notify_field_changed(ENTITYFIELD_program_language);

            cast_ptr->program_includes = program_includes;

            for (Entity::IdSet::const_iterator iter =
                    cast_ptr->program_includes.begin();
                iter != cast_ptr->program_includes.end();
                ++iter)
            {
                cast_ptr->added_id(ENTITYFIELD_program_includes, *iter);
            }

            cast_ptr->notify_field_changed(ENTITYFIELD_program_includes);
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */
