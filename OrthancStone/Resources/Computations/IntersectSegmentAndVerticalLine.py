#!/usr/bin/env python

from sympy import *

# Intersection between the 2D line segment (prevX,prevY)-(curX,curY) and the
# vertical line "x = x0" using homogeneous coordinates

prevX, prevY, curX, curY, x0 = symbols('prevX prevY curX curY x0')

p1 = Matrix([prevX, prevY, 1])
p2 = Matrix([curX, curY, 1])
l1 = p1.cross(p2)

h1 = Matrix([x0, 0, 1])
h2 = Matrix([x0, 1, 1])
l2 = h1.cross(h2)

a = l1.cross(l2)

pprint(a / a[2])
