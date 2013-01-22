//
//  cube.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 Rosedale Lab. All rights reserved.
//

#ifndef interface_cube_h
#define interface_cube_h

#include "glm/glm.hpp"
#include "util.h"
#include "world.h"
#include <GLUT/glut.h>
#include <iostream>

class VoxelSystem {
public:
    VoxelSystem(int num,
                   glm::vec3 box);
    void simulate(float deltaTime);
    void render();
private:
    struct Voxel {
        glm::vec3 color;
        bool hasChildren;
        Voxel * children;
    } *voxels;    
};


#endif
