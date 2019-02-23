/*
 * dbtype_Command.h
 */

#ifndef MUTGOS_DBTYPE_COMMAND_H
#define MUTGOS_DBTYPE_COMMAND_H

#include "dbtype_Entity.h"
#include "dbtype_ActionEntity.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/base_object.hpp>

namespace mutgos
{
namespace dbtype
{
    class Command : public ActionEntity
    {
    public:
        /**
         *  Constructor used for deserialization of a Command.
         */
        Command(void);

        /**
         * Constructs a Command (final type).
         * @param id[in] The ID of the entity.
         */
        Command(const Id &id);

        /**
         * Destructor.
         */
        virtual ~Command();

        /**
         * Creates a copy of this Command.
         * @param id[in] The new ID of the cloned Command.
         * @param version[in] The new version # of this Command.
         * @param instance[in] The new instance # of this Command.
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
        Command(
            const Id &id,
            const EntityType &type,
            const VersionType version,
            const InstanceType instance,
            const bool restoring = false);

    private:
        /**
         * Serialization using Boost Serialization.  MUST be locked externally,
         * however this cannot be checked due to the const requirement.
         */
        friend class boost::serialization::access;
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar & boost::serialization::base_object<ActionEntity>(*this);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar & boost::serialization::base_object<ActionEntity>(*this);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif //MUTGOS_DBTYPE_COMMAND_H
