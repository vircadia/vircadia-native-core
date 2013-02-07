//
//  Field.h
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Field__
#define __interface__Field__

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include "world.h"
#include "Util.h"
#include <glm.hpp>

//  Field is a lattice of vectors uniformly distributed FIELD_ELEMENTS^(1/3) on side 
const int FIELD_ELEMENTS = 1000;

struct {
    glm::vec3 val;
    glm::vec3 center;
    glm::vec3 fld;
    float scalar;
} field[FIELD_ELEMENTS];

// Pre-calculated RGB values for each field element
struct {
    glm::vec3 rgb;
} fieldcolors[FIELD_ELEMENTS];

void field_init();
int field_value(float *ret, float *pos);
void field_render();
void field_add(float* add, float *loc);
void field_interact(float dt, glm::vec3 * pos, glm::vec3 * vel, glm::vec3 * color, float coupling);
void field_simulate(float dt);
glm::vec3 hsv2rgb(glm::vec3 in);
#endif
