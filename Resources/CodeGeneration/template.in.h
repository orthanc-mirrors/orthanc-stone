/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/

#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <memory>
#include <optional>
#include <json/json.h>
#include <gtest/gtest.h>

//#define STONEGEN_NO_CPP11 1

#ifdef STONEGEN_NO_CPP11
#define StoneSmartPtr std::auto_ptr
#else 
#define StoneSmartPtr std::unique_ptr
#endif 

namespace {{rootName}}
{
  /** Throws in case of problem */
  void _StoneDeserializeValue(int32_t& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asInt();
  }

  Json::Value _StoneSerializeValue(int32_t value)
  {
    Json::Value result(value);
    return result;
  }

  /** Throws in case of problem */
  void _StoneDeserializeValue(double& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asDouble();
  }

  Json::Value _StoneSerializeValue(double value)
  {
    Json::Value result(value);
    return result;
  }

  /** Throws in case of problem */
  void _StoneDeserializeValue(bool& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asBool();
  }

  Json::Value _StoneSerializeValue(bool value)
  {
    Json::Value result(value);
    return result;
  }

  /** Throws in case of problem */
  void _StoneDeserializeValue(
       std::string& destValue
     , const Json::Value& jsonValue)
  {
    destValue = jsonValue.asString();
  }

  Json::Value _StoneSerializeValue(const std::string& value)
  {
    // the following is better than 
    Json::Value result(value.data(),value.data()+value.size());
    return result;
  }
    
  /** Throws in case of problem */
  template<typename T>
  void _StoneDeserializeValue(
    std::map<std::string, T>& destValue, const Json::Value& jsonValue)
  {
    destValue.clear();
    for (
      Json::Value::const_iterator itr = jsonValue.begin();
      itr != jsonValue.end();
      itr++)
    {
      std::string key;
      _StoneDeserializeValue(key, itr.key());

      T innerDestValue;
      _StoneDeserializeValue(innerDestValue, *itr);

      destValue[key] = innerDestValue;
    }
  }

  template<typename T>
  Json::Value _StoneSerializeValue(const std::map<std::string,T>& value)
  {
    Json::Value result(Json::objectValue);

    for (std::map<std::string, T>::const_iterator it = value.cbegin();
      it != value.cend(); ++it)
    {
      // it->first it->second
      result[it->first] = _StoneSerializeValue(it->second);
    }
    return result;
  }

  /** Throws in case of problem */
  template<typename T>
  void _StoneDeserializeValue(
    std::vector<T>& destValue, const Json::Value& jsonValue)
  {
    destValue.clear();
    destValue.reserve(jsonValue.size());
    for (Json::Value::ArrayIndex i = 0; i != jsonValue.size(); i++)
    {
      T innerDestValue;
      _StoneDeserializeValue(innerDestValue, jsonValue[i]);
      destValue.push_back(innerDestValue);
    }
  }

  template<typename T>
  Json::Value _StoneSerializeValue(const std::vector<T>& value)
  {
    Json::Value result(Json::arrayValue);
    for (size_t i = 0; i < value.size(); ++i)
    {
      result.append(_StoneSerializeValue(value[i]));
    }
    return result;
  }

  void StoneCheckSerializedValueTypeGeneric(const Json::Value& value)
  {
    if ((!value.isMember("type")) || (!value["type"].isString()))
    {
      std::stringstream ss;
      ss << "Cannot deserialize value ('type' key invalid)";
      throw std::runtime_error(ss.str());
    }
  }

  void StoneCheckSerializedValueType(
    const Json::Value& value, std::string typeStr)
  {
    StoneCheckSerializedValueTypeGeneric(value);

    std::string actTypeStr = value["type"].asString();
    if (actTypeStr != typeStr)
    {
      std::stringstream ss;
      ss << "Cannot deserialize type" << actTypeStr
        << "into " << typeStr;
      throw std::runtime_error(ss.str());
    }
  }

  // end of generic methods

// end of generic methods
{% for enum in enums%}
  enum {{enum['name']}} {
{% for key in enum['fields']%}    {{key}},
{%endfor%}  };

  void _StoneDeserializeValue(
    {{enum['name']}}& destValue, const Json::Value& jsonValue)
  {
    destValue = static_cast<{{enum['name']}}>(jsonValue.asInt64());
  }

  Json::Value _StoneSerializeValue(const {{enum['name']}}& value)
  {
    return Json::Value(static_cast<int64_t>(value));
  }
{%endfor%}
{% for struct in structs%}
#ifdef _MSC_VER
#pragma region {{struct['name']}}
#endif //_MSC_VER

  struct {{struct['name']}}
  {
{% for key in struct['fields']%}    {{CanonToCpp(struct['fields'][key])}} {{key}};
{% endfor %}
    {{struct['name']}}()
    {
{% for key in struct['fields']%}      {{key}} = {{CanonToCpp(struct['fields'][key])}}();
{% endfor %}
    }
  };

  void _StoneDeserializeValue({{struct['name']}}& destValue, const Json::Value& value)
  {
{% for key in struct['fields']%}    _StoneDeserializeValue(destValue.{{key}}, value["{{key}}"]);
{% endfor %}
  }

  Json::Value _StoneSerializeValue(const {{struct['name']}}& value)
  {
    Json::Value result(Json::objectValue);
{% for key in struct['fields']%}    result["{{key}}"] = _StoneSerializeValue(value.{{key}});
{% endfor %}
    return result;
  }

  void StoneDeserialize({{struct['name']}}& destValue, const Json::Value& value)
  {
    StoneCheckSerializedValueType(value, "{{rootName}}.{{struct['name']}}");
    _StoneDeserializeValue(destValue, value["value"]);
  }

  Json::Value StoneSerialize(const {{struct['name']}}& value)
  {
    Json::Value result(Json::objectValue);
    result["type"] = "{{rootName}}.{{struct['name']}}";
    result["value"] = _StoneSerializeValue(value);
    return result;
  }

#ifdef _MSC_VER
#pragma endregion {{struct['name']}}
#endif //_MSC_VER
{% endfor %}
#ifdef _MSC_VER
#pragma region Dispatching code
#endif //_MSC_VER

  class IDispatcher
  {
  public:
{% for struct in structs%}    virtual bool Handle(const {{struct['name']}}& value) = 0;
{% endfor %}  };

  /** Service function for StoneDispatchToHandler */
  bool StoneDispatchJsonToHandler(
    const Json::Value& jsonValue, IDispatcher* dispatcher)
  {
    StoneCheckSerializedValueTypeGeneric(jsonValue);
    std::string type = jsonValue["type"].asString();
    if (type == "")
    {
      // this should never ever happen
      throw std::runtime_error("Caught empty type while dispatching");
    }
{% for struct in structs%}    else if (type == "{{rootName}}.{{struct['name']}}")
    {
      {{struct['name']}} value;
      _StoneDeserializeValue(value, jsonValue["value"]);
      return dispatcher->Handle(value);
    }
{% endfor %}    else
    {
      return false;
    }
  }

  /** Takes a serialized type and passes this to the dispatcher */
  bool StoneDispatchToHandler(std::string strValue, IDispatcher* dispatcher)
  {
    Json::Value readValue;

    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    StoneSmartPtr<Json::CharReader> ptr(reader);

    std::string errors;

    bool ok = reader->parse(
      strValue.c_str(),
      strValue.c_str() + strValue.size(),
      &readValue,
      &errors
    );
    if (!ok)
    {
      std::stringstream ss;
      ss << "Jsoncpp parsing error: " << errors;
      throw std::runtime_error(ss.str());
    }
    return StoneDispatchJsonToHandler(readValue, dispatcher);
  }

#ifdef _MSC_VER
#pragma endregion Dispatching code
#endif //_MSC_VER
}
