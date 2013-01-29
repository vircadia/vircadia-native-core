//
//  cube.cpp
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "cube.h"

#define MAX_CUBES 250000
#define SMALLEST_CUBE 0.2

float cubes_position[MAX_CUBES*3];
float cubes_scale[MAX_CUBES];
float cubes_color[MAX_CUBES*3];
int cube_count = 0;

void makeCubes2D(float location[3], float scale, int * index,
               float * cubes_position, float * cubes_scale, float * cubes_color) {
    int i;
    float spot[3];
    float distance = powf(location[0]*location[0] + location[2]*location[2], 0.5);
    
    if (*index >= MAX_CUBES) return;
    if ((scale <= SMALLEST_CUBE) || (scale/distance < 0.025) || ((scale < 0.1) && (randFloat()<0.01))) {
        //  Make a cube
        for (i = 0; i < 3; i++) cubes_position[*index*3 + i] = location[i]+scale/2.0;
        //glm::vec2 noisepoint(location[0], location[2]);
        //float color = glm::noise(noisepoint);
        float color = 0.3 + randFloat()*0.7;
        cubes_scale[*index] = scale;
        cubes_color[*index*3] = color;
        cubes_color[*index*3 + 1] = color;
        cubes_color[*index*3 + 2] = color;
        *index += 1;
    } else {
        for (i = 0; i < 4; i++) {
            spot[0] = location[0] + (i%2)*scale/2.0;
            spot[2] = location[2] + ((i/2)%2)*scale/2.0;
            spot[1] = sinf(location[0])*0.15 + cosf(location[2]/0.2)*0.10 + randFloat()*0.005;
            makeCubes2D(spot, scale/2.0, index, cubes_position, cubes_scale, cubes_color);
        }
    }
}

VoxelSystem::VoxelSystem(int num,
                         glm::vec3 box) {
     
    float location[] = {0,0,0};
    float scale = 10.0;
    int j = 0;
    int index = 0;
    if (num > 0)
        makeCubes2D(location, scale, &index, cubes_position, cubes_scale, cubes_color);
    std::cout << "Run " << j << " Made " << index << " cubes\n";
    cube_count = index;
     
}

void VoxelSystem::render() {
    glPushMatrix();
    int i = 0;
    while (i < cube_count) {
        glPushMatrix();
        glTranslatef(cubes_position[i*3], cubes_position[i*3+1], cubes_position[i*3+2]);
        glColor3fv(&cubes_color[i*3]);
        glutSolidCube(cubes_scale[i]);
        glPopMatrix();
        i++;
    }
    glPopMatrix();
}

void VoxelSystem::simulate(float deltaTime) {
    
}





