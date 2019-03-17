/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890

Generated on 2019-03-15 10:00:48.763392 by stonegentool

*/
#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <memory>
#include <json/json.h>

//#define STONEGEN_NO_CPP11 1

#ifdef STONEGEN_NO_CPP11
#define StoneSmartPtr std::auto_ptr
#else 
#define StoneSmartPtr std::unique_ptr
#endif 

namespace StoneSampleCommands
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

  enum Tool {
    Tool_LineMeasure,
    Tool_CircleMeasure,
    Tool_Crop,
    Tool_Windowing,
    Tool_Zoom,
    Tool_Pan,
    Tool_Move,
    Tool_Rotate,
    Tool_Resize,
    Tool_Mask,
  };

  inline std::string ToString(const Tool& value)
  {
    if( value == Tool_LineMeasure)
    {
      return std::string("LineMeasure");
    }
    if( value == Tool_CircleMeasure)
    {
      return std::string("CircleMeasure");
    }
    if( value == Tool_Crop)
    {
      return std::string("Crop");
    }
    if( value == Tool_Windowing)
    {
      return std::string("Windowing");
    }
    if( value == Tool_Zoom)
    {
      return std::string("Zoom");
    }
    if( value == Tool_Pan)
    {
      return std::string("Pan");
    }
    if( value == Tool_Move)
    {
      return std::string("Move");
    }
    if( value == Tool_Rotate)
    {
      return std::string("Rotate");
    }
    if( value == Tool_Resize)
    {
      return std::string("Resize");
    }
    if( value == Tool_Mask)
    {
      return std::string("Mask");
    }
    std::stringstream ss;
    ss << "Value \"" << value << "\" cannot be converted to Tool. Possible values are: "
        << " LineMeasure = " << static_cast<int64_t>(Tool_LineMeasure)  << ", " 
        << " CircleMeasure = " << static_cast<int64_t>(Tool_CircleMeasure)  << ", " 
        << " Crop = " << static_cast<int64_t>(Tool_Crop)  << ", " 
        << " Windowing = " << static_cast<int64_t>(Tool_Windowing)  << ", " 
        << " Zoom = " << static_cast<int64_t>(Tool_Zoom)  << ", " 
        << " Pan = " << static_cast<int64_t>(Tool_Pan)  << ", " 
        << " Move = " << static_cast<int64_t>(Tool_Move)  << ", " 
        << " Rotate = " << static_cast<int64_t>(Tool_Rotate)  << ", " 
        << " Resize = " << static_cast<int64_t>(Tool_Resize)  << ", " 
        << " Mask = " << static_cast<int64_t>(Tool_Mask)  << ", " 
        << std::endl;
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }

  inline void FromString(Tool& value, std::string strValue)
  {
    if( strValue == std::string("LineMeasure") )
    {
      value = Tool_LineMeasure;
      return;
    }
    if( strValue == std::string("CircleMeasure") )
    {
      value = Tool_CircleMeasure;
      return;
    }
    if( strValue == std::string("Crop") )
    {
      value = Tool_Crop;
      return;
    }
    if( strValue == std::string("Windowing") )
    {
      value = Tool_Windowing;
      return;
    }
    if( strValue == std::string("Zoom") )
    {
      value = Tool_Zoom;
      return;
    }
    if( strValue == std::string("Pan") )
    {
      value = Tool_Pan;
      return;
    }
    if( strValue == std::string("Move") )
    {
      value = Tool_Move;
      return;
    }
    if( strValue == std::string("Rotate") )
    {
      value = Tool_Rotate;
      return;
    }
    if( strValue == std::string("Resize") )
    {
      value = Tool_Resize;
      return;
    }
    if( strValue == std::string("Mask") )
    {
      value = Tool_Mask;
      return;
    }

    std::stringstream ss;
    ss << "String \"" << strValue << "\" cannot be converted to Tool. Possible values are: LineMeasure CircleMeasure Crop Windowing Zoom Pan Move Rotate Resize Mask ";
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }


  inline void _StoneDeserializeValue(
    Tool& destValue, const Json::Value& jsonValue)
  {
    FromString(destValue, jsonValue.asString());
  }

  inline Json::Value _StoneSerializeValue(const Tool& value)
  {
    std::string strValue = ToString(value);
    return Json::Value(strValue);
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const Tool& value, int indent = 0)
  {
    if( value == Tool_LineMeasure)
    {
      out << MakeIndent(indent) << "LineMeasure" << std::endl;
    }
    if( value == Tool_CircleMeasure)
    {
      out << MakeIndent(indent) << "CircleMeasure" << std::endl;
    }
    if( value == Tool_Crop)
    {
      out << MakeIndent(indent) << "Crop" << std::endl;
    }
    if( value == Tool_Windowing)
    {
      out << MakeIndent(indent) << "Windowing" << std::endl;
    }
    if( value == Tool_Zoom)
    {
      out << MakeIndent(indent) << "Zoom" << std::endl;
    }
    if( value == Tool_Pan)
    {
      out << MakeIndent(indent) << "Pan" << std::endl;
    }
    if( value == Tool_Move)
    {
      out << MakeIndent(indent) << "Move" << std::endl;
    }
    if( value == Tool_Rotate)
    {
      out << MakeIndent(indent) << "Rotate" << std::endl;
    }
    if( value == Tool_Resize)
    {
      out << MakeIndent(indent) << "Resize" << std::endl;
    }
    if( value == Tool_Mask)
    {
      out << MakeIndent(indent) << "Mask" << std::endl;
    }
    return out;
  }


  enum ActionType {
    ActionType_UndoCrop,
    ActionType_Rotate,
    ActionType_Invert,
  };

  inline std::string ToString(const ActionType& value)
  {
    if( value == ActionType_UndoCrop)
    {
      return std::string("UndoCrop");
    }
    if( value == ActionType_Rotate)
    {
      return std::string("Rotate");
    }
    if( value == ActionType_Invert)
    {
      return std::string("Invert");
    }
    std::stringstream ss;
    ss << "Value \"" << value << "\" cannot be converted to ActionType. Possible values are: "
        << " UndoCrop = " << static_cast<int64_t>(ActionType_UndoCrop)  << ", " 
        << " Rotate = " << static_cast<int64_t>(ActionType_Rotate)  << ", " 
        << " Invert = " << static_cast<int64_t>(ActionType_Invert)  << ", " 
        << std::endl;
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }

  inline void FromString(ActionType& value, std::string strValue)
  {
    if( strValue == std::string("UndoCrop") )
    {
      value = ActionType_UndoCrop;
      return;
    }
    if( strValue == std::string("Rotate") )
    {
      value = ActionType_Rotate;
      return;
    }
    if( strValue == std::string("Invert") )
    {
      value = ActionType_Invert;
      return;
    }

    std::stringstream ss;
    ss << "String \"" << strValue << "\" cannot be converted to ActionType. Possible values are: UndoCrop Rotate Invert ";
    std::string msg = ss.str();
    throw std::runtime_error(msg);
  }


  inline void _StoneDeserializeValue(
    ActionType& destValue, const Json::Value& jsonValue)
  {
    FromString(destValue, jsonValue.asString());
  }

  inline Json::Value _StoneSerializeValue(const ActionType& value)
  {
    std::string strValue = ToString(value);
    return Json::Value(strValue);
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const ActionType& value, int indent = 0)
  {
    if( value == ActionType_UndoCrop)
    {
      out << MakeIndent(indent) << "UndoCrop" << std::endl;
    }
    if( value == ActionType_Rotate)
    {
      out << MakeIndent(indent) << "Rotate" << std::endl;
    }
    if( value == ActionType_Invert)
    {
      out << MakeIndent(indent) << "Invert" << std::endl;
    }
    return out;
  }



#ifdef _MSC_VER
#pragma region SelectTool
#endif //_MSC_VER

  struct SelectTool
  {
    Tool tool;

    SelectTool(Tool tool = Tool())
    {
      this->tool = tool;
    }
  };

  inline void _StoneDeserializeValue(SelectTool& destValue, const Json::Value& value)
  {
    _StoneDeserializeValue(destValue.tool, value["tool"]);
    }

  inline Json::Value _StoneSerializeValue(const SelectTool& value)
  {
    Json::Value result(Json::objectValue);
    result["tool"] = _StoneSerializeValue(value.tool);

    return result;
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const SelectTool& value, int indent = 0)
  {
    out << MakeIndent(indent) << "{\n";
    out << MakeIndent(indent) << "tool:\n";
    StoneDumpValue(out, value.tool,indent+2);
    out << "\n";

    out << MakeIndent(indent) << "}\n";
    return out;
  }

  inline void StoneDeserialize(SelectTool& destValue, const Json::Value& value)
  {
    StoneCheckSerializedValueType(value, "StoneSampleCommands.SelectTool");
    _StoneDeserializeValue(destValue, value["value"]);
  }

  inline Json::Value StoneSerializeToJson(const SelectTool& value)
  {
    Json::Value result(Json::objectValue);
    result["type"] = "StoneSampleCommands.SelectTool";
    result["value"] = _StoneSerializeValue(value);
    return result;
  }

  inline std::string StoneSerialize(const SelectTool& value)
  {
    Json::Value resultJson = StoneSerializeToJson(value);
    std::string resultStr = resultJson.toStyledString();
    return resultStr;
  }

#ifdef _MSC_VER
#pragma endregion SelectTool
#endif //_MSC_VER

#ifdef _MSC_VER
#pragma region Action
#endif //_MSC_VER

  struct Action
  {
    ActionType type;

    Action(ActionType type = ActionType())
    {
      this->type = type;
    }
  };

  inline void _StoneDeserializeValue(Action& destValue, const Json::Value& value)
  {
    _StoneDeserializeValue(destValue.type, value["type"]);
    }

  inline Json::Value _StoneSerializeValue(const Action& value)
  {
    Json::Value result(Json::objectValue);
    result["type"] = _StoneSerializeValue(value.type);

    return result;
  }

  inline std::ostream& StoneDumpValue(std::ostream& out, const Action& value, int indent = 0)
  {
    out << MakeIndent(indent) << "{\n";
    out << MakeIndent(indent) << "type:\n";
    StoneDumpValue(out, value.type,indent+2);
    out << "\n";

    out << MakeIndent(indent) << "}\n";
    return out;
  }

  inline void StoneDeserialize(Action& destValue, const Json::Value& value)
  {
    StoneCheckSerializedValueType(value, "StoneSampleCommands.Action");
    _StoneDeserializeValue(destValue, value["value"]);
  }

  inline Json::Value StoneSerializeToJson(const Action& value)
  {
    Json::Value result(Json::objectValue);
    result["type"] = "StoneSampleCommands.Action";
    result["value"] = _StoneSerializeValue(value);
    return result;
  }

  inline std::string StoneSerialize(const Action& value)
  {
    Json::Value resultJson = StoneSerializeToJson(value);
    std::string resultStr = resultJson.toStyledString();
    return resultStr;
  }

#ifdef _MSC_VER
#pragma endregion Action
#endif //_MSC_VER

#ifdef _MSC_VER
#pragma region Dispatching code
#endif //_MSC_VER

  class IHandler
  {
  public:
    virtual bool Handle(const SelectTool& value) = 0;
  };

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
    else if (type == "StoneSampleCommands.SelectTool")
    {
      SelectTool value;
      _StoneDeserializeValue(value, jsonValue["value"]);
      return handler->Handle(value);
    }
    else
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