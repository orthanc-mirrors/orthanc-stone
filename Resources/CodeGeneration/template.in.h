/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/

#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <json/json.h>


namespace VsolStuff
{
  Json::Value StoneSerialize(int32_t value)
  {
    Json::Value result(value);
    return result;
  }

  Json::Value StoneSerialize(double value)
  {
    Json::Value result(value);
    return result;
  }

  Json::Value StoneSerialize(bool value)
  {
    Json::Value result(value);
    return result;
  }

  Json::Value StoneSerialize(const std::string& value)
  {
    // the following is better than 
    Json::Value result(value.data(),value.data()+value.size());
    return result;
  }
    
  template<typename T>
  Json::Value StoneSerialize(const std::map<std::string,T>& value)
  {
    Json::Value result(Json::objectValue);

    for (std::map<std::string, T>::const_iterator it = value.cbegin();
      it != value.cend(); ++it)
    {
      // it->first it->second
      result[it->first] = StoneSerialize(it->second);
    }
    return result;
  }

  template<typename T>
  Json::Value StoneSerialize(const std::vector<T>& value)
  {
    Json::Value result(Json::arrayValue);
    for (size_t i = 0; i < value.size(); ++i)
    {
      result.append(StoneSerialize(value[i]));
    }
    return result;
  }

  %enumerationscpp%

  %structscpp%

}
