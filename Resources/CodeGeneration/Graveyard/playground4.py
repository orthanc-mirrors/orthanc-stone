testYaml = """
enum SomeEnum:
 - january
 - feb

struct Message0:
 a: string

struct Message1:
 a: string
 b: int32
 c: vector<Message0>
 d: SomeEnum = january
 e: SomeEnum= january
 f: SomeEnum=january
 g: SomeEnum =january
  

# github.com/AlDanial/cloc
header2 : 
  cloc_version       : 1.67
  elapsed_seconds    : int32_t

header : 
  cloc_version       : 1.67
  elapsed_seconds    : int32_t
  cloc_url           : vector<map<string,int32>>
  n_files            : 1
  n_lines            : 3
  files_per_second   : 221.393718659277
  lines_per_second   : 664.181155977831
  report_file        : IDL.idl.yaml
IDL :
  nFiles: 1
  blank: 0
  comment: 2
  code: 1
EnumSUM: 
  - aaa
  - bbb

SUM: 
  blank: 0
  comment: 2
  code: 1
  nFiles: 1
"""

import yaml

b = yaml.load(testYaml)
print(b)

c = {
  'enum SomeEnum': ['january', 'feb'], 
  'struct Message0': {'a': 'string'}, 
  'struct Message1': {
    'a': 'string', 
    'b': 'int32', 
    'c': 'vector<Message0>', 
    'd': 'vector<map<string,int32>>', 
    'e': 'SomeEnum= january', 
    'f': 'SomeEnum=january', 
    'g': 'SomeEnum =january'
  }, 
}

print(c)