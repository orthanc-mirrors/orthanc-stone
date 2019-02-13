import json
import re
import sys
from typing import Dict, List, Set


"""
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
"""

import json
import re

"""A set of utilities to perform JSON operation"""

class JsonHelpers:
    @staticmethod
    def removeCommentsFromJsonContent(string):
        """
        remove comments from a JSON file

        Comments are not allowed in JSON but, i.e., Orthanc configuration files contains C++ like comments that we need to remove before python can parse the file
        """
        string = re.sub(re.compile("/\*.*?\*/", re.DOTALL), "",
                        string)  # remove all occurance streamed comments (/*COMMENT */) from string
        string = re.sub(re.compile("//.*?\n"), "",
                        string)  # remove all occurance singleline comments (//COMMENT\n ) from string
        return string

    @staticmethod
    def loadJsonWithComments(path):
        """
        reads a JSON file that may contain C++ like comments
        """
        with open(path, 'r') as fp:
            fileContent = fp.read()
        fileContent = JsonHelpers.removeCommentsFromJsonContent(fileContent)
        return json.loads(fileContent)


def LoadSchema(filePath : str):
    return JsonHelpers.loadJsonWithComments(filePath)

# class Type:
#   def __init__(self, canonicalTypeName:str, kind:str):
#     allowedTypeKinds = ["primitive","enum","struct","collection"]
#     """dependent type is the list of canonical types this type depends on.
#        For instance, vector<map<string,int32>> depends on map<string,int32>
#        that, in turn, depends on string and int32 that, in turn, depend on
#        nothing"""
#     self.canonicalTypeName = canonicalTypeName
#     assert(kind in allowedTypeKinds)

#   def setDependentTypes(self, dependentTypes:List[Type]) -> None:
#     self.dependentTypes = dependentTypes

#   def getDependentTypes(self) -> List[Type]:
#     return self.dependentTypes

def GetCppTypeNameFromCanonical(canonicalTypeName : str) -> str:
    # C++: prefix map vector and string with std::map, std::vector and 
    # std::string
    # replace int32 by int32_t
    # replace float32 by float
    # replace float64 by double
    retVal : str = canonicalTypeName.replace("map","std::map")
    retVal : str = canonicalTypeName.replace("vector","std::vector")
    retVal : str = canonicalTypeName.replace("int32","int32_t")
    retVal : str = canonicalTypeName.replace("float32","float")
    retVal : str = canonicalTypeName.replace("float64","double")
    return retVal

def GetTypeScriptTypeNameFromCanonical(canonicalTypeName : str) -> str:
    # TS: replace vector with Array and map with Map
    # string remains string
    # replace int32 by number
    # replace float32 by number
    # replace float64 by number
    retVal : str = canonicalTypeName.replace("map","Map")
    retVal : str = canonicalTypeName.replace("vector","Array")
    retVal : str = canonicalTypeName.replace("int32","number")
    retVal : str = canonicalTypeName.replace("float32","number")
    retVal : str = canonicalTypeName.replace("float64","number")
    retVal : str = canonicalTypeName.replace("bool","boolean")
    return retVal

# class Schema:
#   def __init__(self, root_prefix : str, defined_types : List[Type]):
#     self.rootName : str = root_prefix
#     self.definedTypes : str = defined_types

def CheckTypeSchema(definedType : Dict) -> None:
  allowedDefinedTypeKinds = ["enum","struct"]
  if not 'name' in definedType:
    raise Exception("type lacks the 'name' key")
  name = definedType['name']
  if not 'kind' in definedType:
    raise Exception(f"type {name} lacks the 'kind' key")
  kind = definedType['kind']
  if not (kind in allowedDefinedTypeKinds):
    raise Exception(f"type {name} : kind {kind} is not allowed. " + 
      f"It must be one of {allowedDefinedTypeKinds}")
  
  if not 'fields' in definedType:
    raise Exception("type {name} lacks the 'fields' key")

  # generic check on all kinds of types
  fields = definedType['fields']
  for field in fields:
    fieldName = field['name']
    if not 'name' in field:
      raise Exception("field in type {name} lacks the 'name' key")

  # fields in struct must have types
  if kind == 'struct':  
    for field in fields:
      fieldName = field['name']
      if not 'type' in field:
        raise Exception(f"field {fieldName} in type {name} "
        + "lacks the 'type' key")

def CheckSchemaSchema(schema : Dict) -> None:
  if not 'root_name' in schema:
    raise Exception("schema lacks the 'root_name' key")
  if not 'types' in schema:
    raise Exception("schema lacks the 'types' key")
  for definedType in schema['types']:
    CheckTypeSchema(definedType)

# def CreateAndCacheTypeObject(allTypes : Dict[str,Type], typeDict : Dict)  -> None:
#   """This does not set the dependentTypes field"""
#   typeName : str = typeDict['name']
#   if typeName in allTypes:
#     raise Exception(f'Type {typeName} is defined more than once!')
#   else:
#     typeObject = Type(typeName, typeDict['kind'])
#     allTypes[typeName] = typeObject

def EatToken(sentence : str) -> (str,str):
  """splits "A,B,C" into "A" and "B,C" where A, B and C are type names
  (including templates) like "int32", "TotoTutu", or 
  "map<map<int32,vector<string>>,map<string,int32>>" """

  if sentence.count('<') != sentence.count('>'):
    raise Exception(f"Error in the partial template type list {sentence}."
      + " The number of < and > do not match!")

  # the template level we're currently in
  templateLevel = 0
  for i in range(len(sentence)):
    if (sentence[i] == ",") and (templateLevel == 0):
      return (sentence[0:i],sentence[i+1:])
    elif (sentence[i] == "<"):
      templateLevel += 1
    elif (sentence[i] == ">"):
      templateLevel -= 1
  return (sentence,"")

def SplitListOfTypes(typeName : str) -> List[str]:
  """Splits something like
  vector<string>,int32,map<string,map<string,int32>> 
  in:
  - vector<string>
  - int32
    map<string,map<string,int32>>
  
  This is not possible with a regex so 
  """
  stillStuffToEat : bool = True
  tokenList = []
  restOfString = typeName
  while stillStuffToEat:
    firstToken,restOfString = EatToken(restOfString)
    tokenList.append(firstToken)
    if restOfString == "":
      stillStuffToEat = False
  return tokenList

templateRegex = \
  re.compile(r"([a-zA-Z0-9_]*[a-zA-Z0-9_]*)<([a-zA-Z0-9_,:<>]+)>")

def ParseTemplateType(typeName) -> (bool,str,List[str]):
  """ If the type is a template like "SOMETHING<SOME<THING,EL<SE>>>", then 
  it returns (true,"SOMETHING","SOME<THING,EL<SE>>")
  otherwise it returns (false,"","")"""
  
  # let's remove all whitespace from the type
  # split without argument uses any whitespace string as separator
  # (space, tab, newline, return or formfeed)
  typeName = "".join(typeName.split())
  matches = templateRegex.match(typeName)
  if matches == None:
    return (False,"","")
  else:
    # we need to split with the commas that are outside of the defined types
    # simply splitting at commas won't work
    listOfDependentTypes = SplitListOfTypes(matches.group(2))
    return (True,matches.group(1),listOfDependentTypes)

# def GetPrimitiveType(typeName : str) -> Type:
#   if typeName in allTypes:
#     return allTypes[typeName]
#   else:
#     primitiveTypes = ['int32', 'float32', 'float64', 'string']
#     if not (typeName in primitiveTypes):
#       raise Exception(f"Type {typeName} is unknown.")
#     typeObject = Type(typeName,'primitive')
#     # there are no dependent types in a primitive type --> Type object
#     # constrution is finished at this point
#     allTypes[typeName] = typeObject
#     return typeObject

def ProcessTypeTree(
     ancestors : List[str]
   , genOrderQueue : List[str]
   , structTypes : Dict[str,Dict], typeName : str) -> None:
  if typeName in ancestors:
    raise Exception(f"Cyclic dependency chain found: the last of {ancestors} "
      + f"depends on {typeName} that is already in the list.")
  
  if not (typeName in genOrderQueue):
    # if we reach this point, it means the type is NOT a struct or an enum.
    # it is another (non directly user-defined) type that we must parse and 
    # create. Let's do it!
    (isTemplate,_,dependentTypeNames) = ParseTemplateType(typeName)
    if isTemplate:
      for dependentTypeName in dependentTypeNames:
        # childAncestors = ancestors.copy()  NO TEMPLATE ANCESTOR!!!
        # childAncestors.append(typeName)
        ProcessTypeTree(ancestors, genOrderQueue,
          structTypes, dependentTypeName)
    else:
      if typeName in structTypes:
        ProcessStructType_DepthFirstRecursive(genOrderQueue, structTypes,
        structTypes[typeName])

def ProcessStructType_DepthFirstRecursive(
    genOrderQueue : List[str], structTypes : Dict[str,Dict]
  , typeDict : Dict) -> None:
    # let's generate the code according to the 
    typeName : str = typeDict['name']
    if typeDict['kind'] != 'struct':
      raise Exception(f"Unexpected kind '{typeDict['kind']}' for " +
      "type '{typeName}'")
    typeFields : List[Dict] = typeDict['fields']
    for typeField in typeFields:
      ancestors = [typeName]
      ProcessTypeTree(ancestors, genOrderQueue
        , structTypes, typeField['type'])
    # now we're pretty sure our dependencies have been processed,
    # we can start marking our code for generation
    genOrderQueue.add(typeName)

def ProcessEnumerationType(definedType) -> (str,str):
  print(f"About to process enumeration: {definedType['name']}")
  tsText : str = """import blah

  enum PROUT
  {
    value1
    value2
  }"""

  cppText : str = """import blah

  enum PROUT
  {
    value1
    value2
  }"""

  return (tsText,cppText)

def ProcessStructType(typeDict) -> (str,str):
  print(f"About to process enumeration: {definedType['name']}")
  tsText : str = """import blah

  class PROUT
  {
    public value1 : Type1
    public value2 : Type2
  }"""

  cppText : str = """import blah

  class PROUT
  {
    public:
    Type1: value1
    Type2: value2
  }"""

  return (tsText,cppText)

    

def ProcessSchema(schema : dict) -> None:
  CheckSchemaSchema(schema)
  rootName : str = schema['root_name']
  definedTypes : list = schema['types']

  print(f"Processing schema. rootName = f{rootName}")
  # this will be filled with the generation queue. That is, the type
  # names in the order where they must be defined.
  genOrderQueue : Set = set()

  # the struct names are mapped to their JSON dictionary
  structTypes : Dict[str,Dict] = {}

  # the order here is the generation order
  for definedType in definedTypes:
    if definedType['kind'] == 'enum':
      ProcessEnumerationType(definedType)

  for definedType in definedTypes:
    if definedType['kind'] == 'struct':
      structTypes[definedType['name']] = definedType

  # the order here is NOT the generation order: the types
  # will be processed according to their dependency graph
  for definedType in definedTypes:
    if definedType['kind'] == 'struct':
      ProcessStructType_DepthFirstRecursive(genOrderQueue,structTypes,
        definedType)

  for i in range(len(genOrderQueue))
    typeName = genOrderQueue[i]
    typeDict = structTypes[typeName]
    ProcessStructType(typeDict)

if __name__ == '__main__':
  import argparse
  parser = argparse.ArgumentParser(
    usage = """stonegentool.py [-h] [-o OUT_DIR] [-v] input_schemas
       EXAMPLE: python command_gen.py -o "generated_files/" """
       + """ "mainSchema.json,App Specific Commands.json" """)
  parser.add_argument("input_schema", type=str,
                      help = "path to the schema file")
  parser.add_argument("-o", "--out_dir", type=str, default=".", 
                      help = """path of the directory where the files 
                                will be generated. Default is current
                                working folder""")
  parser.add_argument("-v", "--verbosity", action="count", default=0,
                      help = """increase output verbosity (0 == errors 
                                only, 1 == some verbosity, 2 == nerd
                                mode""")

  args = parser.parse_args()
  inputSchemaFilename = args.input_schema
  outDir = args.out_dir

  print("input schema = " + str(inputSchemaFilename))
  print("out dir = " + str(outDir))

  ProcessSchema(LoadSchema(inputSchemaFilename))
  

###################
##     ATTIC     ##
###################

# this works 

if False:
  obj = json.loads("""{
    "firstName": "Alice",
    "lastName": "Hall",
    "age": 35
  }""")
  print(obj)
