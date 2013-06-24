//
//  GeometryCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cmath>

#include "GeometryCache.h"
#include "world.h"

GeometryCache::~GeometryCache() {
    foreach (const VerticesIndices& vbo, _hemisphereVBOs) {
        glDeleteBuffers(1, &vbo.first);
        glDeleteBuffers(1, &vbo.second);
    }
}

void GeometryCache::renderHemisphere(int slices, int stacks) {
    VerticesIndices& vbo = _hemisphereVBOs[SlicesStacks(slices, stacks)];
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i < stacks - 1; i++) {
            float phi = PIf * 0.5f * i / (stacks - 1), z, radius;
            sincosf(phi, &z, &radius);
            
            for (int j = 0; j < slices; j++) {
                float theta = PIf * 2.0f * j / slices;
                float x, y;
                sincosf(theta, &x, &y);
                x *= radius;
                y *= radius;
                
                *(vertex++) = x;
                *(vertex++) = y;
                *(vertex++) = z;
            }
        }
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f;
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 2; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        GLushort bottom = (stacks - 2) * slices;
        GLushort top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + i;
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = top;
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
 
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormalPointer(GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
