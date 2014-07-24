//
//  Ragdoll.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Ragdoll.h"

#include "Constraint.h"
#include "DistanceConstraint.h"
#include "FixedConstraint.h"

Ragdoll::Ragdoll() {
}

Ragdoll::~Ragdoll() {
    clearRagdollConstraintsAndPoints();
}
    
void Ragdoll::clearRagdollConstraintsAndPoints() {
    int numConstraints = _ragdollConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        delete _ragdollConstraints[i];
    }
    _ragdollConstraints.clear();
    _ragdollPoints.clear();
}

float Ragdoll::enforceRagdollConstraints() {
    float maxDistance = 0.0f;
    const int numConstraints = _ragdollConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        DistanceConstraint* c = static_cast<DistanceConstraint*>(_ragdollConstraints[i]);
        //maxDistance = glm::max(maxDistance, _ragdollConstraints[i]->enforce());
        maxDistance = glm::max(maxDistance, c->enforce());
    }
    return maxDistance;
}
