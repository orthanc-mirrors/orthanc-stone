/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890

Generated on {{currentDatetime}} by stonegentool

*/
#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <memory>
#include <optional>
#include <json/json.h>

//#define STONEGEN_NO_CPP11 1

#ifdef STONEGEN_NO_CPP11
#define StoneSmartPtr std::auto_ptr
#else 
#define StoneSmartPtr std::unique_ptr
#endif 

namespace {{rootName}}
{
  /** Throws in case of problem */
  inline void _StoneDeserializeValue(int32_t& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asInt();
  }

  inline Json::Value _StoneSerializeValue(int32_t value)
  {
    Json::Value result(value);
    return result;
  }

  inline void _StoneDeserializeValue(Json::Value& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue;
  }

  inline Json::Value _StoneSerializeValue(Json::Value value)
  {
    return value;
  }

  /** Throws in case of problem */
  inline void _StoneDeserializeValue(double& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asDouble();
  }

  inline Json::Value _StoneSerializeValue(double value)
  {
    Json::Value result(value);
    return result;
  }

  /** Throws in case of problem */
  inline void _StoneDeserializeValue(bool& destValue, const Json::Value& jsonValue)
  {
    destValue = jsonValue.asBool();
  }

  inline Json::Value _StoneSerializeValue(bool value)
  {
    Json::Value result(value);
    return result;
  }

  /** Throws in case of problem */
  inline void _StoneDeserializeValue(
       std::string& destValue
     , const Json::Value& jsonValue)
  {
    destValue = jsonValue.asString();
  }

  inline Json::Value _StoneSerializeValue(const std::string& value)
  {
    // the following is better than 
    Json::Value result(value.data(),value.data()+value.size());
    return result;
  }

  inline std::string MakeIndent(int indent)
  {
    char* txt = reinterpret_cast<char*>(malloc(indent+1)); // NO EXCEPTION BELOW!!!!!!!!!!!!
    for(size_t i = 0; i < indent; ++i)
      txt[i] = ' ';
    txt[indent] = 0;
    std::string retVal(txt);
    free(txt); // NO EXCEPTION ABOVE !!!!!!!!!!
    return retVal;
  }

  // generic dumper
  template<typename T>
  std::ostream& StoneDumpValue(std::ostream& out, const T& value, int indent)
  {
    out << MakeIndent(indent) << value;
    return out;
  }

  // string dumper
  inline std::ostream& StoneDumpValue(std::ostream& out, const std::string& value, int indent)
  {
    out << MakeIndent(indent) << "\"" << value  << "\"";
    return out;
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

    for (typename std::map<std::string, T>::const_iterator it = value.cbegin();
      it != value.cend(); ++it)
    {
      // it->first it->second
      result[it->first] = _StoneSerializeValue(it->second);
    }
    return result;
  }

  template<typename T>
  std::ostream& StoneDumpValue(std::ostream& out, const std::map<std::string,T>& value, int indent)
  {
    out << MakeIndent(indent) << "{\n";
    for (typename std::map<std::string, T>::const_iterator it = value.cbegin();
      it != value.cend(); ++it)
    {
      out << MakeIndent(indent+2) << "\"" << it->first << "\" : ";
      StoneDumpValue(out, it->second, indent+2);
    }
    out << MakeIndent(indent) << "}\n";
    return out;
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

  template<typename T>
  std::ostream& StoneDumpValue(std::ostream& out, const std::vector<T>& value, int indent)
  {
    out << MakeIndent(indent) << "[\n";
    for (size_t i = 0; i < value.size(); ++i)
    {
      StoneDumpValue(out, value[i], indent+2);
    }
    out << MakeIndent(indent) << "]\n";
    return out;
  }

  inline void StoneCheckSerializedValueTypeGeneric(const Json::Value& value)
  {
    if ((!value.isMember("type")) || (!value["type"].isString()))
    {
      std::stringstream ss;
      ss << "Cannot deserialize value ('type' key invalid)";
      throw std::runtime_error(ss.str());
    }
  }

  inline void StoneCheckSerializedValueType(
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
{% for key in enum['fields']%}    {{enum['name']}}_{{key}},
{%endfor%}  };

  inline void _StoneDeserializeValue(
    {{enum['name']}}& destValue, const Json::Value& jsonValue)
  {
    destValue = static_cast<{{enum['name']}}>(jsonValue.asInt64());
  }

  inline Json::Value _StoneSerializeValue(const {{enum['name']}}& value)
  {
    return Json::Value(static_cast<int64_t>(value));
  }

  inline std::string ToString(const {{enum['name']}}& value)
  {
{% for key in enum['fields']%}    if( value == {{enum['name']}}_{{key}})
    {
      return std::string("{{key}}");
    }
{%endfor%}    std::stringstream ss;
    ss << "Value \"" << value << "\" cannot be converted to {{enum['name']}}. Possible values are: "
{% for key in enum['fields']%}        << " {{key}} = " << static_cast<int64_t>({{enum['name']}}_{{key}})  << ", " 
{% endfor %}        << std::endl;
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }

  inline void FromString({{enum['name']}}& value, std::string strValue)
  {
{% for key in enum['fields']%}    if( strValue == std::string("{{key}}") )
    {
      value = {{enum['name']}}_{{key}};
    }
{%endfor%}
    std::stringstream ss;
    ss << "String \"" << strValue << "\" cannot be converted to {{enum['name']}}. Possible values are: {% for key in enum['fields']%}{{key}}{% endfor %}";
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const {{enum['name']}}& value, int indent = 0)
  {
{% for key in enum['fields']%}    if( value == {{enum['name']}}_{{key}})
    {
      out << MakeIndent(indent) << "{{key}}" << std::endl;
    }
{%endfor%}    return out;
  }

{%endfor%}
{% for struct in structs%}
#ifdef _MSC_VER
#pragma region {{struct['name']}}
#endif //_MSC_VER

  struct {{struct['name']}}
  {
{% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}    {{CanonToCpp(struct['fields'][key])}} {{key}};
{% endfor %}{% endif %}{% endif %}
    {{struct['name']}}({% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}{{CanonToCpp(struct['fields'][key])}} {{key}} = {{CanonToCpp(struct['fields'][key])}}(){{ ", " if not loop.last }}{% endfor %}{% endif %}{% endif %})
    {
{% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}      this->{{key}} = {{key}};
{% endfor %}{% endif %}{% endif %}    }
  };

  inline void _StoneDeserializeValue({{struct['name']}}& destValue, const Json::Value& value)
  {
{% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}    _StoneDeserializeValue(destValue.{{key}}, value["{{key}}"]);
{% endfor %}{% endif %}{% endif %}    }

  inline Json::Value _StoneSerializeValue(const {{struct['name']}}& value)
  {
    Json::Value result(Json::objectValue);
{% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}    result["{{key}}"] = _StoneSerializeValue(value.{{key}});
{% endfor %}{% endif %}{% endif %}
    return result;
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const {{struct['name']}}& value, int indent = 0)
  {
    out << MakeIndent(indent) << "{\n";
{% if struct %}{% if struct['fields'] %}{% for key in struct['fields']%}    out << MakeIndent(indent) << "{{key}}:\n";
    StoneDumpValue(out, value.{{key}},indent+2);
    out << "\n";
{% endfor %}{% endif %}{% endif %}
    out << MakeIndent(indent) << "}\n";
    return out;
  }

  inline void StoneDeserialize({{struct['name']}}& destValue, const Json::Value& value)
  {
    StoneCheckSerializedValueType(value, "{{rootName}}.{{struct['name']}}");
    _StoneDeserializeValue(destValue, value["value"]);
  }

  inline Json::Value StoneSerializeToJson(const {{struct['name']}}& value)
  {
    Json::Value result(Json::objectValue);
    result["type"] = "{{rootName}}.{{struct['name']}}";
    result["value"] = _StoneSerializeValue(value);
    return result;
  }

  inline std::string StoneSerialize(const {{struct['name']}}& value)
  {
    Json::Value resultJson = StoneSerializeToJson(value);
    std::string resultStr = resultJson.toStyledString();
    return resultStr;
  }

#ifdef _MSC_VER
#pragma endregion {{struct['name']}}
#endif //_MSC_VER
{% endfor %}
#ifdef _MSC_VER
#pragma region Dispatching code
#endif //_MSC_VER

  class IHandler
  {
  public:
{% for struct in structs%}    virtual bool Handle(const {{struct['name']}}& value) = 0;
{% endfor %}  };

  /** Service function for StoneDispatchToHandler */
  inline bool StoneDispatchJsonToHandler(
    const Json::Value& jsonValue, IHandler* handler)
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
      return handler->Handle(value);
    }
{% endfor %}    else
    {
      return false;
    }
  }

  /** Takes a serialized type and passes this to the handler */
  inline bool StoneDispatchToHandler(std::string strValue, IHandler* handler)
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
    return StoneDispatchJsonToHandler(readValue, handler);
  }

#ifdef _MSC_VER
#pragma endregion Dispatching code
#endif //_MSC_VER
}
