/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/

namespace VsolStuff
{
  enum EnumMonth0
  {
    January,
    February,
    March
  };

  // interface Serializer
  // {
  //   Serialize(value: number): string;
  //   Serialize(value: string): string;
  //   Serialize(value: EnumMonth0): string;
  // };
  function printf(value: any):void
  {
    console.log(value)
  }

  // function StoneSerialize(value: string) : string;
  // function StoneSerialize(value: number) : string;
  // function StoneSerialize(value: EnumMonth0) : string;
  function StoneSerialize<T>(value: T[]) : string;

  function StoneSerialize<T>(value: T[] | EnumMonth0) : string
  {
    let valueType = typeof value;
    printf(`About to serialize value. Type is ${valueType}`)
    printf(`About to serialize value. Type is ${typeof value}`)
    return "Choucroute";
  }

  function main():number
  {
    enum Color {Red = 1, Green = 2, Blue = 4}
    let color: Color = Color.Green;
    printf("---------------------------");
    printf(`typeof color: ${typeof color}`);
    printf("---------------------------");
    let colors: Color[] = []
    colors.push(Color.Green);
    colors.push(Color.Red);
    printf(`typeof colors: ${typeof colors}`);
    printf(`Array.isArray(colors): ${Array.isArray(colors)}`);
    printf("---------------------------");
    

    let toto:EnumMonth0[] = [];

    toto.push(EnumMonth0.February);
    toto.push(EnumMonth0.March);

    printf(JSON.stringify(toto));

    return 0;

  }

  main()

//   string StoneSerialize_number(int32_t value)
//   {

//     Json::Value result(value);
//     return result;
//   }

//   Json::Value StoneSerialize(double value)
//   {
//     Json::Value result(value);
//     return result;
//   }

//   Json::Value StoneSerialize(bool value)
//   {
//     Json::Value result(value);
//     return result;
//   }

//   Json::Value StoneSerialize(const std::string& value)
//   {
//     // the following is better than 
//     Json::Value result(value.data(),value.data()+value.size());
//     return result;
//   }

//   template<typename T>
//   Json::Value StoneSerialize(const std::map<std::string,T>& value)
//   {
//     Json::Value result(Json::objectValue);

//     for (std::map<std::string, T>::const_iterator it = value.cbegin();
//       it != value.cend(); ++it)
//     {
//       // it->first it->second
//       result[it->first] = StoneSerialize(it->second);
//     }
//     return result;
//   }

//   template<typename T>
//   Json::Value StoneSerialize(const std::vector<T>& value)
//   {
//     Json::Value result(Json::arrayValue);
//     for (size_t i = 0; i < value.size(); ++i)
//     {
//       result.append(StoneSerialize(value[i]));
//     }
//     return result;
//   }

//   enum EnumMonth0
//   {
//     January,
//     February,
//     March
//   };

//   std::string ToString(EnumMonth0 value)
//   {
//     switch(value)
//     {
//       case January: 
//         return "January";
//       case February:
//         return "February";
//       case March:
//         return "March";
//       default:
//         {
//           std::stringstream ss;
//           ss << "Unrecognized EnumMonth0 value (" << static_cast<int64_t>(value) << ")";
//           throw std::runtime_error(ss.str());
//         }
//     }
//   }

//   void FromString(EnumMonth0& value, std::string strValue)
//   {
//     if (strValue == "January" || strValue == "EnumMonth0_January")
//     {
//       return January;
//     }
//     else if (strValue == "February" || strValue == "EnumMonth0_February")
//     {
//       return February;
//     }
// #error Not implemented yet
//   }

//   Json::Value StoneSerialize(const EnumMonth0& value)
//   {
//     return StoneSerialize(ToString(value));
//   }
//   struct Message1
//   {
//     int32_t a;
//     std::string b;
//     EnumMonth0 c;
//     bool d;
//   };

//   struct Message2
//   {
//     std::string toto;
//     std::vector<Message1> tata;
//     std::vector<std::string> tutu;
//     std::map<std::string, std::string> titi;
//     std::map<std::string, Message1> lulu;
//   };

//   Json::Value StoneSerialize(const Message1& value)
//   {
//     Json::Value result(Json::objectValue);
//     result["a"] = StoneSerialize(value.a);
//     result["b"] = StoneSerialize(value.b);
//     result["c"] = StoneSerialize(value.c);
//     result["d"] = StoneSerialize(value.d);
//     return result;
//   }
    
//   Json::Value StoneSerialize(const Message2& value)
//   {
//     Json::Value result(Json::objectValue);
//     result["toto"] = StoneSerialize(value.toto);
//     result["tata"] = StoneSerialize(value.tata);
//     result["tutu"] = StoneSerialize(value.tutu);
//     result["titi"] = StoneSerialize(value.titi);
//     result["lulu"] = StoneSerialize(value.lulu);
//     return result;
//   }
// }

// int main()
// {
//   VsolStuff::Message1 msg1_0;
//   msg1_0.a = 42;
//   msg1_0.b = "Benjamin";
//   msg1_0.c = VsolStuff::January;
//   msg1_0.d = true;

//   VsolStuff::Message1 msg1_1;
//   msg1_1.a = 43;
//   msg1_1.b = "Sandrine";
//   msg1_1.c = VsolStuff::March;
//   msg1_0.d = false;

//   // std::string toto;
//   // std::vector<Message1> tata;
//   // std::vector<std::string> tutu;
//   // std::map<int32_t, std::string> titi;
//   // std::map<int32_t, Message1> lulu;

//   VsolStuff::Message2 msg2_0;
//   msg2_0.toto = "Prout zizi";
//   msg2_0.tata.push_back(msg1_0);
//   msg2_0.tata.push_back(msg1_1);
//   msg2_0.tutu.push_back("Mercadet");
//   msg2_0.tutu.push_back("Poisson");
//   msg2_0.titi["44"] = "key 44";
//   msg2_0.titi["45"] = "key 45";
//   msg2_0.lulu["54"] = msg1_1;
//   msg2_0.lulu["55"] = msg1_0;
//   auto result = VsolStuff::StoneSerialize(msg2_0);
//   auto resultStr = result.toStyledString();

//   Json::Value readValue;

//   Json::CharReaderBuilder builder;
//   Json::CharReader* reader = builder.newCharReader();
//   std::string errors;

//   bool ok = reader->parse(
//     resultStr.c_str(),
//     resultStr.c_str() + resultStr.size(),
//     &readValue,
//     &errors
//   );
//   delete reader;

//   if (!ok)
//   {
//     std::stringstream ss;
//     ss << "Json parsing error: " << errors;
//     throw std::runtime_error(ss.str());
//   }
//   std::cout << readValue.get("toto", "Default Value").asString() << std::endl;
//   return 0;
// }


}

