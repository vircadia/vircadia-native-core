//
//  Hand.cpp
//  interface
//
//  Created by Philip Rosedale on 10/13/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Hand.h"

const float PHI = 1.618;

const float DEFAULT_X = 0;
const float DEFAULT_Y = -1.5;
const float DEFAULT_Z = 2.0;

Hand::Hand(glm::vec3 initcolor)
{
    color = initcolor;
    reset();
    noise = 0.2;
    scale.x = 0.07;
    scale.y = scale.x * 5.0;
    scale.z = scale.y * 1.0;
}

void Hand::render()
{
g}

void Hand::reset()
{
    position.x = DEFAULT_X;
    position.y = DEFAULT_Y;
    position.z = DEFAULT_Z;
    setTarget(position);
    velocity.x = velocity.y = velocity.z = 0;
}

void Hand::simulate(float deltaTime)
{
    //  If noise, add wandering movement
    if (noise) {
        position += noise * 0.1f * glm::vec3(randFloat() - 0.5, randFloat() - 0.5, randFloat() - 0.5);
    }
    //  Decay position of hand toward target
    position -= deltaTime*(position - target);
    
}