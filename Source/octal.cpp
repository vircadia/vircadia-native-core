//
//  octal.cpp
//  interface
//
//  Created by Philip on 2/4/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Various subroutines for converting between X,Y,Z coords and octree coordinates.
//  

const int X = 0;
const int Y = 1;
const int Z = 2;

#include "util.h"
#include "octal.h"

//  Given a position vector between zero and one (but less than one), and a voxel scale 1/2^scale,
//  returns the smallest voxel at that scale which encloses the given point.
void getVoxel(float * pos, int scale, float * vpos) {
    float vscale = powf(2, scale);
    vpos[X] = floor(pos[X]*vscale)/vscale;
    vpos[Y] = floor(pos[Y]*vscale)/vscale;
    vpos[Z] = floor(pos[Z]*vscale)/vscale;
}

