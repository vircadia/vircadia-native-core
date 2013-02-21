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
int VoxelSystem::initVoxels(Voxel * voxel, float scale, glm::vec3 * position) {
    glm::vec3 averageColor(0,0,0);
    int childrenCreated = 0;
    int newVoxels = 0;
    if (voxel == NULL) voxel = root;
    averageColor[0] = averageColor[1] = averageColor[2] = 0.0;
    //
    //  First, decide whether I should be a leaf node and set/return if so 
    //
    if ((randFloat() < 0.1) && (scale < 1.0))
    {
        voxel->color.x = voxel->color.y = voxel->color.z = 0.5 + randFloat()*0.5;
        for (unsigned char i = 0; i < NUM_CHILDREN; i++) voxel->children[i] = NULL;
        return 0;
    } else {
        // Decide whether to make kids, recurse into them 
        for (unsigned char i = 0; i < NUM_CHILDREN; i++) {
            if  ((scale > 0.01) && (randFloat() > 0.6))
            {
                //  Make a new child
                voxel->children[i] = new Voxel;
                newVoxels++;
                childrenCreated++;
                glm::vec3 shift(scale/2.0*((i&4)>>2)-scale/4.0,
                                scale/2.0*((i&2)>>1)-scale/4.0,
                                scale/2.0*(i&1)-scale/4.0);
                *position += shift;
                newVoxels += initVoxels(voxel->children[i], scale/2.0, position);
                *position -= shift;
                averageColor += voxel->children[i]->color;
            } else {
                //  No child made: Set pointer to null, nothing to see here. 
                voxel->children[i] = NULL;
            }
        }
        if (childrenCreated > 0) {
            //  If there were children created, set this voxels color to the average of it's children
            averageColor *= 1.0/childrenCreated;
            voxel->color = averageColor;
            return newVoxels;
        } else {
            //  Tested and didn't make any children, so i've still got to be a leaf
            voxel->color.x = voxel->color.y = voxel->color.z = 0.5 + randFloat()*0.5;
            for (unsigned char i = 0; i < NUM_CHILDREN; i++) voxel->children[i] = NULL;
            return 0;

        }
    }
}

const float RENDER_DISCARD = 0.01;

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
            //std::cout << "x,y,z: " << shift.x <<  "," << shift.y << "," << shift.z << "\n";
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
        //glColor4f(1,1,1,1);
        glColor4f(voxel->color.x, voxel->color.y, voxel->color.z, 1.0);
        //float bright = 1.0 - glm::length(*distance)/20.0;
        //glColor3f(bright,bright,bright);
        glutSolidCube(scale);
        vRendered++;
    }
    return vRendered;
}

void VoxelSystem::simulate(float deltaTime) {
    
}





