//
//  field.h
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_field_h
#define interface_field_h

#include <glm/glm.hpp>

//  Field is a lattice of vectors uniformly distributed FIELD_ELEMENTS^(1/3) on side 

const int FIELD_ELEMENTS = 1000;

void field_init();
int field_value(float *ret, float *pos);
void field_render();
void field_add(float* add, float *loc);

class Field {
public:
    static void init ();
    static int addTo (const glm::vec3 &pos, glm::vec3 &v);

private:
    const static unsigned int fieldSize = 1000;
    const static float fieldScale; // defined in cpp – inline const float definitions not allowed in standard C++?! (allowed in C++0x)
    static glm::vec3 field[fieldSize];
};

#endif
