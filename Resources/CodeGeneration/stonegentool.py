from typing import List,Dict
import sys, json

def LoadSchema(file_path : str):
    with open(file_path, 'r') as fp:
      obj = json.load(fp)
    return obj

class Type:
  def __init__(self, canonicalTypeName:str, kind:str):
    allowedTypeKinds = ["primitive","enum","struct","collection"]
    """dependent type is the list of canonical types this type depends on.
       For instance, vector<map<string,int32>> depends on map<string,int32>
       that, in turn, depends on string and int32 that, in turn, depend on
       nothing"""
    self.canonicalTypeName = canonicalTypeName
    assert(kind in allowedTypeKinds)

  def setDependentTypes(self, dependentTypes:List[Type]) -> None:
    self.dependentTypes = dependentTypes

  def getDependentTypes(self) -> List[Type]:
    return self.dependentTypes

  def getCppTypeName(self) -> str:
    # C++: prefix map vector and string with std::map, std::vector and std::string
    # replace int32 by int32_t
    # replace float32 by float
    # replace float64 by double
    retVal : str = self.canonicalTypeName.replace("map","std::map")
    retVal : str = self.canonicalTypeName.replace("vector","std::vector")
    retVal : str = self.canonicalTypeName.replace("int32","int32_t")
    retVal : str = self.canonicalTypeName.replace("float32","float")
    retVal : str = self.canonicalTypeName.replace("float64","double")
    return retVal

  def getTypeScriptTypeName(self) -> str:
    # TS: replace vector with Array and map with Map
    # string remains string
    # replace int32 by number
    # replace float32 by number
    # replace float64 by number
    retVal : str = self.canonicalTypeName.replace("map","Map")
    retVal : str = self.canonicalTypeName.replace("vector","Array")
    retVal : str = self.canonicalTypeName.replace("int32","number")
    retVal : str = self.canonicalTypeName.replace("float32","number")
    retVal : str = self.canonicalTypeName.replace("float64","number")
    retVal : str = self.canonicalTypeName.replace("bool","boolean")
    return retVal

class Schema:
  def __init__(self, root_prefix : str, defined_types : List[Type]):
    self.rootName : str = root_prefix
    self.definedTypes : str = defined_types

def CheckTypeSchema(definedType : Dict) -> None:
  allowedDefinedTypeKinds = ["enum","struct"]
  if not definedType.has_key('name'):
    raise Exception("type lacks the 'name' key")
  name = definedType['name']
  if not definedType.has_key('kind'):
    raise Exception(f"type {name} lacks the 'kind' key")
  kind = definedType['kind']
  if not (kind in allowedDefinedTypeKinds):
    raise Exception(f"type {name} : kind {kind} is not allowed. It must be one of {allowedDefinedTypeKinds}")
  
  if not definedType.has_key('fields'):
    raise Exception("type {name} lacks the 'fields' key")

  # generic check on all kinds of types
  fields = definedType['fields']
  for field in fields:
    fieldName = field['name']
    if not field.has_key('name'):
      raise Exception("field in type {name} lacks the 'name' key")

  # fields in struct must have types
  if kind == 'struct':  
    for field in fields:
      fieldName = field['name']
      if not field.has_key('type'):
        raise Exception(f"field {fieldName} in type {name} lacks the 'type' key")

def CheckSchemaSchema(schema : Dict) -> None:
  if not schema.has_key('root_name'):
    raise Exception("schema lacks the 'root_name' key")
  if not schema.has_key('types'):
    raise Exception("schema lacks the 'types' key")
  for definedType in schema['types']:
    CheckTypeSchema(definedType)

def CreateAndCacheTypeObject(allTypes : Dict[str,Type], typeDict : Dict) -> None:
  """This does not set the dependentTypes field"""
  typeName : str = typeDict['name']
  if allTypes.has_key(typeName):
    raise Exception(f'Type {name} is defined more than once!')
  else:
    typeObject = Type(typeName, typeDict['kind'])
    allTypes[typeName] = typeObject


def ParseTemplateType(typeName) -> (bool,str,str):
  """ If the type is a template like "SOMETHING<SOME<THING,EL<SE>>>", then 
  it returns (true,"SOMETHING","SOME<THING,EL<SE>>")
  otherwise it returns (false,"","")"""
  



def GetCachedTypeObject(allTypes : Dict[str,Type], typeName : str) -> Type:
  if allTypes.has_key('typeName')
    return allTypes['typeName']
  # if we reach this point, it means the type is NOT a struct or an enum.
  # it is another (non directly user-defined) type that we must parse and create
  # let's do it


def ComputeTopTypeTree(allTypes : Dict[str,Type], typeDict : Dict) -> None:
  """Now that all the types we might be referring to are known (either structs
  and enums already created OR primitive types OR collections), we can create
  the whole dependency tree"""
  
  if typeDict['kind'] == 'struct':
    typeName : str = typeDict['name']
    typeObject : Type = allTypes[typeName]
    typeFields : List[Dict] = typeDict['fields']
    dependentTypes = []
    for typeField : Dict in typeFields:
      typeFieldObject = CreateTypeObject(allTypes, typeField['name'])
      dependentTypes.append(typeFieldObject)
  elif typeDict['kind'] == 'enum':
    # the enum objects are already created and have no dependencies
  else:
    raise Exception("Internal error: ComputeTopTypeTree() can only be called on the defined types (enums and structs)")

  


def ProcessSchema(schema : dict) -> None:
  CheckSchemaSchema(schema)
  rootName : str = schema['root_name']
  definedTypes : list = schema['types']

  # gather defined types
  allTypes : Dict[str,Type] = {}

  # gather all defined types (first pass) : structs and enums
  # we do not parse the subtypes now as to not mandate
  # a particular declaration order
  for definedType in definedTypes:
    CreateAndCacheTypeObject(allTypes,definedType)

  # now we have objects for all the defined types
  # let's build the whole type dependency tree
  for definedType in definedTypes:
    ComputeTypeTree(allTypes,definedType)





if __name__ == '__main__':
  import argparse
  parser = argparse.ArgumentParser(usage = """stonegentool.py [-h] [-o OUT_DIR] [-v] input_schemas
       EXAMPLE: python command_gen.py -o "generated_files/" "mainSchema.json,App Specific Commands.json" """)
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
