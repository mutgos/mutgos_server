#ifndef MUTGOS_DBINTERFACE_ENTITYREFCOUNTER_H
#define MUTGOS_DBINTERFACE_ENTITYREFCOUNTER_H

namespace mutgos
{
namespace dbtype
{
    class Entity;
}

    //TODO Document
namespace dbinterface
{
    class EntityRefCounter
    {
    public:
        /**
         * Indicates there is a new reference to the given Entity pointer.
         * @param entity_ptr[in] The Entity pointer.
         */
        virtual void mem_reference_added(const dbtype::Entity *entity_ptr) =0;

        /**
         * Indicates a reference has been removed from the given Entity pointer.
         * @param entity_ptr[in] The Entity pointer.
         */
        virtual void mem_reference_removed(const dbtype::Entity *entity_ptr) =0;

        /**
         * Abstract interface virtual destructor
         */
        virtual ~EntityRefCounter();
    };
}
}
#endif //MUTGOS_SERVER_DBINTERFACE_ENTITYREFCOUNTER_H
