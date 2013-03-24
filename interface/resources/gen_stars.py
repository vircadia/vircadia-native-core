#
# gen_stars.py
# interface
#
# Created by Tobias Schwinger on 3/22/13.
# Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
#

# Input file generator for the starfield. 

from random import random,randint
from sys import argv

n = 1000

if len(argv) > 1:
    n = int(argv[1])

for i in range(n):
    # color
    w = randint(30,randint(40,255))
    r = max(0,min(255,w + randint(-10,70)))
    g = max(0,min(255,w + randint(-20,60)))
    b = max(0,min(255,w + randint(-10,100)))
    # position
    azi = random() * 360
    alt = random() *  90
    print "%f %f #%02x%02x%02x" % (azi,alt,r,g,b)

