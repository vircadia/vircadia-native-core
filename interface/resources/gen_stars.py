#
# gen_stars.py
# interface
#
# Created by Tobias Schwinger on 3/22/13.
# Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
#

# Input file generator for the starfield. 

from random import random,randint
from math import sqrt, hypot, atan2, pi, fmod, degrees
from sys import argv,stderr


n = 10
if len(argv) > 1:
    n = int(argv[1])

hemisphere=False

bars_total, bars_prev = 77, 0

for i in range(n):
    # color
    w = randint(30,randint(40,255))
    r = max(0,min(255,w + randint(-10,70)))
    g = max(0,min(255,w + randint(-20,60)))
    b = max(0,min(255,w + randint(-10,100)))
    # position
    x,y,z = random()*2-1,random(),random()*2-1
    if not hemisphere:
        y = y*2-1
    l = sqrt(x*x + y*y + z*z)
    x /= l; y /= l; z /= l
    xz = hypot(x,z)

    azimuth = degrees(fmod(atan2(x,z)+pi,2*pi))
    altitude = degrees(atan2(y,xz))

    bars = round(bars_total*i/n)
    if bars != bars_prev:
        bars_prev = bars
        bars = int(bars)
        stderr.write('\r[%s%s]' % ('#' * bars, '-' * (bars_total-bars)))

    print "%f %f #%02x%02x%02x" % (azimuth,altitude,r,g,b)

stderr.write('\r[%s]\n' % ('#' * bars_total,))

