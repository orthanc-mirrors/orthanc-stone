#!/usr/bin/python

import array
import matplotlib.pyplot as plt

def GenerateColormap(name):
    colormap = []

    for gray in range(256):
        if name == 'red':
            color = (gray / 255.0, 0, 0)
        elif name == 'green':
            color = (0, gray / 255.0, 0)
        elif name == 'blue':
            color = (0, 0, gray / 255.0)
        else:
            color = plt.get_cmap(name) (gray)

        colormap += map(lambda k: int(round(color[k] * 255)), range(3))

    colormap[0] = 0
    colormap[1] = 0
    colormap[2] = 0

    return array.array('B', colormap).tostring()


for name in [ 
        'hot', 
        'jet', 
        'blue',
        'green',
        'red',
]:
    with open('%s.lut' % name, 'w') as f:
        f.write(GenerateColormap(name))
