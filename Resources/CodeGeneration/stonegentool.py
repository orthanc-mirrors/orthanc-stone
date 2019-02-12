from __future__ import print_function
import sys, json

def LoadSchema(file_path : str):
    with open(file_path, 'r') as fp:
      obj = json.load(fp)
    return obj

class Type:
  def __init__(self, canonicalTypeName:str, dependentTypes:str):
    """dependent type is the list of canonical types this type depends on.
       For instance, vector<map<string,int32>> depends on map<string,int32>
       that, in turn, depends on string and int32 that, in turn, depend on
       nothing"""
    self.canonicalTypeName = canonicalTypeName
    self.dependentTypes = dependentTypes
  def getDependentTypes(self):
    return self.dependentTypes
  def getCppTypeName(self):
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
  def getTypeScriptTypeName(self):
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
    self.root_prefix : str = root_prefix
    self.defined_types : str = defined_types

def ProcessSchemaPaths(inputSchemas : list[str])


def ProcessSchemas(schemaPaths):
  schemas = []
  for schemaPath in schemaPaths:
    schemas.append(LoadSchema(schemaPath))
  return schemas

if __name__ == '__main__':
  import argparse
  parser = argparse.ArgumentParser(usage = """stonegentool.py [-h] [-o OUT_DIR] [-v] input_schemas
       EXAMPLE: python command_gen.py -o "generated_files/" "mainSchema.json,App Specific Commands.json" """)
  parser.add_argument("input_schemas", type=str,
                      help = "one or more schema files, as a comma-separated list of paths")
  parser.add_argument("-o", "--out_dir", type=str, default=".", 
                      help = """path of the directory where the files 
                                will be generated. Default is current
                                working folder""")
  parser.add_argument("-v", "--verbosity", action="count", default=0,
                      help = """increase output verbosity (0 == errors 
                                only, 1 == some verbosity, 2 == nerd
                                mode""")

  args = parser.parse_args()
  inputSchemas = args.input_schemas.split(",")
  outDir = args.out_dir

  print("input schemas = " + str(inputSchemas))
  print("out dir = " + str(outDir))

  ProcessSchemaPaths(inputSchemas)



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
