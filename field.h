//
//  field.h
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_field_h
#define interface_field_h

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include "world.h"
#include "util.h"
#include "glm/glm.hpp"

//  Field is a lattice of vectors uniformly distributed FIELD_ELEMENTS^(1/3) on side 
const int FIELD_ELEMENTS = 1000;

struct {
    glm::vec3 val;
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
