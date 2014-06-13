//
//  SimulationEngine.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.06.06
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>

#include "SimulationEngine.h"

int MAX_DOLLS_PER_ENGINE = 32;
int MAX_COLLISIONS_PER_ENGINE = 256;

const int NUM_SHAPE_BITS = 6;
const int SHAPE_INDEX_MASK = (1 << (NUM_SHAPE_BITS + 1)) - 1;

SimulationEngine::SimulationEngine() : _collisionList(MAX_COLLISIONS_PER_ENGINE) {
}

SimulationEngine::~SimulationEngine() {
    _dolls.clear();
}

bool SimulationEngine::addRagDoll(RagDoll* doll) {
    int numDolls = _dolls.size();
    if (numDolls < MAX_DOLLS_PER_ENGINE) {
        for (int i = 0; i < numDolls; ++i) {
            if (doll == _dolls[i]) {
                // already in list
                return true;
            }
        }
        _dolls.push_back(doll);
        return true;
    }
    return false;
}

void SimulationEngine::removeRagDoll(RagDoll* doll) {
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        if (doll == _dolls[i]) {
            if (i == numDolls - 1) {
                // remove it
                _dolls.pop_back();
            } else {
                // swap the last for this one
                RagDoll* lastDoll = _dolls[numDolls - 1];
                _dolls.pop_back();
                _dolls[i] = lastDoll;
            }
            break;
        }
    }
}

float SimulationEngine::enforceConstraints() {
    float maxMovement = 0.0f;
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        maxMovement = glm::max(maxMovement, _dolls[i]->enforceConstraints());
    }
    return maxMovement;
}

int SimulationEngine::computeCollisions() {
    return 0.0f;
}

void SimulationEngine::processCollisions() {
}

