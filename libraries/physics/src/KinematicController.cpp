//
//  KinematicController.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KinematicController.h"
#include "PhysicsEngine.h"

KinematicController::KinematicController() { 
    _lastSubstep = PhysicsEngine::getNumSubsteps(); 
}

void KinematicController::start() { 
    _enabled = true; 
    _lastSubstep = PhysicsEngine::getNumSubsteps(); 
}
