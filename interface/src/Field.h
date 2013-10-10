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

const int FIELD_ELEMENTS = 1000;

///  Field is a lattice of vectors uniformly distributed in 3D with FIELD_ELEMENTS^(1/3) per side
class Field {
    public:
        struct FieldElement {
            glm::vec3 val;
            glm::vec3 center;
            glm::vec3 fld;
        } _field[FIELD_ELEMENTS];
            
        Field(float worldSize, float coupling);
        /// The field value at a position in space, given simply as the value of the enclosing cell
        int value(float *ret, float *pos);
        /// Visualize the field as vector lines drawn at each center
        void render();
        /// Add to the field value cell enclosing a location
        void add(float* add, float *loc);
        /// A particle with a position and velocity interacts with the field given the coupling
        /// constant passed when creating the field.
        void interact(float deltaTime, const glm::vec3& pos, glm::vec3& vel);
        /// Field evolves over timestep
        void simulate(float deltaTime);
    private:
        float _worldSize;
        float _coupling;
};

#endif
