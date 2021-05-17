/*
 * dbtype_Program.h
 */

#ifndef MUTGOS_DBTYPE_PROGRAM_H
#define MUTGOS_DBTYPE_PROGRAM_H

#include <string>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"

#include "dbtypes/dbtype_PropertyEntity.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

#include "dbtypes/dbtype_Entity.h"
#include "dbtypes/dbtype_Id.h"
#include "dbtypes/dbtype_DocumentProperty.h"

#include "concurrency/concurrency_ReaderLockToken.h"
#include "concurrency/concurrency_WriterLockToken.h"
#include "concurrency/concurrency_LockableObject.h"

namespace mutgos
{
namespace dbtype
{
    class Program : public PropertyEntity
    {
    public:
        /**
         * Constructor used for deserialization of a Program.
         */
        Program();

        /**
         * Constructs a Program (final type).
         * @param id[in] The ID of the entity.
         */
        Program(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Program();

        /**
         * Creates a copy of this Program.
         * @param id[in] The new ID of the cloned Program.
         * @param version[in] The new version # of this Program.
         * @param instance[in] The new instance # of this Program.
         * @param token[in] The lock token.
         * @return The clone as a pointer.  Caller must manage the pointer!
         * Null is returned if there is an error, such as an incorrect
         * lock token.
         */
        virtual Entity *clone(
            const Id &id,
            const VersionType version,
            const InstanceType instance,
            concurrency::ReaderLockToken &token);

        /**
         * @return Entity as a string.  Used for debugging and logging
         * purposes only.
         */
        virtual std::string to_string(void);

        /**
         * Increments the culmulative runtime by the given seconds.
         * This is very approximate and subject to the CPU level of
         * accuracy.
         * @param seconds[in] The seconds to increment the runtime by.
         * @param token[in] The lock token.
         * @return True if successfully incremented.
         */
        bool increment_runtime(
            const osinterface::OsTypes::Double seconds,
            concurrency::WriterLockToken &token);

        /**
         * Increments the culmulative runtime by the given seconds.
         * This is very approximate and subject to the CPU level of
         * accuracy.
         * This method will automatically get a lock.
         * @param seconds[in] The seconds to increment the runtime by.
         * @return True if successfully incremented.
         */
        bool increment_runtime(const osinterface::OsTypes::Double seconds);

        /**
         * @param token[in] The lock token.
         * @return A copy of the program registration name (if a global
         * library), or empty if none or error.
         */
        std::string get_program_reg_name(concurrency::ReaderLockToken &token);

        /**
         * This method automatically gets a lock.
         * @return A copy of the program registration name (if a global
         * library), or default if none or error.
         */
        std::string get_program_reg_name(void);

        /**
         * Sets the global registration name for the program; typically this
         * is used for shared libraries.
         * There can only be one program per site with the same reg name,
         * unless the name is empty (none assigned).
         * @param reg_name[in] The global registation name.  Not case sensitive.
         * Excess spaces will be trimmed.
         * Spaces embedded in the name are not allowed.
         * @param token[in] The lock token.
         * @return True if success, false if name already in use, has embedded
         * spaces, etc.
         */
        bool set_program_reg_name(
            const std::string &reg_name,
            concurrency::WriterLockToken &token);

        /**
         * Sets the global registration name for the program; typically this
         * is used for shareed libraries.
         * This method automatically gets a lock.
         * @param reg_name[in] The global registation name.  Not case sensitive.
         * Excess spaces will be trimmed.
         * Spaces embedded in the name are not allowed.
         * @return True if success, false if name already in use, has embedded
         * spaces, etc.
         */
        bool set_program_reg_name(const std::string &reg_name);

        /**
         * @param token[in] The lock token.
         * @return A copy of the source code, or default if none or error.
         */
        DocumentProperty get_source_code(concurrency::ReaderLockToken &token);

        /**
         * This method automatically gets a lock.
         * @param token[in] The lock token.
         * @return A copy of the source code, or default if none or error.
         */
        DocumentProperty get_source_code(void);

        /**
         * Sets the source code to the provided DocumentProperty.
         * Suggested to use a Document retrieved by get_source_code(), to
         * preserve length limits.
         * @param source_code[in] The source code to set.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_source_code(
            const DocumentProperty &source_code,
            concurrency::WriterLockToken &token);

        /**
         * Sets the source code to the provided DocumentProperty.
         * Suggested to use a Document retrieved by get_source_code(), to
         * preserve length limits.
         * This method automatically gets a lock.
         * @param source_code[in] The source code to set.
         * @return True if success.
         */
        bool set_source_code(const DocumentProperty &source_code);


        /**
         * This method will automatically get a lock.
         * @return True if compiled code has been set on this Program, false
         * if not or error.
         */
        bool has_compiled_code(void);

        /**
         * @param token[in] The lock token.
         * @return True if compiled code has been set on this Program, false
         * if not or error.
         */
        bool has_compiled_code(concurrency::ReaderLockToken &token);


        /**
         * Gets the compiled code binary data.  Caller must manage
         * pointer!
         * @param data_ptr[out] The opaque binary data for use with the
         * interpreter.  This is a copy, and therefore you must delete it when
         * done.  Will be null if no compiled version exists.
         * @param data_size[out] The size of data_ptr.
         * @param token[in] The lock token.
         * @return True if binary data was retrieved, false if none
         * available or error.
         */
        bool get_compiled_code(
            char *&data_ptr,
            size_t &data_size,
            concurrency::ReaderLockToken &token);

        /**
         * Gets the compiled code binary data.  Caller must manage
         * pointer!
         * This method will automatically get a lock.
         * @param data_ptr[out] The opaque binary data for use with the
         * interpreter.  This is a copy, and therefore you must delete it when
         * done.  Will be null if no compiled version exists.
         * @param data_size[out] The size of data_ptr.
         * @return True if binary data was retrieved, false if none
         * available or error.
         */
        bool get_compiled_code(
            char *&data_ptr,
            size_t &data_size);

        /**
         * Sets the compiled binary code data.
         * @param data[in] The binary data.  It will be copied.
         * @param data_size[in] The size of data.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_compiled_code(
            const char *data,
            const size_t data_size,
            concurrency::WriterLockToken &token);

        /**
         * Sets the compiled binary code data.
         * This method will automatically get a lock.
         * @param data[in] The binary data.  It will be copied.
         * @param data_size[in] The size of data.
         * @return True if success.
         */
        bool set_compiled_code(
            const char *data,
            const size_t data_size);

        /**
         * @param token[in] The lock token.
         * @return The program language as a string, or empty string if error.
         */
        std::string get_program_language(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The program language as a string, or empty string if error.
         */
        std::string get_program_language(void);

        /**
         * Sets the program language.
         * @param language[in] The program language.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_program_language(
            const std::string &language,
            concurrency::WriterLockToken &token);

        /**
         * Sets the program language.
         * This method will automatically get a lock.
         * @param language[in] The program language.
         * @return True if success.
         */
        bool set_program_language(const std::string &language);


        /**
         * Replaces the current set of program includes with a new set.
         * @param includes[in] The new set of program includes.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool set_program_includes(
            const IdSet &includes,
            concurrency::WriterLockToken &token);

        /**
         * Replaces the current set of program includes with a new set.
         * This method will automatically get a lock.
         * @param includes[in] The new set of program includes.
         * @return True if success.
         */
        bool set_program_includes(
            const IdSet &includes);

        /**
         * @param program_id[in]  The ID of the program to check.
         * @param token[in] The lock token.
         * @return True if the program is in the includes list, false if
         * not or error.
         */
        bool is_program_included(
            const Id &program_id,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param program_id[in]  The ID of the program to check.
         * @return True if the program is in the includes list.
         */
        bool is_program_included(
            const Id &program_id);

        /**
         * Clears the list of programs on the includes list.
         * @param token[in] The lock token.
         * @return True if success.
         */
        bool clear_included_programs(
            concurrency::WriterLockToken &token);

        /**
         * Clears the list of programs on the includes list.
         * This method will automatically get a lock.
         * @return True if success.
         */
        bool clear_included_programs(void);

        /**
         * @param token[in] The lock token.
         * @return The full list of program includes, or empty if error or none.
         */
        IdSet get_program_includes(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The full list of program includes, or empty if error or none.
         */
        IdSet get_program_includes(void);

        /**
         * @param token[in] The lock token.
         * @return The ID of the first program include in the set, or default
         * if none or error.
         */
        Id get_first_program_include(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The ID of the first program include in the set, or default
         * if none or error.
         */
        Id get_first_program_include(void);

        /**
         * @param program_id[in] The current position in the program include set.
         * @param token[in] The lock token.
         * @return The next ID (after the provided ID) of the first program
         * include in the set, or none if default or error.
         */
        Id get_next_program_include(
            const Id &program_id,
            concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @param program_id[in] The current position in the program include set.
         * @return The next ID (after the provided ID) of the first program
         * include in the set, or default if none or error.
         */
        Id get_next_program_include(const Id &program);

        /**
         * @param token[in] The lock token.
         * @return The ID of the last program include in the set, or default if
         * none or error.
         */
        Id get_last_program_include(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return The ID of the last program include in the set, or default
         * if none or error.
         */
        Id get_last_program_include(void);

        /**
         * @param token[in] The lock token.
         * @return How many program includes there are, or 0 if none or
         * error.
         */
        size_t get_program_include_count(concurrency::ReaderLockToken &token);

        /**
         * This method will automatically get a lock.
         * @return How many program includes there are, or 0 if none or
         * error.
         */
        size_t get_program_include_count(void);

    protected:
        /**
         * Constructs an Entity with a provided type.  Used by subclasses.
         * @param id[in] The ID of the entity.
         * @param type[in] The final type (subclass) the Entity will be.
         * @param version[in] The version # of this Entity.
         * @param instance[in] The instance # of this Entity.
         * @param restoring[in] When true, ignore changes as Entity is being
         * restored.
         *
         */
        Program(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

        /**
         * @return Approximate memory used by this class instance, in bytes,
         * or 0 if error.
         */
        virtual size_t mem_used_fields(void);

        /**
         * Copies fields from this Program to the provided
         * Entity.
         * Subclasses will override this and call their parent, the chain as a
         * whole allowing for an Entity of any type to be copied.  This is a
         * helper method used with clone().
         * The copied fields will be toggled as changed.  Locking is assumed
         * to have already been performed.
         * @param entity_ptr[in,out] The Entity to copy field data into.
         */
        virtual void copy_fields(Entity *entity_ptr);

    private:
        // Binary stored as std::string for ease of serialization and
        // memory management.
        typedef std::string CompiledCode;

// TODO This needs to be significantly enhanced later with regards to not loading the source and how to transfer max length around.

        osinterface::OsTypes::Double program_runtime_sec;///< Cumulative runtime
        std::string program_reg_name; ///< Global registration name, if a library.  Not case sensitive.
        DocumentProperty program_source_code; ///< Source code
        CompiledCode program_compiled_code; ///< Optional compiled opaque binary
        std::string program_language; ///< Code language program is in
        Entity::IdSet program_includes; ///< What other programs this one uses

        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & program_runtime_sec;
            ar & program_reg_name;
            ar & program_source_code;
            ar & program_compiled_code;
            ar & program_language;
            ar & program_includes;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<PropertyEntity>(*this);

            ar & program_runtime_sec;
            ar & program_reg_name;
            ar & program_source_code;
            ar & program_compiled_code;
            ar & program_language;
            ar & program_includes;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */
#endif //MUTGOS_DBTYPE_PROGRAM_H
