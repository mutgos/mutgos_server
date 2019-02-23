/*
 * dbtype_SetProperty.h
 */

#ifndef MUTGOS_DBTYPE_SETPROPERTY_H_
#define MUTGOS_DBTYPE_SETPROPERTY_H_

#include <stddef.h>
#include <list>
#include <set>
#include <string>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>

namespace mutgos
{
namespace dbtype
{
    /**
     * A property containing a set of simple types (except Document).  This
     * is primarily used for lists of dbrefs, but may have other uses as well.
     * The set can only contain a single type.  That type is set when the
     * first item is added.
     */
    class SetProperty : public PropertyData
    {
    public:
        /**
         * Creates an empty SetProperty.
         */
        SetProperty();

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        SetProperty(const SetProperty &rhs);

        virtual ~SetProperty();

        /**
         * Equals operator.
         * @param rhs[in] The source to compare.
         * @return True if contents are equal.
         */
        virtual bool operator==(const PropertyData *rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * This really shouldn't be used for this property data type, but is here
         * for completeness.
         * @param rhs[in] The source to compare.
         * @return True if the contents of this is < rhs.
         */
        virtual bool operator<(const PropertyData *rhs) const;

        /**
         * Creates a clone of the given property data.  Caller must manage
         * returned pointer.
         * @return A pointer to the newly cloned instance of PropertyData.
         * Caller must manage pointer.
         */
        virtual PropertyData *clone(void) const;

        /**
         * @return The data contained by this instance as a 'short' string
         * form, which in this case is 60 characters.
         */
        virtual std::string get_as_short_string(void) const;

        /**
         * @return The data contained by this instance as a string.
         */
        virtual std::string get_as_string(void) const;

        /**
         * This is not currently supported for this property type.
         * @param str[in] The data to set, as a string.
         * @return False since it can't be set this way.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * @return The approximate amount of memory used by this
         * SetProperty.
         */
        virtual size_t mem_used(void) const;


        /**
         * Removes everything in the set.
         */
        void clear(void);

        /**
         * Adds an item to the set.  If this is the first item, it will
         * determine the type of items the set can hold.
         * @param item[in] The item to add.  A copy will be made.
         * @return True if successfully added.  Returns false if full or
         * type mismatch.
         */
        bool add(PropertyData &item);

        /**
         * Removes an item from the set, if found.
         * Important:  If you are using an item already in the set to remove
         * itself, that item will become invalid when this method completes.
         * @param item[in] The item to remove.  It does not have to be the
         * exact item, just as long as it is ==.
         * When all items have been removed, the set may accept a new type.
         * @return True if removed (or if not found), false if a type mismatch.
         */
        bool remove(PropertyData &item);

        /**
         * Determines if the set contains a particular item.
         * @param item[in] The item to check for.
         * @return True if the set contains the item, false if not.
         */
        bool contains(const PropertyData &item) const;

        /**
         * @return The number of items in the set.
         */
        size_t size(void) const
        {
            return property_data_set.size();
        }

        /**
         * @return True if this set is full (cannot add any more items).
         */
        bool is_full(void) const;

        /**
         * @return The type contained by the set, or PROPERTYDATATYPE_invalid
         * if empty.
         */
        PropertyDataType get_contained_type(void) const
        {
            return property_data_set_type;
        }

        /**
         * Do not delete the pointer!
         * @return A pointer to the 'first' item in the set, or null if empty.
         */
        const PropertyData *iter_first(void) const;

        /**
         * Do not delete the pointer!
         * @return A pointer to the 'last' item in the set, or null if empty.
         */
        const PropertyData *iter_last(void) const;

        /**
         * Given an item (typically from the iter_ methods), return the
         * item adjacent to it, moving forward.
         * This is a somewhat expensive call, O(log n), but allows for it to
         * be thread safe (in containing classes).
         * Do not delete the pointer!
         * @param data[in] A data item in this set.
         * @return Null if the data item is not actually in the set, or
         * is at the end.  Otherwise, returns a pointer to the next data item.
         */
        const PropertyData *iter_next(const PropertyData *data) const;

        /**
         * Given an item (typically from the iter_ methods), return the
         * item adjacent to it, moving backwards.
         * This is a somewhat expensive call, O(log n).
         * Do not delete the pointer!
         * @param data[in] A data item in this set.
         * @return Null if the data item is not actually in the set, or
         * is at the beginning.  Otherwise, returns a pointer to the previous
         * data item.
         */
        const PropertyData *iter_previous(const PropertyData *data) const;

    protected:
        class SetPropertyComp
        {
        public:
          bool operator()(const PropertyData *lhs,
                          const PropertyData *rhs) const
          {
            return (*lhs) < (*rhs);
          }
        };

        typedef std::set<const PropertyData *,
                         SetPropertyComp> PropertyDataSet;

        PropertyDataSet property_data_set; ///< Contains the data entries
        PropertyDataType property_data_set_type; ///< The type of data entry

    private:
        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;
        template<class Archive>
        inline void save(Archive & ar, const unsigned int version) const;

        template<class Archive>
        inline void load(Archive & ar, const unsigned int version);
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////////////////
    };

} /* namespace dbtype */
} /* namespace mutgos */

// Prevents circular reference
#include "dbtypes/dbtype_PropertyDataSerializer.h"

namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    template<class Archive>
    inline void SetProperty::save(Archive & ar, const unsigned int version) const
    {
        ar & boost::serialization::base_object<PropertyData>(*this);

        ar & property_data_set_type;

        // Save everything in the set.
        // First save how many items are there.
        const MG_UnsignedInt data_set_size =
            (MG_UnsignedInt) property_data_set.size();
        ar & data_set_size;

        for (PropertyDataSet::const_iterator
                iter = property_data_set.begin();
            iter != property_data_set.end();
            ++iter)
        {
            PropertyDataSerializer::save((*iter), ar, version);
        }
    }

    // ----------------------------------------------------------------------
    template<class Archive>
    inline void SetProperty::load(Archive & ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<PropertyData>(*this);

        ar & property_data_set_type;

        MG_UnsignedInt set_size = 0;

        ar & set_size;

        if (set_size)
        {
            PropertyData *data_ptr = 0;

            for (MG_UnsignedInt index = 0; index < set_size; ++index)
            {
                data_ptr = PropertyDataSerializer::load(ar, version);

                if (data_ptr)
                {
                    property_data_set.insert(data_ptr);
                }
            }
        }
    }
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_SETPROPERTY_H_ */
