/*
 * dbtype_PropertyDataSerializer.h
 */

#ifndef MUTGOS_DBTYPE_PROPERTYDATASERIALIZER_H_
#define MUTGOS_DBTYPE_PROPERTYDATASERIALIZER_H_

#include <stddef.h>
#include <string>

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>

#include "text/text_StringConversion.h"

#include "logging/log_Logger.h"

#include "dbtypes/dbtype_PropertyDataType.h"

#include "dbtypes/dbtype_PropertyData.h"

#include "dbtypes/dbtype_BooleanProperty.h"
#include "dbtypes/dbtype_DocumentProperty.h"
#include "dbtypes/dbtype_FloatProperty.h"
#include "dbtypes/dbtype_IdProperty.h"
#include "dbtypes/dbtype_IntegerProperty.h"
#include "dbtypes/dbtype_StringProperty.h"

namespace mutgos
{
namespace dbtype
{
    // Some static methods to aid in serialization / deserialization of
    // PropertyData instances.  Used with boost serialization.
    //
    class PropertyDataSerializer
    {
    public:
        /**
         * Serializes the provided PropertyData.
         * @param data_ptr[in] The pointer of the PropertyData to serialize.
         * @param ar[out] The serialization archive to put the serialization
         * data.
         * @param version[in] The version of the archive.
         */
        template<class Archive>
        static inline void save(
            const PropertyData *data_ptr,
            Archive & ar,
            const unsigned int version);

        /**
         * Deserializes the PropertyData and returns it.  Caller must manage
         * pointer!
         * @param ar[in] The serialization archive to get the data from.
         * @param version[in] The version of the archive.
         * @return The deserialized PropertyData as a pointer.  Caller must
         * manage the pointer.  Null if error.
         */
        template<class Archive>
        static inline PropertyData *load(
            Archive & ar,
            const unsigned int version);

    private:
        // Static only
        PropertyDataSerializer();
        ~PropertyDataSerializer();
    };
} /* namespace dbtype */
} /* namespace mutgos */

// Put any property data types that can contain other data types here.
// Prevents circular reference.
//
#include "dbtypes/dbtype_SetProperty.h"


namespace mutgos
{
namespace dbtype
{
    // ----------------------------------------------------------------------
    template<class Archive>
    inline void PropertyDataSerializer::save(
        const PropertyData *data_ptr,
        Archive & ar,
        const unsigned int version)
    {
        if (not data_ptr)
        {
            LOG(fatal, "dbtypes", "save()",
                "data_ptr is null!  Will not deserialize properly.");

            return;
        }

        const PropertyDataType type = data_ptr->get_data_type();
        ar & type;

        switch (data_ptr->get_data_type())
        {
            case PROPERTYDATATYPE_boolean:
            {
                ar & (*(static_cast<const BooleanProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_document:
            {
                ar & (*(static_cast<const DocumentProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_float:
            {
                ar & (*(static_cast<const FloatProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_id:
            {
                ar & (*(static_cast<const IdProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_integer:
            {
                ar & (*(static_cast<const IntegerProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_set:
            {
                ar & (*(static_cast<const SetProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_string:
            {
                ar & (*(static_cast<const StringProperty *>(data_ptr)));
                break;
            }

            case PROPERTYDATATYPE_invalid:
            {
                LOG(fatal, "dbtypes", "serialize()",
                    "data_ptr type is invalid!  "
                    "Will not deserialize properly.");
                break;
            }

            default:
            {
                std::string enum_string = "-1";

                enum_string = text::to_string(
                    (int) data_ptr->get_data_type());

                LOG(fatal, "dbtypes", "save()",
                    "data_ptr type is unknown:  " + enum_string +
                    ".   Will not deserialize properly.");
            }
        }
    }

    // ----------------------------------------------------------------------
    template<class Archive>
    inline PropertyData *PropertyDataSerializer::load(
        Archive & ar,
        const unsigned int version)
    {
        PropertyDataType type = PROPERTYDATATYPE_invalid;
        ar & type;

        PropertyData *data_ptr = 0;

        switch (type)
        {
            case PROPERTYDATATYPE_boolean:
            {
                BooleanProperty *bool_ptr = new BooleanProperty();
                ar & (*bool_ptr);
                data_ptr = bool_ptr;
                break;
            }

            case PROPERTYDATATYPE_document:
            {
                DocumentProperty *doc_ptr = new DocumentProperty();
                ar & (*doc_ptr);
                data_ptr = doc_ptr;
                break;
            }

            case PROPERTYDATATYPE_float:
            {
                FloatProperty *float_ptr = new FloatProperty();
                ar & (*float_ptr);
                data_ptr = float_ptr;
                break;
            }

            case PROPERTYDATATYPE_id:
            {
                IdProperty *id_ptr = new IdProperty();
                ar & (*id_ptr);
                data_ptr = id_ptr;
                break;
            }

            case PROPERTYDATATYPE_integer:
            {
                IntegerProperty *int_ptr = new IntegerProperty();
                ar & (*int_ptr);
                data_ptr = int_ptr;
                break;
            }

            case PROPERTYDATATYPE_set:
            {
                SetProperty *set_ptr = new SetProperty();
                ar & (*set_ptr);
                data_ptr = set_ptr;
                break;
            }

            case PROPERTYDATATYPE_string:
            {
                StringProperty *string_ptr = new StringProperty();
                ar & (*string_ptr);
                data_ptr = string_ptr;
                break;
            }

            case PROPERTYDATATYPE_invalid:
            {
                LOG(fatal, "dbtypes", "load()",
                    "Type is invalid!  Skipping.");
                break;
            }

            default:
            {
                std::string enum_string = "-1";

                enum_string = text::to_string(
                    type);

                LOG(fatal, "dbtypes", "load()",
                    "Type is unknown:  " + enum_string +
                    ".  A crash is likely to follow.");
            }
        }

        return data_ptr;
    }
} /* namespace dbtype */
} /* namespace mutgos */

#endif /* MUTGOS_DBTYPE_PROPERTYDATASERIALIZER_H_ */
