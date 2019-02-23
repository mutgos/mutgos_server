/*
 * angelscript_SimpleGCObject.h
 */

#ifndef MUTGOS_ANGELSCRIPT_SIMPLEGCOBJECT_H
#define MUTGOS_ANGELSCRIPT_SIMPLEGCOBJECT_H

#include <angelscript.h>

namespace mutgos
{
namespace angelscript
{
    /**
     * Abstract class used to handle common AngelScript garbage collection
     * methods.
     *
     * As this is a 'simple' version, it assumes instance has no references
     * to any other instances of any type.
     */
    class SimpleGCObject
    {
    public:
        /**
         * Required virtual destructor.
         */
        virtual ~SimpleGCObject();

        /**
         * Indicates a reference to this instance has been added within
         * AngelScript.
         */
        void add_ref(void);

        /**
         * Indicates a references to this instance has been removed within
         * AngelScript.  When the reference count reaches 0, this class
         * will automatically delete itself.
         */
        void release_ref(void);

        /**
         * @return How many references there are to this object instance.
         */
        int get_ref_count(void);

        /**
         * Called by AngelScript to indicate object is to be garbage collected.
         */
        void set_gc_flag(void);

        /**
         * @return True if this object is to be garbage collected.
         */
        bool get_gc_flag(void);

        /**
         * Called to determine what references to other objects this instance
         * has.
         * NOTE: This implementation assumes no other references.
         * @param engine_ptr[in] The script engine pointer.
         */
        void enum_references(asIScriptEngine *engine_ptr);

        /**
         * Called when we must release all references to other objects.  This
         * is called just prior to being destructed and our own ref count going
         * to zero.
         * NOTE: This implementation assumes no other references.
         * @param engine_ptr[in] The script engine pointer.
         */
        void release_all_references(asIScriptEngine *engine_ptr);

    protected:
        /**
         * Constructor that initializes this class.
         * @param engine[in] The AngelScript engine associated with this
         * instance.
         * @param type[in] The type of class being registered with the
         * garbage collector.
         * @param register_with_gc[in] True to register with the garbage
         * collector (default), false to not register.
         */
        SimpleGCObject(
            asIScriptEngine *engine,
            const std::string &type,
            bool register_with_gc = true);

        /**
         * Constructor that initializes this class based on selected data
         * from another instance.
         * @param rhs[in] The instance to copy from.
         * @param type[in] The type of class being registered with the
         * garbage collector.
         * @param register_with_gc[in] True to register with the garbage
         * collector (default), false to not register.
         */
        SimpleGCObject(
            const SimpleGCObject &rhs,
            const std::string &type,
            bool register_with_gc = true);

        asIScriptEngine * const engine_ptr; ///< Pointer to the AngelScript Engine associated with this instance

    private:
        int ref_count; ///< How many references to this String
        bool gc_flag; ///< True if garbage collected


        // Nothing in this class can be copied since that would mess up
        // reference counting.  The contents in subclasses may be copied,
        // however.
        SimpleGCObject &operator=(const SimpleGCObject &rhs);
        SimpleGCObject(const SimpleGCObject &rhs);
    };
}
}

#endif //MUTGOS_ANGELSCRIPT_SIMPLEGCOBJECT_H
