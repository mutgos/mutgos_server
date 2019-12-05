/*
 * dbtype_DocumentProperty.h
 */

#ifndef MUTGOS_DBTYPE_DOCUMENTPROPERTY_H_
#define MUTGOS_DBTYPE_DOCUMENTPROPERTY_H_

#include <string>
#include <vector>
#include <stddef.h>

#include "osinterface/osinterface_OsTypes.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "dbtypes/dbtype_PropertyData.h"
#include "dbtypes/dbtype_PropertyDataType.h"


namespace mutgos
{
namespace dbtype
{
    /**
     * A DocumentProperty is similar to a StringProperty, except it can be
     * accessed at the line level.  Basically, an array of strings.  Also,
     * each string entry can be longer/bigger.
     *
     * Strings are pointers for faster manipulation.
     *
     * The line numbers start at 0, like any normal vector.
     */
    class DocumentProperty : public PropertyData
    {
    public:
        /** Internal storage structure of a Document. */
        typedef std::vector<std::string *> DocumentData;

        /**
         * Creates an empty Document.
         */
        DocumentProperty();

        /**
         * Copy constructor
         * @param data[in] The Document to copy.
         */
        DocumentProperty(const DocumentProperty &data);

        virtual ~DocumentProperty();

        /**
         * Equals operator.
         * @param rhs The source to compare.
         * @return True if contents are equal.
         */
        virtual bool operator==(const PropertyData *rhs) const;

        /**
         * Less than comparison operator.  Primarily used in STL containers.
         * This really shouldn't be used for this property data type, but is here
         * for completeness.
         * @param rhs The source to compare.
         * @return True if the contents of this is < rhs.
         */
        virtual bool operator<(const PropertyData *rhs) const;

        /**
         * Sets the maximum line length.  Set this BEFORE adding any
         * lines.
         * @param max[in] The maximum line length.  Must be > 0.
         */
        void set_max_lines(const osinterface::OsTypes::UnsignedInt max);

        /**
         * Creates a clone of the given property data.  Caller must manage
         * returned pointer.
         * @return A pointer to the newly cloned instance of PropertyData.
         * Caller must manage pointer.
         */
        virtual PropertyData *clone(void) const;

        /**
         * Adds a string at the end of the document.
         * @param data[in] The string to add.
         * @return True if success. If false, it means the document is probably
         * full.
         */
        bool append_line(const std::string &data);

        /**
         * Inserts a string anywhere in the document.
         * @param data[in] The string to add.
         * @param line[in] What line to insert the string at.  If greater
         * than the number of lines, it will be inserted at the end.
         * @return True if success. If false, it means the document is probably
         * full.
         */
        bool insert_line(
            const std::string &data,
            const MG_UnsignedInt line);

        /**
         * Deletes a line anywhere in the document.
         * @param line[in] The line number to delete.  If the line number
         * is greater than the actual number of lines, nothing is deleted.
         * @return True if successfully deleted a line.
         */
        bool delete_line(const MG_UnsignedInt line);

        /**
         * @return The number of lines in this document.
         */
        MG_UnsignedInt get_number_lines(void) const;

        /**
         * @return True if no more lines can be added.
         */
        bool is_full(void) const;

        /**
         * Gets a specific line from the document.
         * @param line[in] The line number to get.
         * @return A reference to the string on that line, or an empty
         * string if line is greater than the length of the document.
         */
        const std::string &get_line(
            const MG_UnsignedInt line) const;

        /**
         * Removes all lines from the document.
         */
        void clear(void);

        /**
         * @return The data contained by this instance as a 'short' string
         * form, which in this case is 60 characters.  Used for
         * condensed output screens.
         */
        virtual std::string get_as_short_string(void) const;

        /**
         * @return The data contained by this instance as a string.  This can
         * be very long for a Document.  A newline will be put at the end of
         * each line.
         */
        virtual std::string get_as_string(void) const;

        /**
         * Sets the string data contained by this instance using a string.
         * Newlines will be converted into new line entries.
         * @param str[in] The data to set, as a string.
         * @return True if successfully set.
         */
        virtual bool set_from_string(const std::string &str);

        /**
         * Sets the string data contained by this instance using a vector
         * of strings.
         * @param data[in] The data to set.
         * @return True if successfully set.
         */
        bool set(const std::vector<std::string> &data);

        /**
         * Sets the string data contained by this instance using another
         * DocumentData instance.  This is for internal use only;
         * no limit checks are performed.
         * @param data[in] The data to set.  It will be copied.
         * @return True if successfully set.
         */
        bool set(const DocumentData &data);

        /**
         * Generally for internal use only; not to be exposed to user code.
         * @return The data contained by this DocumentProperty.
         */
        const DocumentData &get(void) const;

        /**
         * @return The approximate amount of memory used by this
         * DocumentProperty.
         */
        virtual size_t mem_used(void) const;

    protected:
        DocumentData document_data; ///< The array of strings

    private:

        osinterface::OsTypes::UnsignedInt max_lines; ///< Max number of lines

        /**
         * Serialization using Boost Serialization.
         */
        friend class boost::serialization::access;

        /**
         * Serializes the provided DocumentData.
         * @param data[in] The DocumentData to serialize.
         * @param ar[out] The serialization archive to put the serialization
         * data.
         * @param version[in] The version of the archive.
         */
        template<class Archive>
        static inline void save_document_data(
            const DocumentData &data,
            Archive & ar,
            const unsigned int version)
        {
            const size_t size = data.size();
            ar & size;

            for (DocumentData::const_iterator iter = data.begin();
                 iter != data.end();
                 ++iter)
            {
                ar & (**iter);
            }
        }

        /**
         * Deserializes the DocumentData.
         * @param ar[in] The serialization archive to get the data from.
         * @param version[in] The version of the archive.
         * @param data[out] The deserialized DocumentData.
         */
        template<class Archive>
        static inline void load_document_data(
            Archive & ar,
            const unsigned int version,
            DocumentData &data)
        {
            // Clear out data first.
            //
            for (DocumentData::iterator iter = data.begin();
                 iter != data.end();
                 ++iter)
            {
                delete *iter;
            }

            data.clear();

            size_t size = 0;
            ar & size;

            // Deserialize
            //
            for (size_t index = 0; index < size; ++index)
            {
                std::string *item_ptr = new std::string();

                ar & (*item_ptr);
                data.push_back(item_ptr);
            }
        }

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            // serialize base class information
            ar & boost::serialization::base_object<PropertyData>(*this);

            ar & max_lines;
            save_document_data(document_data, ar, version);
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            // serialize base class information
            ar & boost::serialization::base_object<PropertyData>(*this);

            clear();

            ar & max_lines;
            load_document_data(ar, version, document_data);
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER();
        ////
    };
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_DOCUMENTPROPERTY_H_ */
