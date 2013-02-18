//
//  Cube.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Cube__
#define __interface__Cube__

#include <glm/glm.hpp>
#include "Util.h"
#include "world.h"
#include "InterfaceConfig.h"
#include <iostream>

const int NUM_CHILDREN = 8;

struct Voxel {
    glm::vec3 color;
    Voxel * children[NUM_CHILDREN];
};

class VoxelSystem {
public:
    void simulate(float deltaTime);
    void render(Voxel * voxel, float scale);
    void init();
    int initVoxels(Voxel * root, float scale);
    Voxel * root;
};

#endif
