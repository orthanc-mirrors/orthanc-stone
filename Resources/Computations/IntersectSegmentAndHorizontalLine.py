#!/usr/bin/env python

from sympy import *

# Intersection between the 2D line segment (prevX,prevY)-(curX,curY) and the
# horizontal line "y = y0" using homogeneous coordinates

prevX, prevY, curX, curY, y0 = symbols('prevX prevY curX curY y0')

p1 = Matrix([prevX, prevY, 1])
p2 = Matrix([curX, curY, 1])
l1 = p1.cross(p2)

h1 = Matrix([0, y0, 1])
h2 = Matrix([1, y0, 1])
l2 = h1.cross(h2)

a = l1.cross(l2)

#pprint(cse(a/a[2], symbols = symbols('a b')))
pprint(a / a[2])
