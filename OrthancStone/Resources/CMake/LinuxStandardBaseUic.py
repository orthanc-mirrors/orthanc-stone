#!/usr/bin/env python

import subprocess
import sys

if len(sys.argv) <= 1:
    sys.stderr.write('Please provide arguments for uic\n')
    sys.exit(-1)

path = ''
pos = 1
while pos < len(sys.argv):
    if sys.argv[pos].startswith('-'):
        pos += 2
    else:
        path = sys.argv[pos]
        break

if len(path) == 0:
    sys.stderr.write('Unable to find the input file in the arguments to uic\n')
    sys.exit(-1)

with open(path, 'r') as f:
    lines = f.read().split('\n')
    if (len(lines) > 1 and
        lines[0].startswith('<?')):
        content = '\n'.join(lines[1:])
    else:
        content = '\n'.join(lines)
        
# Remove the source file from the arguments
args = sys.argv[1:pos] + sys.argv[pos+1:]

p = subprocess.Popen([ '/opt/lsb/bin/uic' ] + args,
                     stdin = subprocess.PIPE)
p.communicate(input = content)
