//
//  Physics.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void applyDamping(float deltaTime, glm::vec3& velocity, float linearStrength, float squaredStrength) {
    if (squaredStrength == 0.f) {
        velocity *= glm::clamp(1.f - deltaTime * linearStrength, 0.f, 1.f);
    } else {
        velocity *= glm::clamp(1.f - deltaTime * (linearStrength + glm::length(velocity) * squaredStrength), 0.f, 1.f);
    }
}

void applyDampedSpring(float deltaTime, glm::vec3& velocity, glm::vec3& position, glm::vec3& targetPosition, float k, float damping) {
    
}

