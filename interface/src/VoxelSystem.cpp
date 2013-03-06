//
//  Cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "VoxelSystem.h"

const float MAX_UNIT_ANY_AXIS = 20.0f;
const float CUBE_WIDTH = 0.01f;
const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int COLOR_VALUES_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int INDICES_PER_VOXEL = 3 * 12;

GLfloat identityVertices[] = { -1, -1, 1,
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


bool onSphereShell(float radius, float scale, glm::vec3 * position) {
    float vRadius = glm::length(*position);
    return ((vRadius + scale/2.0 > radius) && (vRadius - scale/2.0 < radius));
}

void VoxelSystem::init() {
    root = new Voxel;
}

float randomFloat(float maximumValue) {
    return ((float) rand() / ((float) RAND_MAX / maximumValue));
}

void VoxelSystem::init(int numberOfRandomVoxels) {
    // create the arrays needed to pass to glDrawElements later
    // position / color are random for now
    
    voxelsRendered = numberOfRandomVoxels;
    
    // there are 3 points for each vertices, 24 vertices in each cube
    GLfloat *verticesArray = new GLfloat[VERTEX_POINTS_PER_VOXEL * numberOfRandomVoxels];
    
    // we need a color for each vertex in each voxel
    GLfloat *colorsArray = new GLfloat[COLOR_VALUES_PER_VOXEL * numberOfRandomVoxels];
    
    // there are 12 triangles in each cube, with three indices for each triangle
    GLuint *indicesArray = new GLuint[INDICES_PER_VOXEL * numberOfRandomVoxels];
    
    // new seed based on time now so voxels are different each time
    srand((unsigned)time(0));
    
    for (int n = 0; n < numberOfRandomVoxels; n++) {        
        // pick a random point for the center of the cube
        glm::vec3 position = glm::vec3(
            randomFloat(MAX_UNIT_ANY_AXIS * 2) - MAX_UNIT_ANY_AXIS,
            randomFloat(MAX_UNIT_ANY_AXIS * 2) - MAX_UNIT_ANY_AXIS,
            randomFloat(MAX_UNIT_ANY_AXIS * 2) - MAX_UNIT_ANY_AXIS
        );
        
        // fill the vertices array
        GLfloat *currentVerticesPos = verticesArray + (n * VERTEX_POINTS_PER_VOXEL);
        
        for (int v = 0; v < VERTEX_POINTS_PER_VOXEL; v++) {
            currentVerticesPos[v] = position[v % 3] + (identityVertices[v] * CUBE_WIDTH);
        }
        
        // fill the colors array
        GLfloat *currentColorPos = colorsArray + (n * COLOR_VALUES_PER_VOXEL);
        float voxelR = randomFloat(1);
        float voxelG = randomFloat(1);
        float voxelB = randomFloat(1);
        
        for (int c = 0; c < VERTICES_PER_VOXEL; c++) {
            currentColorPos[0 + (c * 3)] = voxelR;
            currentColorPos[1 + (c * 3)] = voxelG;
            currentColorPos[2 + (c * 3)] = voxelB;
        }

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
    glBufferData(GL_ARRAY_BUFFER, VERTEX_POINTS_PER_VOXEL * sizeof(GLfloat) * numberOfRandomVoxels, verticesArray, GL_STATIC_DRAW);
    
    // VBO for colorsArray
    glGenBuffers(1, &vboColorsID);
    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glBufferData(GL_ARRAY_BUFFER, VERTICES_PER_VOXEL * sizeof(GLfloat) * numberOfRandomVoxels, colorsArray, GL_STATIC_DRAW);
    
    // VBO for the indicesArray
    glGenBuffers(1, &vboIndicesID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numberOfRandomVoxels * INDICES_PER_VOXEL * sizeof(GLuint), indicesArray, GL_STATIC_DRAW);

    // delete the verticesArray, indicesArray
    delete[] verticesArray;
    delete[] indicesArray;
    delete[] colorsArray;
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

void VoxelSystem::render() {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndicesID);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    glColorPointer(3, GL_FLOAT, 0, 0);
    
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

