//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "VoxelSystem.h"
#include <AgentList.h>

const int MAX_VOXELS_PER_SYSTEM = 500000;

const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int COLOR_VALUES_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int INDICES_PER_VOXEL = 3 * 12;

const float CUBE_WIDTH = 0.025f;

float identityVertices[] = { -1, -1, 1,
                            1, -1, 1,
                            1, -1, -1,
                            -1, -1, -1,
                            1, 1, 1,
                            -1, 1, 1,
                            -1, 1, -1,
                            1, 1, -1 };

GLubyte identityIndices[] = { 0,1,2, 0,2,3,
                              0,4,1, 0,4,5,
                              0,3,6, 0,5,6,
                              1,2,4, 2,4,7,
                              2,3,6, 2,6,7,
                              4,5,6, 4,6,7 };

VoxelSystem::VoxelSystem() {
    voxelsRendered = 0;
}

VoxelSystem::~VoxelSystem() {    
    delete[] verticesArray;
    delete[] colorsArray;
}

void VoxelSystem::parseData(void *data, int size) {
    // ignore the first char, it's a V to tell us that this is voxel data
    char *voxelDataPtr = (char *) data + 1;

    float *position = new float[3];
    char *color = new char[3];

    // get pointers to position of last append of data
    GLfloat *parseVerticesPtr = lastAddPointer;
    GLubyte *parseColorsPtr = colorsArray + (lastAddPointer - verticesArray);
    
    int voxelsInData = 0;    
    
    // pull voxels out of the received data and put them into our internal memory structure
    while ((voxelDataPtr - (char *) data) < size) {
        
        memcpy(position, voxelDataPtr, 3 * sizeof(float));
        voxelDataPtr += 3 * sizeof(float);
        memcpy(color, voxelDataPtr, 3);
        voxelDataPtr += 3;
        
        for (int v = 0; v < VERTEX_POINTS_PER_VOXEL; v++) {
            parseVerticesPtr[v] = position[v % 3] + (identityVertices[v] * CUBE_WIDTH);
        }
        
        parseVerticesPtr += VERTEX_POINTS_PER_VOXEL;
        
        for (int c = 0; c < COLOR_VALUES_PER_VOXEL; c++) {
            parseColorsPtr[c] = color[c % 3];
        }
        
        parseColorsPtr += COLOR_VALUES_PER_VOXEL;
        
        voxelsInData++;
    }
    
    // increase the lastAddPointer to the new spot, increase the number of rendered voxels
    lastAddPointer = parseVerticesPtr;
    voxelsRendered += voxelsInData;
}

VoxelSystem* VoxelSystem::clone() const {
    // this still needs to be implemented, will need to be used if VoxelSystem is attached to agent
    return NULL;
}

void VoxelSystem::init() {
    // prep the data structures for incoming voxel data
    lastDrawPointer = lastAddPointer = verticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    colorsArray = new GLubyte[COLOR_VALUES_PER_VOXEL * MAX_VOXELS_PER_SYSTEM];
    
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
    glBufferData(GL_ARRAY_BUFFER, COLOR_VALUES_PER_VOXEL * sizeof(GLubyte) * MAX_VOXELS_PER_SYSTEM, NULL, GL_DYNAMIC_DRAW);
    
    // VBO for the indicesArray
    glGenBuffers(1, &vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDICES_PER_VOXEL * sizeof(GLuint) * MAX_VOXELS_PER_SYSTEM, indicesArray, GL_STATIC_DRAW);
    
    // delete the indices array that is no longer needed
    delete[] indicesArray;
}

void VoxelSystem::render() {
    // check if there are new voxels to draw
    int vertexValuesToDraw = lastAddPointer - lastDrawPointer;
    
    if (vertexValuesToDraw > 0) {
        // calculate the offset into each VBO, in vertex point values
        int vertexBufferOffset = lastDrawPointer - verticesArray;
        
        // bind the vertices VBO, copy in new data
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset * sizeof(float), vertexValuesToDraw * sizeof(float), lastDrawPointer);
        
        // bind the colors VBO, copy in new data
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset * sizeof(GLubyte), vertexValuesToDraw * sizeof(GLubyte), (colorsArray + (lastDrawPointer - verticesArray)));
        
        // increment the lastDrawPointer to the lastAddPointer value used for this draw
        lastDrawPointer += vertexValuesToDraw;
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
    glDrawElements(GL_TRIANGLES, 36 * voxelsRendered, GL_UNSIGNED_INT, 0);
    
    // deactivate vertex and color arrays after drawing
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VoxelSystem::simulate(float deltaTime) {
    
}

