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
        } _field[FIELD_ELEMENTS];
            
        Field(float worldSize, float coupling);

        int value(float *ret, float *pos);
        void render();
        void add(float* add, float *loc);
        void interact(float deltaTime, const glm::vec3& pos, glm::vec3& vel);
        void simulate(float deltaTime);
    private:
        float _worldSize;
        float _coupling;
};

#endif
