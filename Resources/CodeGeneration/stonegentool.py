import json
import yaml
import re
import os
import sys
from jinja2 import Template
from io import StringIO
import time
import datetime

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

class FieldDefinition:

    def __init__(self, name: str, type: str, defaultValue: str):
        self.name = name
        self.type = type
        self.defaultValue = defaultValue

    @staticmethod
    def fromKeyValue(key: str, value: str):

        if "=" in value:
            splitValue = value.split(sep="=")
            type = splitValue[0].strip(" ")
            defaultValue = splitValue[1].strip(" ")
        else:
            type = value
            defaultValue = None

        return FieldDefinition(name = key, type = type, defaultValue = defaultValue)


def LoadSchemaFromJson(filePath):
    return JsonHelpers.loadJsonWithComments(filePath)

def CanonToCpp(canonicalTypename):
  # C++: prefix map vector and string with std::map, std::vector and
  # std::string
  # replace int32... by int32_t...
  # replace float32 by float
  # replace float64 by double
  retVal = canonicalTypename
  retVal = retVal.replace("map", "std::map")
  retVal = retVal.replace("vector", "std::vector")
  retVal = retVal.replace("set", "std::set")
  retVal = retVal.replace("string", "std::string")
  #uint32 and uint64 are handled by int32 and uint32 (because search and replace are done as partial words)
  retVal = retVal.replace("int32", "int32_t")
  retVal = retVal.replace("int64", "int64_t")
  retVal = retVal.replace("float32", "float")
  retVal = retVal.replace("float64", "double")
  retVal = retVal.replace("json", "Json::Value")
  return retVal

def CanonToTs(canonicalTypename):
  # TS: replace vector with Array and map with Map
  # string remains string
  # replace int32... by number
  # replace float32... by number
  retVal = canonicalTypename
  retVal = retVal.replace("map", "Map")
  retVal = retVal.replace("vector", "Array")
  retVal = retVal.replace("set", "Set")
  retVal = retVal.replace("uint32", "number")
  retVal = retVal.replace("uint64", "number")
  retVal = retVal.replace("int32", "number")
  retVal = retVal.replace("int64", "number")
  retVal = retVal.replace("float32", "number")
  retVal = retVal.replace("float64", "number")
  retVal = retVal.replace("bool", "boolean")
  retVal = retVal.replace("json", "Object")
  return retVal

def NeedsTsConstruction(enums, tsType):
  if tsType == 'boolean':
    return False
  elif tsType == 'number':
    return False
  elif tsType == 'string':
    return False
  else:
    enumNames = []
    for enum in enums:
      enumNames.append(enum['name'])
    if tsType in enumNames:
      return False
  return True

def NeedsCppConstruction(canonTypename):
  return False

def DefaultValueToTs(enums, field:FieldDefinition):
    tsType = CanonToTs(field.type)

    enumNames = []
    for enum in enums:
        enumNames.append(enum['name'])

    if tsType in enumNames:
        return tsType + "." + field.defaultValue
    else:
        return field.defaultValue

def DefaultValueToCpp(root, enums, field:FieldDefinition):
    cppType = CanonToCpp(field.type)

    enumNames = []
    for enum in enums:
        enumNames.append(enum['name'])

    if cppType in enumNames:
        return root + "::" + cppType + "_" + field.defaultValue
    else:
        return field.defaultValue

def RegisterTemplateFunction(template,func):
  """Makes a function callable by a jinja2 template"""
  template.globals[func.__name__] = func
  return func

def MakeTemplate(templateStr):
  template = Template(templateStr)
  RegisterTemplateFunction(template,CanonToCpp)
  RegisterTemplateFunction(template,CanonToTs)
  RegisterTemplateFunction(template,NeedsTsConstruction)
  RegisterTemplateFunction(template,NeedsCppConstruction)
  RegisterTemplateFunction(template, DefaultValueToTs)
  RegisterTemplateFunction(template, DefaultValueToCpp)
  RegisterTemplateFunction(template, sorted)
  return template

def MakeTemplateFromFile(templateFileName):

  with open(templateFileName, "r") as templateFile:
    templateFileContents = templateFile.read()
    return MakeTemplate(templateFileContents)


def EatToken(sentence):
    """splits "A,B,C" into "A" and "B,C" where A, B and C are type names
  (including templates) like "int32", "TotoTutu", or 
  "map<map<int32,vector<string>>,map<string,int32>>" """

    if sentence.count("<") != sentence.count(">"):
        raise Exception(
            "Error in the partial template type list " + str(sentence) + "."
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


def SplitListOfTypes(typename):
    """Splits something like
  vector<string>,int32,map<string,map<string,int32>> 
  in:
  - vector<string>
  - int32
    map<string,map<string,int32>>
  
  This is not possible with a regex so 
  """
    stillStuffToEat = True
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


def ParseTemplateType(typename):
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
        m = matches
        assert len(m.groups()) == 2
        # we need to split with the commas that are outside of the
        # defined types. Simply splitting at commas won't work
        listOfDependentTypes = SplitListOfTypes(m.group(2))
        return (True, m.group(1), listOfDependentTypes)

def GetStructFields(struct):
  """This filters out the special metadata key from the struct fields"""
  return [k for k in struct.keys() if k != '__handler']

def ComputeOrderFromTypeTree(
  ancestors, 
  genOrder, 
  shortTypename, schema):

  if shortTypename in ancestors:
    raise Exception(
      "Cyclic dependency chain found: the last of " + str(ancestors) +
      + " depends on " + str(shortTypename) + " that is already in the list."
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
        struct = schema[GetLongTypename(shortTypename, schema)]
        # The keys in the struct dict are the member names
        # The values in the struct dict are the member types
        if struct:
          # we reach this if struct is not None AND not empty
          for field in GetStructFields(struct):
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

def IsShortStructType(typename, schema):
  fullStructName = "struct " + typename
  return (fullStructName in schema)

def GetLongTypename(shortTypename, schema):
  if shortTypename.startswith("enum "):
    raise RuntimeError('shortTypename.startswith("enum "):')
  enumName = "enum " + shortTypename
  isEnum = enumName in schema

  if shortTypename.startswith("struct "):
    raise RuntimeError('shortTypename.startswith("struct "):')
  structName = "struct " + shortTypename
  isStruct = ("struct " + shortTypename) in schema

  if isEnum and isStruct:
    raise RuntimeError('Enums and structs cannot have the same name')

  if isEnum:
    return enumName
  if isStruct:
    return structName

def IsTypename(fullName):
  return (fullName.startswith("enum ") or fullName.startswith("struct "))

def IsEnumType(fullName):
  return fullName.startswith("enum ")

def IsStructType(fullName):
  return fullName.startswith("struct ")

def GetShortTypename(fullTypename):
  if fullTypename.startswith("struct "):
    return fullTypename[7:] 
  elif fullTypename.startswith("enum"):
    return fullTypename[5:] 
  else:
    raise RuntimeError \
      ('fullTypename should start with either "struct " or "enum "')

def CheckSchemaSchema(schema):
  if not "rootName" in schema:
      raise Exception("schema lacks the 'rootName' key")
  for name in schema.keys():
    if (not IsEnumType(name)) and (not IsStructType(name)) and \
      (name != 'rootName'):
      raise RuntimeError \
        ('Type "' + str(name) + '" should start with "enum " or "struct "')

  # TODO: check enum fields are unique (in whole namespace)
  # TODO: check struct fields are unique (in each struct)
  # TODO: check that in the source schema, there are spaces after each colon

nonTypeKeys = ['rootName']
def GetTypesInSchema(schema):
  """Returns the top schema keys that are actual type names"""
  typeList = [k for k in schema if k not in nonTypeKeys]
  return typeList

# +-----------------------+
# | Main processing logic |
# +-----------------------+

def ComputeRequiredDeclarationOrder(schema):
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
  genOrder = []
  for fullName in GetTypesInSchema(schema):
    if IsStructType(fullName):
      realName = GetShortTypename(fullName)
      ancestors = []
      ComputeOrderFromTypeTree(ancestors, genOrder, realName, schema)
  return genOrder

def GetStructFields(fieldDict):
  """Returns the regular (non __handler) struct fields"""
  # the following happens for empty structs
  if fieldDict == None:
    return fieldDict
  ret = {}
  for k,v in fieldDict.items():
    if k != "__handler":
      ret[k] = FieldDefinition.fromKeyValue(k, v)
    if k.startswith("__") and k != "__handler":
      raise RuntimeError("Fields starting with __ (double underscore) are reserved names!")
  return ret

def GetStructMetadata(fieldDict):
  """Returns the __handler struct fields (there are default values that
     can be overridden by entries in the schema
     Not tested because it's a fail-safe: if something is broken in this, 
     dependent projects will not build."""
  metadataDict = {}
  metadataDict['handleInCpp'] = False
  metadataDict['handleInTypescript'] = False
  
  if fieldDict != None:
    for k,v in fieldDict.items():
      if k.startswith("__") and k != "__handler":
        raise RuntimeError("Fields starting with __ (double underscore) are reserved names")
      if k == "__handler":
        if type(v) == list:
          for i in v:
            if i == "cpp":
              metadataDict['handleInCpp'] = True
            elif i == "ts":
              metadataDict['handleInTypescript'] = True
            else:
              raise RuntimeError("Error in schema. Allowed values for __handler are \"cpp\" or \"ts\"")
        elif type(v) == str:
          if v == "cpp":
            metadataDict['handleInCpp'] = True
          elif v == "ts":
            metadataDict['handleInTypescript'] = True
          else:
            raise RuntimeError("Error in schema. Allowed values for __handler are \"cpp\" or \"ts\" (or a list of both)")
        else:
            raise RuntimeError("Error in schema. Allowed values for __handler are \"cpp\" or \"ts\" (or a list of both)")
  return metadataDict

def ProcessSchema(schema, genOrder):
  # sanity check
  CheckSchemaSchema(schema)

  # let's doctor the schema to clean it up a bit
  # order DOES NOT matter for enums, even though it's a list
  enums = []
  for fullName in schema.keys():
    if IsEnumType(fullName):
      # convert "enum Toto" to "Toto"
      typename = GetShortTypename(fullName)
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

  structs = []
  for i in range(len(genOrder)):
    # this is already the short name
    typename = genOrder[i]
    fieldDict = schema["struct " + typename]
    struct = {}
    struct['name'] = typename
    struct['fields'] = GetStructFields(fieldDict)
    struct['__meta__'] = GetStructMetadata(fieldDict)
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
  # latin-1 is a trick, when we do NOT care about NON-ascii chars but
  # we wish to avoid using a decoding error handler
  # (see http://python-notes.curiousefficiency.org/en/latest/python3/text_file_processing.html#files-in-an-ascii-compatible-encoding-best-effort-is-acceptable)
  # TL;DR: all 256 values are mapped to characters in latin-1 so the file 
  # contents never cause an error.
  with open(fn, 'r', encoding='latin-1') as f:
    schemaText = f.read()
    assert(type(schemaText) == str)
  return LoadSchemaFromString(schemaText = schemaText)

def LoadSchemaFromString(schemaText:str):
    # ensure there is a space after each colon. Otherwise, dicts could be
    # erroneously recognized as an array of strings containing ':'
    for i in range(len(schemaText)-1):
      ch = schemaText[i]
      nextCh = schemaText[i+1]
      if ch == ':':
        if not (nextCh == ' ' or nextCh == '\n'):
          lineNumber = schemaText.count("\n",0,i) + 1
          raise RuntimeError("Error at line " + str(lineNumber) + " in the schema: colons must be followed by a space or a newline!")
    schema = yaml.load(schemaText, Loader = yaml.SafeLoader)
    return schema

def GetTemplatingDictFromSchemaFilename(fn):
  return GetTemplatingDictFromSchema(LoadSchema(fn))

def GetTemplatingDictFromSchema(schema):
  genOrder = ComputeRequiredDeclarationOrder(schema)
  templatingDict = ProcessSchema(schema, genOrder)
  currentDT = datetime.datetime.now()
  templatingDict['currentDatetime'] = str(currentDT)
  return templatingDict

# +-----------------------+
# |      ENTRY POINT      |
# +-----------------------+
def Process(schemaFile, outDir):
  tdico = GetTemplatingDictFromSchemaFilename(schemaFile)

  tsTemplateFile = \
    os.path.join(os.path.dirname(__file__), 'template.in.ts.j2')
  template = MakeTemplateFromFile(tsTemplateFile)
  renderedTsCode = template.render(**tdico)
  outputTsFile = os.path.join( \
    outDir,str(tdico['rootName']) + "_generated.ts")
  with open(outputTsFile,"wt",encoding='utf8') as outFile:
    outFile.write(renderedTsCode)

  cppTemplateFile = \
    os.path.join(os.path.dirname(__file__), 'template.in.h.j2')
  template = MakeTemplateFromFile(cppTemplateFile)
  renderedCppCode = template.render(**tdico)
  outputCppFile = os.path.join( \
    outDir, str(tdico['rootName']) + "_generated.hpp")
  with open(outputCppFile,"wt",encoding='utf8') as outFile:
    outFile.write(renderedCppCode)

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
  schemaFile = args.input_schema
  outDir = args.out_dir
  Process(schemaFile, outDir)
