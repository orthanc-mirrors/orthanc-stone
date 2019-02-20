import json
import yaml
import re
import sys
from jinja2 import Template
from typing import (
    Any,
    Dict,
    Generator,
    Iterable,
    Iterator,
    List,
    Match,
    Optional,
    Tuple,
    Union,
    cast,
)
from io import StringIO
import time

"""
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
"""

# see https://stackoverflow.com/a/2504457/2927708
def trim(docstring):
    if not docstring:
        return ''
    # Convert tabs to spaces (following the normal Python rules)
    # and split into a list of lines:
    lines = docstring.expandtabs().splitlines()
    # Determine minimum indentation (first line doesn't count):
    indent = sys.maxsize
    for line in lines[1:]:
        stripped = line.lstrip()
        if stripped:
            indent = min(indent, len(line) - len(stripped))
    # Remove indentation (first line is special):
    trimmed = [lines[0].strip()]
    if indent < sys.maxsize:
        for line in lines[1:]:
            trimmed.append(line[indent:].rstrip())
    # Strip off trailing and leading blank lines:
    while trimmed and not trimmed[-1]:
        trimmed.pop()
    while trimmed and not trimmed[0]:
        trimmed.pop(0)
    # Return a single string:
    return '\n'.join(trimmed)


class GenCode:
    def __init__(self):

        # file-wide preamble (#include directives, comment...)
        self.cppPreamble = StringIO()

        self.cppEnums = StringIO()
        self.cppStructs = StringIO()
        self.cppDispatcher = StringIO()
        self.cppHandler = StringIO()

        # file-wide preamble (module directives, comment...)
        self.tsPreamble = StringIO()

        self.tsEnums = StringIO()
        self.tsStructs = StringIO()
        self.tsDispatcher = StringIO()
        self.tsHandler = StringIO()

    def FlattenToFiles(self, outputDir: str):
        raise NotImplementedError()


class JsonHelpers:
    """A set of utilities to perform JSON operations"""

    @staticmethod
    def removeCommentsFromJsonContent(string):
        """
      Remove comments from a JSON file

      Comments are not allowed in JSON but, i.e., Orthanc configuration files
      contains C++ like comments that we need to remove before python can
      parse the file
      """
        # remove all occurrence streamed comments (/*COMMENT */) from string
        string = re.sub(re.compile("/\*.*?\*/", re.DOTALL), "", string)

        # remove all occurrence singleline comments (//COMMENT\n ) from string
        string = re.sub(re.compile("//.*?\n"), "", string)

        return string

    @staticmethod
    def loadJsonWithComments(path):
        """
      Reads a JSON file that may contain C++ like comments
      """
        with open(path, "r") as fp:
            fileContent = fp.read()
        fileContent = JsonHelpers.removeCommentsFromJsonContent(fileContent)
        return json.loads(fileContent)


def LoadSchemaFromJson(filePath: str):
    return JsonHelpers.loadJsonWithComments(filePath)

def GetCppTypenameFromCanonical(canonicalTypename: str) -> str:
    # C++: prefix map vector and string with std::map, std::vector and
    # std::string
    # replace int32 by int32_t
    # replace float32 by float
    # replace float64 by double
    retVal: str = canonicalTypename
    retVal = retVal.replace("map", "std::map")
    retVal = retVal.replace("vector", "std::vector")
    retVal = retVal.replace("int32", "int32_t")
    retVal = retVal.replace("float32", "float")
    retVal = retVal.replace("float64", "double")
    return retVal

def GetTypeScriptTypenameFromCanonical(canonicalTypename: str) -> str:
    # TS: replace vector with Array and map with Map
    # string remains string
    # replace int32 by number
    # replace float32 by number
    # replace float64 by number
    retVal: str = canonicalTypename
    retVal = retVal.replace("map", "Map")
    retVal = retVal.replace("vector", "Array")
    retVal = retVal.replace("int32", "number")
    retVal = retVal.replace("float32", "number")
    retVal = retVal.replace("float64", "number")
    retVal = retVal.replace("bool", "boolean")
    return retVal

def EatToken(sentence: str) -> Tuple[str, str]:
    """splits "A,B,C" into "A" and "B,C" where A, B and C are type names
  (including templates) like "int32", "TotoTutu", or 
  "map<map<int32,vector<string>>,map<string,int32>>" """

    if sentence.count("<") != sentence.count(">"):
        raise Exception(
            f"Error in the partial template type list {sentence}."
            + " The number of < and > do not match!"
        )

    # the template level we're currently in
    templateLevel = 0
    for i in range(len(sentence)):
        if (sentence[i] == ",") and (templateLevel == 0):
            return (sentence[0:i], sentence[i + 1 :])
        elif sentence[i] == "<":
            templateLevel += 1
        elif sentence[i] == ">":
            templateLevel -= 1
    return (sentence, "")


def SplitListOfTypes(typename: str) -> List[str]:
    """Splits something like
  vector<string>,int32,map<string,map<string,int32>> 
  in:
  - vector<string>
  - int32
    map<string,map<string,int32>>
  
  This is not possible with a regex so 
  """
    stillStuffToEat: bool = True
    tokenList = []
    restOfString = typename
    while stillStuffToEat:
        firstToken, restOfString = EatToken(restOfString)
        tokenList.append(firstToken)
        if restOfString == "":
            stillStuffToEat = False
    return tokenList


templateRegex = \
  re.compile(r"([a-zA-Z0-9_]*[a-zA-Z0-9_]*)<([a-zA-Z0-9_,:<>]+)>")


def ParseTemplateType(typename) -> Tuple[bool, str, List[str]]:
    """ If the type is a template like "SOMETHING<SOME<THING,EL<SE>>>", 
    then it returns (true,"SOMETHING","SOME<THING,EL<SE>>")
  otherwise it returns (false,"","")"""

    # let's remove all whitespace from the type
    # split without argument uses any whitespace string as separator
    # (space, tab, newline, return or formfeed)
    typename = "".join(typename.split())
    matches = templateRegex.match(typename)
    if matches == None:
        return (False, "", [])
    else:
        m = cast(Match[str], matches)
        assert len(m.groups()) == 2
        # we need to split with the commas that are outside of the
        # defined types. Simply splitting at commas won't work
        listOfDependentTypes = SplitListOfTypes(m.group(2))
        return (True, m.group(1), listOfDependentTypes)


def ComputeOrderFromTypeTree(
  ancestors: List[str], 
  genOrder: List[str], 
  shortTypename: str, schema: Dict[str, Dict]) -> None:

  if shortTypename in ancestors:
    raise Exception(
      f"Cyclic dependency chain found: the last of {ancestors} "
      + f"depends on {shortTypename} that is already in the list."
    )

  if not (shortTypename in genOrder):
    (isTemplate, _, dependentTypenames) = ParseTemplateType(shortTypename)
    if isTemplate:
      # if it is a template, it HAS dependent types... They can be 
      # anything (primitive, collection, enum, structs..). 
      # Let's process them!
      for dependentTypename in dependentTypenames:
        # childAncestors = ancestors.copy()  NO TEMPLATE ANCESTOR!!!
        # childAncestors.append(typename)
        ComputeOrderFromTypeTree(
            ancestors, genOrder, dependentTypename, schema
        )
    else:
      # If it is not template, we are only interested if it is a 
      # dependency that we must take into account in the dep graph,
      # i.e., a struct.
      if IsShortStructType(shortTypename, schema):
        struct:Dict = schema[GetLongTypename(shortTypename, schema)]
        # The keys in the struct dict are the member names
        # The values in the struct dict are the member types
        for field in struct.keys():
          # we fill the chain of dependent types (starting here)
          ancestors.append(shortTypename)
          ComputeOrderFromTypeTree(
            ancestors, genOrder, struct[field], schema)
          # don't forget to restore it!
          ancestors.pop()
        
        # now we're pretty sure our dependencies have been processed,
        # we can start marking our code for generation (it might 
        # already have been done if someone referenced us earlier)
        if not shortTypename in genOrder:
          genOrder.append(shortTypename)

# +-----------------------+
# |   Utility functions   |
# +-----------------------+

def IsShortStructType(typename: str, schema: Dict[str, Dict]) -> bool:
  fullStructName = "struct " + typename
  return (fullStructName in schema)

def GetLongTypename(shortTypename: str, schema: Dict):
  if shortTypename.startswith("enum "):
    raise RuntimeError('shortTypename.startswith("enum "):')
  enumName: str = "enum " + shortTypename
  isEnum = enumName in schema

  if shortTypename.startswith("struct "):
    raise RuntimeError('shortTypename.startswith("struct "):')
  structName: str = "struct " + shortTypename
  isStruct = ("struct " + shortTypename) in schema

  if isEnum and isStruct:
    raise RuntimeError('Enums and structs cannot have the same name')

  if isEnum:
    return enumName
  if isStruct:
    return structName

def IsTypename(fullName: str) -> bool:
  return (fullName.startswith("enum ") or fullName.startswith("struct "))

def IsEnumType(fullName: str) -> bool:
  return fullName.startswith("enum ")

def IsStructType(fullName: str) -> bool:
  return fullName.startswith("struct ")

def GetShortTypename(fullTypename: str) -> str:
  if fullTypename.startswith("struct "):
    return fullTypename[7:] 
  elif fullTypename.startswith("enum"):
    return fullTypename[5:] 
  else:
    raise RuntimeError \
      ('fullTypename should start with either "struct " or "enum "')

def CheckSchemaSchema(schema: Dict) -> None:
  if not "rootName" in schema:
      raise Exception("schema lacks the 'rootName' key")
  for name in schema.keys():
    if (not IsEnumType(name)) and (not IsStructType(name)) and \
      (name != 'rootName'):
      raise RuntimeError \
        (f'Type "{name}" should start with "enum " or "struct "')

  # TODO: check enum fields are unique (in whole namespace)
  # TODO: check struct fields are unique (in each struct)

# +-----------------------+
# | Main processing logic |
# +-----------------------+

def ComputeRequiredDeclarationOrder(schema: dict) -> List[str]:
  # sanity check
  CheckSchemaSchema(schema)

  # we traverse the type dependency graph and we fill a queue with
  # the required struct types, in a bottom-up fashion, to compute
  # the declaration order
  # The genOrder list contains the struct full names in the order
  # where they must be defined.
  # We do not care about the enums here... They do not depend upon
  # anything and we'll handle them, in their original declaration 
  # order, at the start
  genOrder: List = []
  for fullName in schema.keys():
    if IsStructType(fullName):
      realName: str = GetShortTypename(fullName)
      ancestors: List[str] = []
      ComputeOrderFromTypeTree(ancestors, genOrder, realName, schema)
  return genOrder

def ProcessSchema(schema: dict, genOrder: List[str]) -> Dict:
  # sanity check
  CheckSchemaSchema(schema)

  # let's doctor the schema to clean it up a bit
  # order DOES NOT matter for enums, even though it's a list
  enums: List[Dict] = []
  for fullName in schema.keys():
    if IsEnumType(fullName):
      # convert "enum Toto" to "Toto"
      typename:str = GetShortTypename(fullName)
      enum = {}
      enum['name'] = typename
      assert(type(schema[fullName]) == list)
      enum['fields'] = schema[fullName] # must be a list
      enums.append(enum)

  # now that the order has been established, we actually store\
  # the structs in the correct order
  # the structs are like:
  # example = [
  #   {
  #     "name": "Message1",
  #     "fields": {
  #       "someMember":"int32",
  #       "someOtherMember":"vector<string>"
  #     }
  #   },
  #   { 
  #     "name": "Message2",
  #     "fields": {
  #       "someMember":"int32",
  #       "someOtherMember22":"vector<Message1>"
  #     }
  #   }
  # ]

  structs: List[Dict] = []
  for i in range(len(genOrder)):
    # this is already the short name
    typename = genOrder[i]
    fieldDict = schema["struct " + typename]
    struct = {}
    struct['name'] = typename
    struct['fields'] = fieldDict
    structs.append(struct)
  
  templatingDict = {}
  templatingDict['enums'] = enums
  templatingDict['structs'] = structs
  templatingDict['rootName'] = schema['rootName']

  return templatingDict

# +-----------------------+
# |    Write to files     |
# +-----------------------+

# def WriteStreamsToFiles(rootName: str, genc: Dict[str, StringIO]) \
#   -> None:
#   pass

def LoadSchema(fn):
  with open(fn, 'rb') as f:
    schema = yaml.load(f)
  return schema

def GetTemplatingDictFromSchemaFilename(fn):
  obj = LoadSchema(fn)
  genOrder: str = ComputeRequiredDeclarationOrder(obj)
  templatingDict = ProcessSchema(obj, genOrder)
  return templatingDict

# +-----------------------+
# |      ENTRY POINT      |
# +-----------------------+

if __name__ == "__main__":
  import argparse

  parser = argparse.ArgumentParser(
    usage="""stonegentool.py [-h] [-o OUT_DIR] [-v] input_schema
    EXAMPLE: python stonegentool.py -o "generated_files/" """
      + """ "mainSchema.yaml,App Specific Commands.json" """
  )
  parser.add_argument("input_schema", type=str, \
    help="path to the schema file")
  parser.add_argument(
    "-o",
    "--out_dir",
    type=str,
    default=".",
    help="""path of the directory where the files 
                          will be generated. Default is current
                          working folder""",
  )
  parser.add_argument(
    "-v",
    "--verbosity",
    action="count",
    default=0,
    help="""increase output verbosity (0 == errors 
                          only, 1 == some verbosity, 2 == nerd
                          mode""",
  )

  args = parser.parse_args()
  inputSchemaFilename = args.input_schema
  outDir = args.out_dir

  schema: Dict = LoadSchema(inputSchemaFilename)
  genOrder: List[str] = ComputeRequiredDeclarationOrder(schema)
  processedSchema: Dict = ProcessSchema(schema,genOrder)


# def GenEnumDecl(genc: GenCode, fullName: str, schema: Dict) -> None:
#   """Writes the enumerations in genc"""
#   enumDict:Dict=schema[fullName]
#   # jinja2 template
#   j2cppEnum = Template(trim(
#   """ {{fullName}}
#       {
#           {% for key in enumDict.keys()%}
#           {{key}},
#           {%endfor%}
#       };
#   """))
#   j2cppEnumR = j2cppEnum.render(locals())
#   genc.cppEnums.write(j2cppEnumR)

#   j2tsEnum = Template(trim(
#   """ export {{fullName}}
#       {
#           {% for key in enumDict.keys()%}
#           {{key}},
#           {%endfor%}
#       };
#   """))
#   j2cppEnumR = j2cppEnum.render(locals())
#   genc.tsEnums.write(j2cppEnumR)

  

# def GetSerializationCode(typename: str,valueName: str, tempName: str)
#   if IsPrimitiveType(typename) or IsTemplateCollection(typename):
#     # no need to write code for the primitive types or collections.
#     # It is handled in C++ by the template functions and in TS by 
#     # the JSON.stringify code.
#   elif IsStructType(typename):
#     pass

# def GenStructTypeDeclAndSerialize(genc: GenCode, type, schema) -> None:
#   ######
#   # CPP
#   ######
#   sampleCpp = """  struct Message1
#     {
#       int32_t a;
#       std::string b;
#       EnumMonth0 c;
#       bool d;
#     };

#     Json::Value StoneSerialize(const Message1& value)
#     {
#       Json::Value result(Json::objectValue);
#       result["a"] = StoneSerialize(value.a);
#       result["b"] = StoneSerialize(value.b);
#       result["c"] = StoneSerialize(value.c);
#       result["d"] = StoneSerialize(value.d);
#       return result;
#     }
#     """
    

#   ######
#   # TS
#   ######
#   sampleTs = """
#       {
#         export class Message1 {
#           a: number;
#           b: string;
#           c: EnumMonth0;
#           d: boolean;
#           public StoneSerialize(): string {
#             let container: object = {};
#             container['type'] = 'Message1';
#             container['value'] = this;
#             return JSON.stringify(container);
#           }
#         };
#       }
#     """




#   tsText: StringIO = StringIO()
#   cppText: StringIO = StringIO()

#   tsText.write("class %s\n" % typeDict["name"])
#   tsText.write("{\n")

#   cppText.write("struct %s\n" % typeDict["name"])
#   cppText.write("{\n")

#   """
  
#   GenerateSerializationCode(typename,valueName)

#   primitives:
#   -----------
#   int
#     jsonValue val(objectInt);
#     val.setValue("$name")
#     parent.add(("$name",$name)
#   double
#     ...
#   string
#     ...

#   collections:
#   -----------
#   dict { }

#   serializeValue()
#   """

#   for i in range(len(typeDict["fields"])):
#     field = typeDict["fields"][i]
#     name = field["name"]
#     tsType = GetTypeScriptTypenameFromCanonical(field["type"])
#     tsText.write("    public %s %s;\n" % (tsType, name))
#     cppType = GetCppTypenameFromCanonical(field["type"])
#     cppText.write("    %s %s;\n" % (cppType, name))

#   tsText.write("};\n\n")
#   cppText.write("};\n\n")

#   genc.tsStructs.write(tsText.getvalue())
#   genc.cppStructs.write(cppText.getvalue())


# def GenerateCodeFromTsTemplate(genc)


# +-----------------------+
# |    CODE GENERATION    |
# +-----------------------+

# def GenPreambles(rootName: str, genc: GenCode) -> None:
#   cppPreambleT = Template(trim(
#     """// autogenerated by stonegentool on {{time.ctime()}} 
#     // for module {{rootName}}
#     #include <cstdint>
#     #include <string>
#     #include <vector>
#     #include <map>
#     namespace {{rootName}}
#     {
#       Json::Value StoneSerialize(int32_t value)
#       {
#         Json::Value result(value);
#         return result;
#       }
#       Json::Value StoneSerialize(double value)
#       {
#         Json::Value result(value);
#         return result;
#       }
#       Json::Value StoneSerialize(bool value)
#       {
#         Json::Value result(value);
#         return result;
#       }
#       Json::Value StoneSerialize(const std::string& value)
#       {
#         // the following is better than 
#         Json::Value result(value.data(),value.data()+value.size());
#         return result;
#       }
#       template<typename T>
#       Json::Value StoneSerialize(const std::map<std::string,T>& value)
#       {
#         Json::Value result(Json::objectValue);

#         for (std::map<std::string, T>::const_iterator it = value.cbegin();
#           it != value.cend(); ++it)
#         {
#           // it->first it->second
#           result[it->first] = StoneSerialize(it->second);
#         }
#         return result;
#       }
#       template<typename T>
#       Json::Value StoneSerialize(const std::vector<T>& value)
#       {
#         Json::Value result(Json::arrayValue);
#         for (size_t i = 0; i < value.size(); ++i)
#         {
#           result.append(StoneSerialize(value[i]));
#         }
#         return result;
#       }
#     """
#   cppPreambleR = cppPreambleT.render(locals())
#   genc.cppPreamble.write(cppPreambleR)
  
#   tsPreambleT = Template(trim(
#     """// autogenerated by stonegentool on {{time.ctime()}} 
#     // for module {{rootName}}
    
#     namespace {{rootName}}
#     {
#     """
#   tsPreambleR = tsPreambleT.render(locals())
#   genc.tsPreamble.write(tsPreambleR)

# def ComputeOrder_ProcessStruct( \
#   genOrder: List[str], name:str, schema: Dict[str, str]) -> None:
#   # let's generate the code according to the
#   struct = schema[name]

#   if not IsStructType(name):
#     raise Exception(f'{typename} should start with "struct "')
