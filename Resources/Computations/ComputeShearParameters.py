#!/usr/bin/python

from sympy import *
import pprint

init_printing(use_unicode=True)

s13, s23, s43 = symbols('s13 s23 s43')
x, y, z, w = symbols('x y z w')

A = Matrix([[ 1, 0, s13, 0 ],
            [ 0, 1, s23, 0 ],
            [ 0, 0, 1,   0 ],
            [ 0, 0, s43, 1 ]])

print('\nLacroute\'s shear matrix (A.14) is:')
pprint.pprint(A)

# At this point, we can write "print(A*p)". However, we don't care
# about the output "z" axis, as it is fixed. So we delete the 3rd row
# of A.

A.row_del(2)

p = Matrix([ x, y, z, 1 ])

v = A*p
print('\nAction of Lacroute\'s shear matrix on plane z (taking w=1):\n%s\n' % v)

print('Scaling = %s' % (1/v[2]))
print('Offset X = %s' % (v[0]/v[2]).subs(x, 0))
print('Offset Y = %s' % (v[1]/v[2]).subs(y, 0))
