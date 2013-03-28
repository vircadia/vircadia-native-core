//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cmath>
#include <iostream> // to load voxels from file
#include <fstream> // to load voxels from file
#include <SharedUtil.h>
#include <OctalCode.h>
#include "VoxelSystem.h"

const int MAX_VOXELS_PER_SYSTEM = 1500000; //250000;

const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int INDICES_PER_VOXEL = 3 * 12;

float identityVertices[] = { 0, 0, 0,
                            1, 0, 0,
                            1, 1, 0,
                            0, 1, 0,
                            0, 0, 1,
                            1, 0, 1,
                            1, 1, 1,
                            0, 1, 1 };

GLubyte identityIndices[] = { 0,1,2, 0,2,3,
                              0,1,5, 0,4,5,
                              0,3,7, 0,4,7,
                              1,2,6, 1,5,6,
                              2,3,7, 2,6,7,
                              4,5,6, 4,6,7 };

VoxelSystem::VoxelSystem() {
    voxelsRendered = 0;
    tree = new VoxelTree();
}

VoxelSystem::~VoxelSystem() {    
    delete[] verticesArray;
    delete[] colorsArray;
    delete tree;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelSystem::loadVoxelsFile()
// Description: Loads HiFidelity encoded Voxels from a binary file. The current file
//              format is a stream of single voxels with NO color data. Currently
//              colors are set randomly
// Complaints:  Brad :)
// To Do:       Need to add color data to the file.
void VoxelSystem::loadVoxelsFile(const char* fileName, bool wantColorRandomizer) {
    
    tree->loadVoxelsFile(fileName,wantColorRandomizer);

    // reset the verticesEndPointer so we're writing to the beginning of the array
    verticesEndPointer = verticesArray;
    // call recursive function to populate in memory arrays
    // it will return the number of voxels added
    voxelsRendered = treeToArrays(tree->rootNode);
    // set the boolean if there are any voxels to be rendered so we re-fill the VBOs
    voxelsToRender = (voxelsRendered > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelSystem::createSphere()
// Description: Creates a sphere of voxels in the local system at a given location/radius
// To Do:       Move this function someplace better? I put it here because we need a
//              mechanism to tell the system to redraw it's arrays after voxels are done
//              being added. This is a concept mostly only understood by VoxelSystem.
// Complaints:  Brad :)
void VoxelSystem::createSphere(float r,float xc, float yc, float zc, float s, bool solid) {

    tree->createSphere(r,xc,yc,zc,s,solid);

    // reset the verticesEndPointer so we're writing to the beginning of the array
    verticesEndPointer = verticesArray;
    // call recursive function to populate in memory arrays
    // it will return the number of voxels added
    voxelsRendered = treeToArrays(tree->rootNode);
    // set the boolean if there are any voxels to be rendered so we re-fill the VBOs
    voxelsToRender = (voxelsRendered > 0);
}


void VoxelSystem::parseData(void *data, int size) {
    // output the bits received from the voxel server
    unsigned char *voxelData = (unsigned char *) data + 1;
    
    // ask the VoxelTree to read the bitstream into the tree
    tree->readBitstreamToTree(voxelData, size - 1);
    
    // reset the verticesEndPointer so we're writing to the beginning of the array
    verticesEndPointer = verticesArray;
    
    // call recursive function to populate in memory arrays
    // it will return the number of voxels added
    int newVoxels = treeToArrays(tree->rootNode);
    
    if (newVoxels > voxelsRendered) {
        
        voxelsRendered = newVoxels;
        
        // set the boolean if there are any voxels to be rendered so we re-fill the VBOs
        voxelsToRender = true;
    }
}

int VoxelSystem::treeToArrays(VoxelNode *currentNode) {
    int voxelsAdded = 0;

    for (int i = 0; i < 8; i++) {
        // check if there is a child here
        if (currentNode->children[i] != NULL) {
            voxelsAdded += treeToArrays(currentNode->children[i]);
        }
    }
    
    // if we didn't get any voxels added then we're a leaf
    // add our vertex and color information to the interleaved array
    if (voxelsAdded == 0 && currentNode->color[3] == 1) {
        float * startVertex = firstVertexForCode(currentNode->octalCode);
        float voxelScale = 1 / powf(2, *currentNode->octalCode);
        
        // populate the array with points for the 8 vertices
        // and RGB color for each added vertex
        for (int j = 0; j < VERTEX_POINTS_PER_VOXEL; j++ ) {
            *verticesEndPointer = startVertex[j % 3] + (identityVertices[j] * voxelScale);
            *(colorsArray + (verticesEndPointer - verticesArray)) = currentNode->color[j % 3];
            
            verticesEndPointer++;
        }
        
        voxelsAdded++;
       
        delete [] startVertex;
    }

    return voxelsAdded;
}

VoxelSystem* VoxelSystem::clone() const {
    // this still needs to be implemented, will need to be used if VoxelSystem is attached to agent
    return NULL;
}

void VoxelSystem::init() {
    // prep the data structures for incoming voxel data
    verticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    colorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    
    GLuint *indicesArray = new GLuint[INDICES_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    
    // populate the indicesArray
    // this will not change given new voxels, so we can set it all up now
    for (int n = 0; n < MAX_VOXELS_PER_SYSTEM; n++) {
        // fill the indices array
        int voxelIndexOffset = n * INDICES_PER_VOXEL;
        GLuint *currentIndicesPos = indicesArray + voxelIndexOffset;
        int startIndex = (n * VERTICES_PER_VOXEL);
        
        for (int i = 0; i < INDICES_PER_VOXEL; i++) {
            // add indices for this side of the cube
            currentIndicesPos[i] = startIndex + identityIndices[i];
        }
    }
    
    // VBO for the verticesArray
    glGenBuffers(1, &vboVerticesID);
    glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
    
    // VBO for colorsArray
    glGenBuffers(1, &vboColorsID);
    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
    
    // VBO for the indicesArray
    glGenBuffers(1, &vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 INDICES_PER_VOXEL * sizeof(GLuint) * MAX_VOXELS_PER_SYSTEM,
                 indicesArray, GL_STATIC_DRAW);
    
    // delete the indices array that is no longer needed
    delete[] indicesArray;
}

void VoxelSystem::render() {

    glPushMatrix();
    
    if (voxelsToRender) {
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (verticesEndPointer - verticesArray) * sizeof(GLfloat), verticesArray);
        
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (verticesEndPointer - verticesArray) * sizeof(GLubyte), colorsArray);
        
        voxelsToRender = false;
    }

    // tell OpenGL where to find vertex and color information
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);
   
    // draw the number of voxels we have
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glTranslatef(40, 0, 40);
    glScalef(10, 10, 10);
    glDrawElements(GL_TRIANGLES, 36 * voxelsRendered, GL_UNSIGNED_INT, 0);
    
    // deactivate vertex and color arrays after drawing
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    // scale back down to 1 so heads aren't massive
    glPopMatrix();
}

void VoxelSystem::simulate(float deltaTime) {
    
}


