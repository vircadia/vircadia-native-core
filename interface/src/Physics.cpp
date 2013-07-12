//
//  Physics.cpp
//  hifi
//
//  Created by Philip on July 11, 2013
//
//  Routines to help with doing virtual world physics
//

#include <glm/glm.hpp>
#include <SharedUtil.h>

#include "Util.h"
#include "world.h"
#include "Physics.h"

//
//  Applies static friction:  maxVelocity is the largest velocity for which there
//  there is friction, and strength is the amount of friction force applied to reduce
//  velocity. 
// 
void applyStaticFriction(float deltaTime, glm::vec3& velocity, float maxVelocity, float strength) {
    float v = glm::length(velocity);
    if (v < maxVelocity) {
        velocity *= glm::clamp((1.0f - deltaTime * strength * (1.f - v / maxVelocity)), 0.0f, 1.0f);
    }
}

//
//  Applies velocity damping, with a strength value for linear and squared velocity damping
//

void applyDamping(float deltaTime, glm::vec3& velocity, float linearStrength, float squaredStrength) {
    if (squaredStrength == 0.f) {
        velocity *= glm::clamp(1.f - deltaTime * linearStrength, 0.f, 1.f);
    } else {
        velocity *= glm::clamp(1.f - deltaTime * (linearStrength + glm::length(velocity) * squaredStrength), 0.f, 1.f);
    }
}

