#ifndef MUTGOS_ANGELSCRIPT_AFORMATTEDTEXT_H
#define MUTGOS_ANGELSCRIPT_AFORMATTEDTEXT_H

#include <angelscript.h>

#include "dbtypes/dbtype_Id.h"

#include "angelscript_SimpleGCObject.h"
#include "angelscript_AString.h"
#include "angelscript_AEntity.h"
#include "osinterface/osinterface_OsTypes.h"

#include "text/text_ExternalText.h"

namespace mutgos
{
namespace angelscript
{
    // Forward declarations
    //
    class AEntity;

    /**
     * Represents ExternalText within AngelScript.  This allows the script
     * to build up such a line to take advantage of advanced features.
     *
     * TODO In the future, this may also be used to examine an ExternalText incoming from a client.
     *
     * Note this class is meant exclusively to interface with AngelScript.
     * It is designed to be a wrapper and not reusable.  When instantiating,
     * always do it on the heap.
     *
     * TODO THis has only the most basic features for a demo.  The remaining bits need to be added later.
     */
    class AFormattedText : public SimpleGCObject
    {
    public:
        /**
         * Used by the MUTGOS AngelScript management subsystem to register
         * this class and its methods as an AngelScript class.
         * @param engine[in] The script engine to register with.
         * @return True if success.
         */
        static bool register_methods(asIScriptEngine &engine);

        /**
         * A factory used to create a new instance of an AFormattedText,
         * equivalent to the default constructor.  The created
         * AFormattedText will be empty.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void formatted_text_factory(asIScriptGeneric *gen_ptr);

        /**
         * A factory used to create a copy of an AFormattedText, equivalent
         * to the copy constructor.
         * @param gen_ptr[in,out] Pointer to the generic type info, needed
         * to get the engine and set the pointer to the newly created string.
         */
        static void formatted_text_factory_copy(asIScriptGeneric *gen_ptr);

        /**
         * Constructor that creates an empty instance.
         * @param engine[in] The pointer to the script engine.
         */
        AFormattedText(asIScriptEngine *engine);

        /**
         * Required virtual destructor.
         */
        virtual ~AFormattedText();

        /**
         * Assignment operator.
         * @param rhs[in] The AFormattedText to copy from.
         * @return This.
         */
        AFormattedText &operator=(const AFormattedText &rhs);

        /**
         * Overwrites whatever is in this formatted text with what's supplied.
         * @param text[in] The AFormattedText whose data is to be copied in.
         */
        void assign(const AFormattedText &text);

        /**
         * Parses a string and appends the resulting formatted text.
         * @param str[in] The string to parse and append.
         * @return This AFormattedText.
         */
        AFormattedText &append_formatted(const AString &str);

        /**
         * Appends a string as-is.
         * @param str[in] The string to append.
         * @return This AFormattedText.
         */
        AFormattedText &append_plain(const AString &str);

        /**
         * Appends an Entity as a formatted ID, if permissions allow.  It will
         * automatically determine the type (again, if permissions allow).
         * @param entity[in] The Entity to append.
         * @return This AFormattedText.
         */
        AFormattedText &append_entity(AEntity &entity);

        /**
         * Transfers the resulting ExternalText (and ownership of the pointers)
         * to the caller.  When done, this AFormattedText will be empty.
         * This is primarily used when moving the text out of the AngelScript
         * virtual heap.
         * @param destination[out] The ExternalText will be appended to the
         * end of this.  Existing elements will not be touched.
         */
        void transfer(text::ExternalTextLine &destination);

        /**
         * Used to allow give other instances of AFormattedText the
         * ability to see the text contained by this one.
         * @return The stored text line.
         */
        const text::ExternalTextLine &get_text_line(void) const
        { return text_line; }

    private:
        /**
         * Checks the return code from registering with AngelScript,
         * logs relevant info if failure, and updates the status flag.
         * @param rc[in] The return code from AngelScript.
         * @param line[in] The line number of the registration call.
         * @param current_result[in,out] The current successful status.  It
         * will be updated to show failure as needed.
         */
        static void check_register_rc(
            const int rc,
            const size_t line,
            bool &current_result);

        text::ExternalTextLine text_line; ///< The line of external text
    };
}
}
#endif //MUTGOS_ANGELSCRIPT_AFORMATTEDTEXT_H
