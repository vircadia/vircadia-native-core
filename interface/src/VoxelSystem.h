//
//  Cube.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Cube__
#define __interface__Cube__

#include "InterfaceConfig.h"
#include <glm/glm.hpp>
#include <iostream>
#include <UDPSocket.h>
#include <AgentData.h>
#include "Util.h"
#include "world.h"

const int NUM_CHILDREN = 8;

class VoxelSystem : public AgentData {
public:
    VoxelSystem();
    ~VoxelSystem();
    
    void parseData(void *data, int size);
    VoxelSystem* clone() const;
    
    void init();
    void simulate(float deltaTime);
    void render();
    void setVoxelsRendered(int v) {voxelsRendered = v;};
    int getVoxelsRendered() {return voxelsRendered;};
private:
    int voxelsRendered;
    GLfloat *verticesArray;
    GLubyte *colorsArray;
    GLfloat *lastAddPointer;
    GLfloat *lastDrawPointer;
    GLuint vboVerticesID;
    GLuint vboColorsID;
    GLuint vboIndicesID;
};

#endif
