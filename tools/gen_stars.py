#!/usr/bin/env python

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

hemisphere_only, equator, meridians=False, 0, 1000

n_random = 50000
if len(argv) > 1:
    n_random = int(argv[1])

bars_total, bars_prev = 77, 0

def meridian(azimuth,n,(r0,g0,b0),(r1,g1,b1)):
    alts = 180.0/n
    for alti in range(n):
        # color 
        altj = n-alti-1
        r = (r0 *altj+alti* r1)/n
        g = (g0 *altj+alti* g1)/n
        b = (b0 *altj+alti* b1)/n
        # position
        altitude = alts*alti
        print "%f %f #%02x%02x%02x" % (azimuth,altitude,r,g,b)
        print "%f %f #%02x%02x%02x" % (azimuth,-altitude,r,g,b)

#if meridians:
    #meridian( 0,meridians,(255,255,255), (180, 60,255)) # N->S
    #meridian(90,meridians,( 80,255, 80), (255,240, 40)) # E->W

if equator:
    azis = 360.0/equator
    for azii in range(equator):
        azimuth = azis*azii
        print "%f %f #%02x%02x%02x" % (azimuth,0,255,255,255)

for i in range(n_random):
    # color
    w = randint(30,randint(40,255))
    r = max(0,min(255,w + randint(-10,70)))
    g = max(0,min(255,w + randint(-20,60)))
    b = max(0,min(255,w + randint(-10,100)))
    # position
    while True:
        x,y,z = random()*2-1,random(),random()*2-1
        if not hemisphere_only:
            y = y*2-1
        l = sqrt(x*x + y*y + z*z)
        if l <= 1.0:
            break
    x /= l; y /= l; z /= l
    xz = hypot(x,z)

    azimuth = degrees(fmod(atan2(x,z)+pi,2*pi))
    altitude = degrees(atan2(y,xz))

    bars = round(bars_total*i/n_random)
    if bars != bars_prev:
        bars_prev = bars
        bars = int(bars)
        stderr.write('\r[%s%s]' % ('#' * bars, '-' * (bars_total-bars)))

    print "%f %f #%02x%02x%02x" % (azimuth,altitude,r,g,b)

stderr.write('\r[%s]\n' % ('#' * bars_total,))

