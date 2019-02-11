from __future__ import print_function
import sys






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
  input_schemas = args.input_schemas.split(",")
  out_dir = args.out_dir

  print("input schemas = " + str(input_schemas))
  print("out dir = " + str(out_dir))

