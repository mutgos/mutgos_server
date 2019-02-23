/*
 * json_JsonUtilities.cpp
 */

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "logging/log_Logger.h"

#include "utilities/json_JsonUtilities.h"
#include "utilities/json_JsonParsedObject.h"

namespace mutgos
{
namespace json
{
    // ----------------------------------------------------------------------
    JsonParsedObject *parse_json(char * const data_ptr)
    {
        // TODO Fix to have an allocator with a max size to prevent DoS OOM attacks
        JsonParsedObject *parsed_result = 0;

        if (data_ptr)
        {
            bool threw_exception = false;
            rapidjson::Document * const doc = new rapidjson::Document();

            try
            {
                doc->ParseInsitu(data_ptr);
            }
            catch (...)
            {
                LOG(warning, "json", "parse_json",
                    "ParseInsitu threw exception!");
                threw_exception = true;
            }

            if (doc->HasParseError() or threw_exception)
            {
                // Did not parse correctly, so ignore.
                //
                delete [] data_ptr;
                delete doc;

                LOG(warning, "json", "parse_json",
                    "Invalid JSON parse attempted!");
            }
            else
            {
                parsed_result = new JsonParsedObject(doc, data_ptr);
            }
        }

        return parsed_result;
    }

    // ----------------------------------------------------------------------
    std::string write_json(JSONRoot &root)
    {
        std::string output;

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        root.Accept(writer);

        output.assign(buffer.GetString());

        return output;
    }
}
}
