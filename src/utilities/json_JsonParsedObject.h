/*
 * json_JsonParsedObject.h
 */

#ifndef MUTGOS_JSON_JSONPARSEDOBJECT_H
#define MUTGOS_JSON_JSONPARSEDOBJECT_H

#include "utilities/json_JsonUtilities.h"

namespace mutgos
{
namespace json
{
    /**
     * Contains a parsed JSON and any additional data structures to support it.
     * Normally this is instantiated on the heap and passed around.
     * This is not thread safe if contents is intended to be modified.
     */
    class JsonParsedObject
    {
    public:
        /**
         * Creates a JsonObject with parsed JSON information.
         * @param dom_ptr[in] The pointer to a JSON DOM document.  Must not be
         * null or the program will crash later on.  Pointer ownership
         * transfers to this class.
         * @param json_str_ptr[in] An optional raw string pointer that will
         * be kept with the DOM pointer.  This is used when insitu parsing
         * was performed.  Pointer ownership transfers to this class.
         */
        JsonParsedObject(JSONRoot *dom_ptr, char * const json_str_ptr = 0)
          : document_ptr(dom_ptr),
            string_ptr(json_str_ptr)
          {  }

        /**
         * Destructor to delete stored pointers.
         */
        ~JsonParsedObject()
        {
            delete document_ptr;
            delete [] string_ptr;
        }

        /**
         * @return The JSON DOM document.
         */
        JSONRoot &get(void) const
          { return *document_ptr; }

    private:
        // No copying
        JsonParsedObject(const JsonParsedObject &rhs);
        JsonParsedObject &operator=(const JsonParsedObject &rhs);

        JSONRoot * const document_ptr; ///< JSON as a DOM document
        char * const string_ptr; ///< JSON raw string, for insitu document parsing
    };
}
}

#endif //MUTGOS_JSON_JSONPARSEDOBJECT_H
