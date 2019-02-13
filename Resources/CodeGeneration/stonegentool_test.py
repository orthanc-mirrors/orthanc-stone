from stonegentool import \
EatToken,SplitListOfTypes,ParseTemplateType,LoadSchema,CheckSchemaSchema,ProcessSchema
import unittest
import os

class TestStonegentool(unittest.TestCase):
  def test_EatToken_empty(self):
    c = r""
    a,b = EatToken(c)
    self.assertEqual(a,r"")
    self.assertEqual(b,r"")

  def test_EatToken_simpleNonTemplate(self):
    c = r"int32"
    a,b = EatToken(c)
    self.assertEqual(a,r"int32")
    self.assertEqual(b,r"")

  def test_EatToken_simpleTemplate(self):
    c = r"vector<string>"
    a,b = EatToken(c)
    self.assertEqual(a,r"vector<string>")
    self.assertEqual(b,r"")

  def test_EatToken_complexTemplate(self):
    c = r"vector<map<int64,string>>,vector<map<int32,string>>"
    a,b = EatToken(c)
    self.assertEqual(a,r"vector<map<int64,string>>")
    self.assertEqual(b,r"vector<map<int32,string>>")

  def test_EatToken_complexTemplates(self):
    c = r"vector<map<vector<string>,map<int32,string>>>,map<int32,string>,map<map<int32,string>,string>"
    a,b = EatToken(c)
    self.assertEqual(a,r"vector<map<vector<string>,map<int32,string>>>")
    self.assertEqual(b,r"map<int32,string>,map<map<int32,string>,string>")
    a,b = EatToken(b)
    self.assertEqual(a,r"map<int32,string>")
    self.assertEqual(b,r"map<map<int32,string>,string>")

  def test_SplitListOfTypes(self):
    c = r"vector<map<vector<string>,map<int32,string>>>,map<int32,string>,map<map<int32,string>,string>"
    lot = SplitListOfTypes(c)
    self.assertEqual(3,len(lot))
    self.assertEqual("vector<map<vector<string>,map<int32,string>>>",lot[0])
    self.assertEqual("map<int32,string>",lot[1])
    self.assertEqual("map<map<int32,string>,string>",lot[2])

  def test_SplitListOfTypes_bogus(self):
    c = r"vector<map<vector<string>,map<int32,string>>,map<int32,string>,map<map<int32,string>,string"
    self.assertRaises(Exception,SplitListOfTypes,c) # the argument c must be passed to assertRaises, not as a normal call of SplitListOfTypes
    
  def test_ParseTemplateType_true(self):
    c = "map<vector<map<int,vector<string>>>,map<vector<int>,vector<string>>>"
    (ok,a,b) = ParseTemplateType(c)
    self.assertEqual(ok,True)
    self.assertEqual(a,"map")
    self.assertEqual(b,["vector<map<int,vector<string>>>","map<vector<int>,vector<string>>"])

    (ok2,a2,b2) = ParseTemplateType(b[0])
    self.assertEqual(ok2,True)
    self.assertEqual(a2,"vector")
    self.assertEqual(b2,["map<int,vector<string>>"])

    (ok3,a3,b3) = ParseTemplateType(b[1])
    self.assertEqual(ok3,True)
    self.assertEqual(a3,"map")
    self.assertEqual(b3,["vector<int>","vector<string>"])

    (ok4,a4,b4) = ParseTemplateType(b2[0])
    self.assertEqual(ok4,True)
    self.assertEqual(a4,"map")
    self.assertEqual(b4,["int","vector<string>"])
    
  def test_ParseSchema(self):
    fn = os.path.join(os.path.dirname(__file__), 'test', 'test1.jsonc')
    obj = LoadSchema(fn)
    # we're happy if it does not crash
    CheckSchemaSchema(obj)

  def test_ParseSchema_bogus_json(self):
    fn = os.path.join(os.path.dirname(__file__), 'test', 'test1_bogus_json.jsonc')
    self.assertRaises(Exception,LoadSchema,fn)

  def test_ParseSchema_bogus_schema(self):
    fn = os.path.join(os.path.dirname(__file__), 'test', 'test1_bogus_schema.jsonc')
    obj = LoadSchema(fn)
    self.assertRaises(Exception,CheckSchemaSchema,obj) 

  def test_GenOrderQueue(self):
    fn = os.path.join(os.path.dirname(__file__), 'test', 'test1.jsonc')
    obj = LoadSchema(fn)
    genOrderQueue,structTypes = ProcessSchema(obj)
    print(f"genOrderQueue = {genOrderQueue}")
    print("")

  def test_GenerateTypeScriptEnumeration(self):
    pass

  def test_GenerateCppEnumeration(self):
    pass

  def test_GenerateTypeScriptClasses(self):
    pass

  def test_GenerateCppClasses(self):
    pass

  def test_GenerateTypeScriptHandlerInterface(self):
    pass

  def test_GenerateCppHandlerInterface(self):
    pass

  def test_GenerateTypeScriptDispatcher(self):
    pass

  def test_GenerateCppDispatcher(self):
    pass

# def test(self):
#   s = 'hello world'
#   self.assertEqual(s.split(), ['hello', 'world'])
#   # check that s.split fails when the separator is not a string
#   with self.assertRaises(TypeError):
#   s.split(2)

if __name__ == '__main__':
  print("")
  unittest.main()
