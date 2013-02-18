//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "VoxelSystem.h"

void VoxelSystem::init() {
    root = new Voxel;
}

//
//  Recursively initialize the voxel tree
//
int VoxelSystem::initVoxels(Voxel * voxel, float scale) {
    float childColor[3], averageColor[3];
    int averageCount = 0;
    int newVoxels = 0;
    if (voxel == NULL) voxel = root;
    averageColor[0] = averageColor[1] = averageColor[2] = 0.0;
    for (unsigned char i = 0; i < NUM_CHILDREN; i++) {
        if ((scale > 0.01) && (randFloat() < 0.5)) {
            voxel->children[i] = new Voxel;
            newVoxels += initVoxels(voxel->children[i], scale/2.0);
            for (int j = 0; j < 3; j++) averageColor[j] += childColor[j];
            averageCount++;
        }
        else {
            voxel->children[i] = NULL;
        }
    }
    if (averageCount == 0) {
        //  This is a leaf, so just pick a random color
        voxel->color.x = voxel->color.y = voxel->color.z = randFloat();
    } else {
        voxel->color.x = averageColor[0]/averageCount;
        voxel->color.y = averageColor[1]/averageCount;
        voxel->color.z = averageColor[2]/averageCount;
    }
    newVoxels++;
    return newVoxels;
}

void VoxelSystem::render(Voxel * voxel, float scale) {
    if (voxel == NULL) voxel = root;
    unsigned char i;
    for (i = 0; i < NUM_CHILDREN; i++) {
        if (voxel->children[i] != NULL) {
            glTranslatef(scale/2.0*((i&4)>>2), scale/2.0*((i&2)>>1), scale/2.0*(i&1));
            render(voxel->children[i], scale/2.0);
            glTranslatef(-scale/2.0*((i&4)>>2), -scale/2.0*((i&2)>>1), -scale/2.0*(i&1));
        }
    }
    glColor4f(voxel->color.x, voxel->color.y, voxel->color.z, 0.5);
    glutSolidCube(scale);
}

void VoxelSystem::simulate(float deltaTime) {
    
}





