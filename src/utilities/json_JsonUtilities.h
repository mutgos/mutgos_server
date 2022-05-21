/*
 * json_JsonUtilities.h
 */

#ifndef MUTGOS_JSON_JSONUTILITIES_H
#define MUTGOS_JSON_JSONUTILITIES_H

#include <unistd.h>
#include <string>
#include <string.h>

#include <rapidjson/document.h>

#include "osinterface/osinterface_OsTypes.h"

/**
 * Creates a JSON array root node, named varname.
 * Example:  JSON_MAKE_ARRAY_ROOT(my_array);
 */
#ifndef JSON_MAKE_ARRAY_ROOT
#define JSON_MAKE_ARRAY_ROOT(varname) \
        mutgos::json::JSONRoot varname(rapidjson::kArrayType)
#endif

/**
 * Creates a JSON array root node pointer.
 * Example:  JSON_MAKE_ARRAY_ROOT_PTR();
 */
#ifndef JSON_MAKE_ARRAY_ROOT_PTR
#define JSON_MAKE_ARRAY_ROOT_PTR() \
        new mutgos::json::JSONRoot(rapidjson::kArrayType)
#endif

/**
 * Creates a JSON array node, named varname.
 * Example:  JSON_MAKE_ARRAY_NODE(my_array);
 */
#ifndef JSON_MAKE_ARRAY_NODE
#define JSON_MAKE_ARRAY_NODE(varname) \
        mutgos::json::JSONNode varname(rapidjson::kArrayType)
#endif

/**
 * Creates a JSON array node pointer.
 * Example:  JSON_MAKE_ARRAY_NODE();
 */
#ifndef JSON_MAKE_ARRAY_NODE_PTR
#define JSON_MAKE_ARRAY_NODE_PTR() \
        new mutgos::json::JSONNode(rapidjson::kArrayType)
#endif

/**
 * Creates a JSON map (object) node, named varname.
 * Example:  JSON_MAKE_MAP_NODE(my_object);
 */
#ifndef JSON_MAKE_MAP_NODE
#define JSON_MAKE_MAP_NODE(varname) \
        mutgos::json::JSONNode varname(rapidjson::kObjectType)
#endif

/**
 * Creates a JSON map (object) node pointer.
 * Example:  JSON_MAKE_MAP_NODE();
 */
#ifndef JSON_MAKE_MAP_NODE_PTR
#define JSON_MAKE_MAP_NODE_PTR() \
        new mutgos::json::JSONNode(rapidjson::kObjectType)
#endif

/**
 * Creates a JSON map (object) root node, named varname.
 * Example:  JSON_MAKE_MAP_ROOT(my_object);
 */
#ifndef JSON_MAKE_MAP_ROOT
#define JSON_MAKE_MAP_ROOT(varname) \
        mutgos::json::JSONRoot varname(rapidjson::kObjectType)
#endif

/**
 * Creates a JSON map (object) root node pointer.
 * Example:  JSON_MAKE_MAP_NODE();
 */
#ifndef JSON_MAKE_MAP_ROOT_PTR
#define JSON_MAKE_MAP_ROOT_PTR() \
        new mutgos::json::JSONRoot(rapidjson::kObjectType)
#endif

/**
 * Creates a standard JSON node (non-array, non-map), named varname.
 * This is not normally needed, since the add_* methods auto-create these.
 * Example:  JSON_MAKE_NODE(my_node);
 */
#ifndef JSON_MAKE_NODE
#define JSON_MAKE_NODE(varname) \
        mutgos::json::JSONNode varname
#endif

// TODO Nulls in strings are not well supported when parsing.  Will need to add new methods here, and modify any callers of existing, affected methods.

namespace mutgos
{
namespace json
{
    // Forward declarations
    //
    class JsonParsedObject;

    //
    // Use these typedefs when referring to any JSON object; never refer to
    // the underlying JSON parser library directly to maintain portability.
    //

    /** A JSON Node, which could be any type */
    typedef rapidjson::Value JSONNode;
    /** The root of the JSON document tree */
    typedef rapidjson::Document JSONRoot;

    //
    // Static methods to make working with the JSON library easier and portable.
    //

    /**
     * Parses the provided string as JSON.
     * @param data_ptr[in] Pointer to the JSON data as a string.  Ownership
     * of the pointer will pass to this method.  The data will be modified.
     * @return A pointer to the parsed JSON, or null if error or invalid JSON
     * string.  Caller must manage the pointer.
     */
    JsonParsedObject *parse_json(char * const data_ptr);

    /**
     * Converts the document into a JSON string.
     * @param root[in] The document root to convert to a JSON string.
     * @return The document as a JSON string.
     */
    std::string write_json(JSONRoot &root);

    /**
     * Clears an array or map of all contents.
     * @param array[out] The array or map to clear.
     */
    static void json_clear(JSONNode &json)
    {
        if (json.IsObject())
        {
            json.RemoveAllMembers();
        }
        else if (json.IsArray())
        {
            json.Clear();
        }
    }

    /**
     * Adds key value pair to json.  The value must be static and never
     * modified (in other words, a program constant).
     * @param key[in] The key.  Must be const static.
     * @param value[in] The const static value.
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_static_value(
        const std::string &key,
        const std::string &value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                rapidjson::StringRef(value.c_str(), value.size()),
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key.  Must be const static.
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        const std::string &value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value val;
            val.SetString(value.c_str(), value.size(), root.GetAllocator());

            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                val,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key (will be copied).
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_key_value(
        const std::string &key,
        const std::string &value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value keyVal;
            rapidjson::Value val;

            keyVal.SetString(key.c_str(), key.size());
            val.SetString(value.c_str(), value.size());

            json.AddMember(keyVal, val, root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key (will be copied).
     * @param value[in,out] The value (will be moved into json).  When done,
     * this will be empty/unset.
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type or value
     * is unset, or key is empty.
     */
    static bool add_key_value(
        const std::string &key,
        JSONNode &value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty())
             and (not value.IsNull());

        if (success)
        {
            rapidjson::Value keyVal;

            keyVal.SetString(key.c_str(), key.size());

            json.AddMember(keyVal, value, root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key (will be copied).
     * @param value[in,out] The value (will be moved into json).  When done,
     * this will be empty/unset.
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type or value
     * is unset, or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        JSONNode &value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty())
            and (not value.IsNull());

        if (success)
        {
            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                value,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key.  Must be const static.
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        const bool value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value val;

            val.SetBool(value);

            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                val,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key.  Must be const static.
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        const MG_UnsignedInt value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value val;

            val.SetUint(value);

            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                val,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key.  Must be const static.
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        const MG_VeryLongUnsignedInt value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value val;

            val.SetInt64(value);

            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                val,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds key value pair to json.
     * @param key[in] The key.  Must be const static.
     * @param value[in] The value (will be copied).
     * @param json[out] The section of JSON being added to.
     * @param root[in] The document root.
     * @return True if success, false if json isn't a map/object type
     * or key is empty.
     */
    static bool add_static_key_value(
        const std::string &key,
        const MG_SignedInt value,
        JSONNode &json,
        JSONRoot &root)
    {
        const bool success = json.IsObject() and (not key.empty());

        if (success)
        {
            rapidjson::Value val;

            val.SetInt(value);

            json.AddMember(
                rapidjson::StringRef(key.c_str(), key.size()),
                val,
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Gets a string from json.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The value associated with the key.  It will
     * be empty if the return value is false.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        std::string &value)
    {
        bool success = false;

        value.clear();

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if ((iter != json.MemberEnd()) and iter->value.IsString())
            {
                value.assign(
                    iter->value.GetString(),
                    iter->value.GetStringLength());

                success = true;
            }
        }

        return success;
    }

    /**
     * Gets an unsigned int from json.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The value associated with the key.  It will
     * be 0 if the return value is false.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        MG_UnsignedInt &value)
    {
        bool success = false;

        value = 0;

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if ((iter != json.MemberEnd()) and iter->value.IsUint())
            {
                value = iter->value.GetUint();
                success = true;
            }
        }

        return success;
    }

    /**
     * Gets a very long unsigned int from json.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The value associated with the key.  It will
     * be 0 if the return value is false.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        MG_VeryLongUnsignedInt &value)
    {
        bool success = false;

        value = 0;

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if ((iter != json.MemberEnd()) and iter->value.IsUint64())
            {
                value = iter->value.GetUint64();
                success = true;
            }
        }

        return success;
    }

    /**
     * Gets an int from a json.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The value associated with the key.  It will
     * be 0 if the return value is false.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        MG_SignedInt &value)
    {
        bool success = false;

        value = 0;

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if ((iter != json.MemberEnd()) and iter->value.IsInt())
            {
                value = iter->value.GetInt();
                success = true;
            }
        }

        return success;
    }

    /**
     * Gets a bool from json.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The value associated with the key.  It will
     * be false if the return value is false.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        bool &value)
    {
        bool success = false;

        value = false;

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if ((iter != json.MemberEnd()) and iter->value.IsBool())
            {
                value = iter->value.GetBool();
                success = true;
            }
        }

        return success;
    }

    /**
     * Gets a node from json.  This is generally used to retrieve arrays
     * or objects/maps.
     * @param key[in] The key to retrieve.
     * @param json[in] The JSON the key is retrieved from.
     * @param value[out] The pointer to the node.  Ownership of the pointer
     * does NOT transfer to the caller (do not delete it).  It will be set
     * to null if this method returns null.
     * @return True if found and able to retrieve the value, false if
     * not found or of the wrong type.
     */
    static bool get_key_value(
        const std::string &key,
        const JSONNode &json,
        const JSONNode *&value)
    {
        bool success = false;

        value = 0;

        if (json.IsObject())
        {
            rapidjson::Value::ConstMemberIterator iter =
                json.FindMember(key.c_str());

            if (iter != json.MemberEnd())
            {
                value = &(iter->value);
                success = true;
            }
        }

        return success;
    }

    /**
     * @param json[in] The JSON node to check.
     * @return True if node is a map.
     */
    static bool is_map(const JSONNode &json)
    {
        return json.IsObject();
    }

    /**
     * @param json[in] The JSON node to check.
     * @return True if node is an array.
     */
    static bool is_array(const JSONNode &json)
    {
        return json.IsArray();
    }

    /**
     * @param array[in] The array to check.
     * @return True if JSON array is empty.
     */
    static bool array_empty(const JSONNode &array)
    {
        bool empty = true;

        if (is_array(array))
        {
            empty = array.Empty();
        }

        return empty;
    }

    /**
     * @param array[in] The array to check.
     * @return The size of the JSON array.
     */
    static MG_UnsignedInt array_size(const JSONNode &array)
    {
        MG_UnsignedInt size = 0;

        if (is_array(array))
        {
            size = array.Size();
        }

        return size;
    }

    /**
     * Adds non-static string value to an array.
     * @param value[in] The value to add.
     * @param array[out] The JSON Array to add the value to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array.
     */
    static bool array_add_value(
        const std::string &value,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray();

        if (success)
        {
            rapidjson::Value val;
            val.SetString(value.c_str(), value.size());

            array.PushBack(val, root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds a value to an array.
     * @param value[in] The value to add.
     * @param array[out] The JSON Array to add the value to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array.
     */
    static bool array_add_value(
        const osinterface::OsTypes::UnsignedInt value,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray();

        if (success)
        {
            rapidjson::Value val;
            val.SetUint(value);

            array.PushBack(val, root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds non-static string values to an array.
     * @tparam Container std::list of std::string, std::set, and the like.
     * @param values[in] The string values to add.
     * @param array[out] The JSON Array to add the values to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array.
     */
    template <typename Container>
    static bool array_add_value(
        const Container &values,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray();

        if (success)
        {
            rapidjson::Value val;

            for (typename Container::const_iterator iter = values.begin();
                iter != values.end();
                ++iter)
            {
                val.SetString(iter->c_str(), iter->size());
                array.PushBack(val, root.GetAllocator());
            }
        }

        return success;
    }

    /**
     * Adds a non-static JSON node to the array.
     * @param value[in,out] The value to add.  The value itself will be cleared.
     * @param array[out] The array to add the value to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array or unable to add value.
     */
    static bool array_add_node(
        JSONNode &value,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray() and not value.IsNull();

        if (success)
        {
            array.PushBack(value, root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds static (never changes, program constant) string value to an array.
     * @param value[in] The value to add.
     * @param array[out] The JSON Array to add the value to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array.
     */
    static bool array_add_static_value(
        const std::string &value,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray();

        if (success)
        {
            array.PushBack(
                rapidjson::StringRef(value.c_str(), value.size()),
                root.GetAllocator());
        }

        return success;
    }

    /**
     * Adds const static string values to an array.
     * @tparam Container std::list of std::string *, std::set, and the like.
     * @param values[in] The pointers to const static string values to add.
     * Pointer ownership does NOT transfer to this method.
     * @param array[out] The JSON Array to add the values to.
     * @param root[in] The document root.
     * @return True if success, or false if not an array.
     */
    template <typename Container>
    static bool array_add_static_value(
        const Container &values,
        JSONNode &array,
        JSONRoot &root)
    {
        const bool success = array.IsArray();

        if (success)
        {
            for (typename Container::const_iterator iter = values.begin();
                 iter != values.end();
                 ++iter)
            {
                array.PushBack(
                    rapidjson::StringRef((*iter)->c_str(), (*iter)->size()),
                    root.GetAllocator());
            }
        }

        return success;
    }

    /**
     * Gets an array of strings from a JSON array. The strings will be copied.
     * Non-string elements will be filtered out.
     * @tparam Container std::list of std::string, std::set, and the like.
     * @param array[in] The array to retrieve the strings from.
     * @param value[out] Where to place the strings retrieved from array.
     * The contents will NOT be cleared success or fail.
     * @return True if an array and able to retrieve values.  Note this will
     * return true even if the array is empty.
     */
    template <typename Container>
    static bool array_get_value(
        const JSONNode &array,
        Container &value)
    {
        const bool success = array.IsArray();

        if (success)
        {
            for (JSONNode::ConstValueIterator iter = array.Begin();
                iter != array.End();
                ++iter)
            {
                if (iter->IsString())
                {
                    value.push_back(std::string(
                        iter->GetString(),
                        iter->GetStringLength()));
                }
            }
        }

        return success;
    }

    /**
     * Gets a string from a specific location in an array.  The string will
     * be copied.
     * @param array[in] The array to retrieve the string from.
     * @param index[in] The index of the string in the array to retrieve.
     * @param value[out] The value of the array element.  The value will be
     * cleared before populating, success or fail.
     * @return True if an array, the index is within range, the value is
     * of type string, and able to retrieve the value.  False otherwise.
     */
    static bool array_get_value(
        const JSONNode &array,
        const ssize_t index,
        std::string &value)
    {
        bool success = array.IsArray() and (array.Size() > index);

        value.clear();

        if (success)
        {
            const JSONNode &element = array[index];

            if (element.IsString())
            {
                value.assign(element.GetString(), element.GetStringLength());
            }
            else
            {
                success = false;
            }
        }

        return success;
    }

    /**
     * Gets a string from a specific location in an array.  The string will
     * be copied.
     * Note this method is generally used for specific performance reasons;
     * the standard std::string variant is preferred for most use cases.
     * @param array[in] The array to retrieve the string from.
     * @param index[in] The index of the string in the array to retrieve.
     * @param value_ptr[out] If returning true, this will contain a pointer
     * to the string data.  Caller must manage the pointer!  If false is
     * returned, this will be null.
     * @param value_size[out] The length of value, excluding the final null.
     * As strings are permitted to contain nulls, this is the most accurate
     * size of the string.
     * @return True if an array, the index is within range, the value is
     * of type string, and able to retrieve the value.  False otherwise.
     */
    static bool array_get_value(
        const JSONNode &array,
        const ssize_t index,
        char *&value_ptr,
        size_t &value_size)
    {
        bool success = array.IsArray() and (array.Size() > index);

        value_ptr = 0;

        if (success)
        {
            const JSONNode &element = array[index];

            if (element.IsString())
            {
                value_size = element.GetStringLength();
                value_ptr = new char[value_size + 1];
                memcpy(value_ptr, element.GetString(), value_size);
                value_ptr[value_size] = 0;
            }
            else
            {
                success = false;
            }
        }

        return success;
    }

    /**
     * Gets a value from a specific location in an array.  The value will
     * be copied.
     * @param array[in] The array to retrieve the string from.
     * @param index[in] The index of the string in the array to retrieve.
     * @param value[out] The value of the array element.  The value will be
     * cleared before populating, success or fail.
     * @return True if an array, the index is within range, the value is
     * the proper type, and able to retrieve the value.  False otherwise.
     */
    static bool array_get_value(
        const JSONNode &array,
        const ssize_t index,
        osinterface::OsTypes::UnsignedInt &value)
    {
        bool success = array.IsArray() and (array.Size() > index);
        value = 0;

        if (success)
        {
            const JSONNode &element = array[index];

            if (element.IsUint())
            {
                value = element.GetUint();
            }
            else
            {
                success = false;
            }
        }

        return success;
    }

    /**
     * Gets a JSON node from a specific location in an array.
     * @param array[in] The array to retrieve the node from.
     * @param index[in] The index of the node in the array to retrieve.
     * @param value[out] The pointer to the node.  Ownership of the pointer
     * does NOT transfer to the caller (do not delete it).  It will be set
     * to null if this method returns null.
     * @return True if an array and the index is within range, and able to
     * retrieve the value.  False otherwise.
     */
    static bool array_get_node(
        const JSONNode &array,
        const ssize_t index,
        const JSONNode *&value)
    {
        const bool success = array.IsArray() and (array.Size() > index);

        value = 0;

        if (success)
        {
            value = &array[index];
        }

        return success;
    }

    // TODO array get/add bool, uint, int, ID
    // TODO array helpers for list/set of IDs
}
}

#endif //MUTGOS_JSON_JSONUTILITIES_H
