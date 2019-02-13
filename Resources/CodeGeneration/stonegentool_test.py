from stonegentool import EatToken
import unittest

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

# def test(self):
#   s = 'hello world'
#   self.assertEqual(s.split(), ['hello', 'world'])
#   # check that s.split fails when the separator is not a string
#   with self.assertRaises(TypeError):
#   s.split(2)

if __name__ == '__main__':
    unittest.main()