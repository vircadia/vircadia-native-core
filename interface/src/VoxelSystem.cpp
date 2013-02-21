//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "VoxelSystem.h"


bool onSphereShell(float radius, float scale, glm::vec3 * position) {
    float vRadius = glm::length(*position);
    return ((vRadius + scale/2.0 > radius) && (vRadius - scale/2.0 < radius));
}

void VoxelSystem::init() {
    root = new Voxel;
}

//
//  Recursively initialize the voxel tree
//
int VoxelSystem::initVoxels(Voxel * voxel, float scale, glm::vec3 * position) {
    glm::vec3 averageColor(0,0,0);
    int childrenCreated = 0;
    int newVoxels = 0;
    if (voxel == NULL) voxel = root;
    averageColor[0] = averageColor[1] = averageColor[2] = 0.0;
    
    const float RADIUS = 3.9;
    
    //
    //  First, randomly decide whether to stop here without recursing for children 
    //
    if (onSphereShell(RADIUS, scale, position) && (scale < 0.25) && (randFloat() < 0.01))
    {
        voxel->color.x = 0.1;
        voxel->color.y = 0.5 + randFloat()*0.5;
        voxel->color.z = 0.1;
        for (unsigned char i = 0; i < NUM_CHILDREN; i++) voxel->children[i] = NULL;
        return 0;
    } else {
        // Decide whether to make kids, recurse into them 
        for (unsigned char i = 0; i < NUM_CHILDREN; i++) {
            if  (scale > 0.01)              {
                glm::vec3 shift(scale/2.0*((i&4)>>2)-scale/4.0,
                                scale/2.0*((i&2)>>1)-scale/4.0,
                                scale/2.0*(i&1)-scale/4.0);
                *position += shift;
                //  Test to see whether the child is also on edge of sphere
                if (onSphereShell(RADIUS, scale/2.0, position)) {
                    voxel->children[i] = new Voxel;
                    newVoxels++;
                    childrenCreated++;
                    newVoxels += initVoxels(voxel->children[i], scale/2.0, position);
                    averageColor += voxel->children[i]->color;
                } else voxel->children[i] = NULL;
                *position -= shift;
            } else {
                //  No child made: Set pointer to null, nothing to see here. 
                voxel->children[i] = NULL;
            }
        }
        if (childrenCreated > 0) {
            //  If there were children created, the color of this voxel node is average of children
            averageColor *= 1.0/childrenCreated;
            voxel->color = averageColor;
            return newVoxels;
        } else {
            //  Tested and didn't make any children, so choose my color as a leaf, return
            voxel->color.x = voxel->color.y = voxel->color.z = 0.5 + randFloat()*0.5;
            for (unsigned char i = 0; i < NUM_CHILDREN; i++) voxel->children[i] = NULL;
            return 0;

        }
    }
}

//
//  The Render Discard is the ratio of the size of the voxel to the distance from the camera
//  at which the voxel will no longer be shown.  Smaller = show more detail.  
//  

const float RENDER_DISCARD = 0.04;  //0.01;

//
//  Returns the total number of voxels actually rendered
//
int VoxelSystem::render(Voxel * voxel, float scale, glm::vec3 * distance) {
    // If null passed in, start at root
    if (voxel == NULL) voxel = root;    
    unsigned char i;
    bool renderedChildren = false;
    int vRendered = 0;
    // Recursively render children
    for (i = 0; i < NUM_CHILDREN; i++) {
        glm::vec3 shift(scale/2.0*((i&4)>>2)-scale/4.0,
                        scale/2.0*((i&2)>>1)-scale/4.0,
                        scale/2.0*(i&1)-scale/4.0);
        if ((voxel->children[i] != NULL) && (scale / glm::length(*distance) > RENDER_DISCARD)) {
            glTranslatef(shift.x, shift.y, shift.z);
            *distance += shift;
            vRendered += render(voxel->children[i], scale/2.0, distance);
            *distance -= shift;
            glTranslatef(-shift.x, -shift.y, -shift.z);
            renderedChildren = true;
        }
    }
    //  Render this voxel if the children were not rendered 
    if (!renderedChildren)
    {
        //  This is the place where we need to copy this data to a VBO to make this FAST
        glColor4f(voxel->color.x, voxel->color.y, voxel->color.z, 1.0);
        glutSolidCube(scale);
        vRendered++;
    }
    return vRendered;
}

void VoxelSystem::simulate(float deltaTime) {
    
}





