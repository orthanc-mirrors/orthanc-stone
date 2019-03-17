import sys
import os

# add the generation script location to the search paths
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'Resources', 'CodeGeneration'))

# import the code generation tooling script
import stonegentool

schemaFile = os.path.join(os.path.dirname(__file__), 'StoneSampleCommands.yml')
outDir = os.path.dirname(__file__)

# ignition!
stonegentool.Process(schemaFile, outDir)


