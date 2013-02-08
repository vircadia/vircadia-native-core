//
//  Field.h
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Field__
#define __interface__Field__

#include <iostream>
#include <glm/glm.hpp>
#include "InterfaceConfig.h"
#include "world.h"
#include "Util.h"

//  Field is a lattice of vectors uniformly distributed FIELD_ELEMENTS^(1/3) on side 
const int FIELD_ELEMENTS = 1000;

class Field {
    public:
        struct FieldElement {
            glm::vec3 val;
            glm::vec3 center;
            glm::vec3 fld;
            float scalar;
        } field[FIELD_ELEMENTS];
        
        // Pre-calculated RGB values for each field element
        struct FieldColor {
            glm::vec3 rgb;
        } fieldcolors[FIELD_ELEMENTS];
    
        Field();
        int value(float *ret, float *pos);
        void render();
        void add(float* add, float *loc);
        void interact(float dt, glm::vec3 * pos, glm::vec3 * vel, glm::vec3 * color, float coupling);
        void simulate(float dt);
        glm::vec3 hsv2rgb(glm::vec3 in);
    private:
        void avg_neighbors(int index, glm::vec3 * result);
};

#endif
