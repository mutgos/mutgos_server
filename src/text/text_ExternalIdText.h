#ifndef MUTGOS_TEXT_EXTERNALIDTEXT_H
#define MUTGOS_TEXT_EXTERNALIDTEXT_H

#include <string>

#include "text/text_ExternalText.h"
#include "dbtypes/dbtype_Id.h"

namespace mutgos
{
namespace text
{
    /**
     * Represents an ID, providing hints as to how to display it and what to
     * do when hovered over, clicked, etc.
     */
    class ExternalIdText : public ExternalText
    {
    public:
        /**
         * Indicates type of Entity referred to by this ExternalIdText.
         * Update ExternalTextConverter if this enum changes.
         * TODO Add a type that indicates 'thing/player/puppet/vehicle'?
         */
        enum IdType
        {
            /** ID represents a standard Entity in the database (not action
                or exit).  Must always be first enum entry. */
            ID_TYPE_ENTITY,
            /** ID represents an action in the database */
            ID_TYPE_ACTION,
            /** ID represents an exit in the database */
            ID_TYPE_EXIT,
            /** Internal use only.  Insert new enums above this. */
            ID_TYPE_END_INVALID
        };

        /**
         * Creates an ID 'text', suitable for deserialization.
         */
        ExternalIdText(void)
            : ExternalText(ExternalText::TEXT_TYPE_ID),
              db_id_type(ID_TYPE_END_INVALID)
        { }

        /**
         * Creates an ID 'text'.
         * @param id[in] The ID of the Entity in the database.
         * @param name[in] The 'name' of the Entity in the database.
         * @param type[in] The type of the entity in the database, used
         * for hinting how the UI should display, react to clicks, etc.
         */
        ExternalIdText(
            const dbtype::Id &id,
            const std::string &name,
            const IdType type)
          : ExternalText(ExternalText::TEXT_TYPE_ID),
            db_id(id),
            db_id_name(name),
            db_id_type(type)
          { }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ExternalIdText(const ExternalIdText &rhs)
            : ExternalText(rhs),
              db_id(rhs.db_id),
              db_id_name(rhs.db_id_name),
              db_id_type(rhs.db_id_type)
        { }

        /**
         * Destructor.
         */
        virtual ~ExternalIdText()
          { }

        /**
         * @return How much memory this ExternalText instance uses.
         */
        virtual size_t mem_used(void) const
        {
            return ExternalText::mem_used() + db_id.mem_used()
                + db_id_name.size() + sizeof(db_id_name);
        }

        /**
         * Creates a copy of this ExternalText.
         * @return A cloned copy.  Caller must manage the pointer.
         */
        virtual ExternalText *clone(void) const
        {
            return new ExternalIdText(*this);
        }

        /**
         * Saves this to the provided JSON node.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

        /**
         * @return The ID.
         */
        const dbtype::Id &get_id(void) const
          { return db_id; }

        /**
         * @return The name of the Entity referred to by the ID.
         */
        const std::string &get_name(void) const
          { return db_id_name; }

        /**
         * @return The type of Entity referred to by the ID.
         */
        const IdType get_type(void) const
          {return db_id_type; }

        /**
         * @return The ExternalText component as a plain string, stripping
         * all metadata or other formatting.
         */
        virtual std::string to_string(void) const
          { return get_name(); }

    private:
        dbtype::Id db_id; ///< ID of Entity in the database
        std::string db_id_name; ///< Name of Entity for quick reference
        IdType db_id_type; ///< Type of entity for display/click/hover
    };
}
}

#endif //MUTGOS_TEXT_EXTERNALIDTEXT_H
