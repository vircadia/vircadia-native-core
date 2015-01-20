//
//  SimpleEntityKinematicController.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsEngine.h"
#include "SimpleEntityKinematicController.h"

void SimpleEntityKinematicController:: stepForward() {
    uint32_t substep = PhysicsEngine::getNumSubsteps();
    float dt = (substep - _lastSubstep) * PHYSICS_ENGINE_FIXED_SUBSTEP;
    _entity->simulateSimpleKinematicMotion(dt);
    _lastSubstep = substep;
}

