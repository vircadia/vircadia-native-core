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
    pthread_mutex_init(&bufferWriteLock, NULL);
}

VoxelSystem::~VoxelSystem() {    
    delete[] readVerticesArray;
    delete[] writeVerticesArray;
    delete[] readColorsArray;
    delete[] writeColorsArray;
    delete tree;
    pthread_mutex_destroy(&bufferWriteLock);
}

void VoxelSystem::setViewerHead(Head *newViewerHead) {
    viewerHead = newViewerHead;
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
    
    copyWrittenDataToReadArrays();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Method:      VoxelSystem::createSphere()
// Description: Creates a sphere of voxels in the local system at a given location/radius
// To Do:       Move this function someplace better? I put it here because we need a
//              mechanism to tell the system to redraw it's arrays after voxels are done
//              being added. This is a concept mostly only understood by VoxelSystem.
// Complaints:  Brad :)
void VoxelSystem::createSphere(float r,float xc, float yc, float zc, float s, bool solid, bool wantColorRandomizer) {

    tree->createSphere(r,xc,yc,zc,s,solid,wantColorRandomizer);
    setupNewVoxelsForDrawing();
}


void VoxelSystem::parseData(void *data, int size) {
    // output the bits received from the voxel server
    unsigned char *voxelData = (unsigned char *) data + 1;
    
    // ask the VoxelTree to read the bitstream into the tree
    tree->readBitstreamToTree(voxelData, size - 1);
    
    setupNewVoxelsForDrawing();
}

void VoxelSystem::setupNewVoxelsForDrawing() {
    // reset the verticesEndPointer so we're writing to the beginning of the array
    writeVerticesEndPointer = writeVerticesArray;
    // call recursive function to populate in memory arrays
    // it will return the number of voxels added
    float treeRoot[3] = {0,0,0};
    voxelsRendered = treeToArrays(tree->rootNode, treeRoot);
    
    // copy the newly written data to the arrays designated for reading
    copyWrittenDataToReadArrays();
}

void VoxelSystem::copyWrittenDataToReadArrays() {
    // lock on the buffer write lock so we can't modify the data when the GPU is reading it
    pthread_mutex_lock(&bufferWriteLock);
    // store a pointer to the current end so it doesn't change during copy
    GLfloat *endOfCurrentVerticesData = writeVerticesEndPointer;
    // copy the vertices and colors
    memcpy(readVerticesArray, writeVerticesArray, (endOfCurrentVerticesData - writeVerticesArray) * sizeof(GLfloat));
    memcpy(readColorsArray, writeColorsArray, (endOfCurrentVerticesData - writeVerticesArray) * sizeof(GLubyte));
    
    // set the read vertices end pointer to the correct spot so the GPU knows how much to pull
    readVerticesEndPointer = readVerticesArray + (endOfCurrentVerticesData - writeVerticesArray);
    pthread_mutex_unlock(&bufferWriteLock);
}

int VoxelSystem::treeToArrays(VoxelNode *currentNode, float nodePosition[3]) {
    int voxelsAdded = 0;

    float halfUnitForVoxel = powf(0.5, *currentNode->octalCode) * (0.5 * TREE_SCALE);
    glm::vec3 viewerPosition = viewerHead->getPos();
    
    float distanceToVoxelCenter = sqrtf(powf(viewerPosition[0] - nodePosition[0] - halfUnitForVoxel, 2) +
                                        powf(viewerPosition[1] - nodePosition[1] - halfUnitForVoxel, 2) +
                                        powf(viewerPosition[2] - nodePosition[2] - halfUnitForVoxel, 2));
    
    if (distanceToVoxelCenter < boundaryDistanceForRenderLevel(*currentNode->octalCode + 1)) {
        for (int i = 0; i < 8; i++) {
            // check if there is a child here
            if (currentNode->children[i] != NULL) {
                
                // calculate the child's position based on the parent position
                float childNodePosition[3];
                
                for (int j = 0; j < 3; j++) {
                    childNodePosition[j] = nodePosition[j];
                    
                    if (oneAtBit(branchIndexWithDescendant(currentNode->octalCode,
                                                           currentNode->children[i]->octalCode),
                                 (7 - j))) {
                        childNodePosition[j] -= (powf(0.5, *currentNode->children[i]->octalCode) * TREE_SCALE);
                    }
                }
                
                
                voxelsAdded += treeToArrays(currentNode->children[i], childNodePosition);
            }
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
            *writeVerticesEndPointer = startVertex[j % 3] + (identityVertices[j] * voxelScale);
            *(writeColorsArray + (writeVerticesEndPointer - writeVerticesArray)) = currentNode->color[j % 3];
            
            writeVerticesEndPointer++;
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
    writeVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    readVerticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    writeColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    readColorsArray = new GLubyte[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    
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
    
    
    if (readVerticesEndPointer != readVerticesArray) {
        // try to lock on the buffer write
        // just avoid pulling new data if it is currently being written
        if (pthread_mutex_trylock(&bufferWriteLock) == 0) {
            
            glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
            glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, (readVerticesEndPointer - readVerticesArray) * sizeof(GLfloat), readVerticesArray);
            
            glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
            glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, (readVerticesEndPointer - readVerticesArray) * sizeof(GLubyte), readColorsArray);
            
            readVerticesEndPointer = readVerticesArray;
            
            pthread_mutex_unlock(&bufferWriteLock);
        }
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


