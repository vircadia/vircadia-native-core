//
//  PhysicsSimulation.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.06.06
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>

#include "PhysicsSimulation.h"

#include "PhysicsEntity.h"
#include "Ragdoll.h"
#include "SharedUtil.h"
#include "ShapeCollider.h"

int MAX_DOLLS_PER_SIMULATION = 16;
int MAX_ENTITIES_PER_SIMULATION = 64;
int MAX_COLLISIONS_PER_SIMULATION = 256;


const int NUM_SHAPE_BITS = 6;
const int SHAPE_INDEX_MASK = (1 << (NUM_SHAPE_BITS + 1)) - 1;

PhysicsSimulation::PhysicsSimulation() : _collisionList(MAX_COLLISIONS_PER_SIMULATION) {
}

PhysicsSimulation::~PhysicsSimulation() {
    _dolls.clear();
}

bool PhysicsSimulation::addEntity(PhysicsEntity* entity) {
    if (!entity) {
        return false;
    }
    if (entity->_simulation == this) {
        int numEntities = _entities.size();
        for (int i = 0; i < numEntities; ++i) {
            if (entity == _entities.at(i)) {
                // already in list
                assert(entity->_simulation == this);
                return true;
            }
        }
        // belongs to some other simulation
        return false;
    }
    int numEntities = _entities.size();
    if (numEntities > MAX_ENTITIES_PER_SIMULATION) {
        // list is full
        return false;
    }
    // add to list
    entity->_simulation = this;
    _entities.push_back(entity);
    return true;
}

void PhysicsSimulation::removeEntity(PhysicsEntity* entity) {
    if (!entity || !entity->_simulation || !(entity->_simulation == this)) {
        return;
    }
    int numEntities = _entities.size();
    for (int i = 0; i < numEntities; ++i) {
        if (entity == _entities.at(i)) {
            if (i == numEntities - 1) {
                // remove it
                _entities.pop_back();
            } else {
                // swap the last for this one
                PhysicsEntity* lastEntity = _entities[numEntities - 1];
                _entities.pop_back();
                _entities[i] = lastEntity;
            }
            entity->_simulation = NULL;
            break;
        }
    }
}

bool PhysicsSimulation::addRagdoll(Ragdoll* doll) {
    if (!doll) {
        return false;
    }
    int numDolls = _dolls.size();
    if (numDolls > MAX_DOLLS_PER_SIMULATION) {
        // list is full
        return false;
    }
    for (int i = 0; i < numDolls; ++i) {
        if (doll == _dolls[i]) {
            // already in list
            return true;
        }
    }
    // add to list
    _dolls.push_back(doll);
    return true;
}

void PhysicsSimulation::removeRagdoll(Ragdoll* doll) {
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        if (doll == _dolls[i]) {
            if (i == numDolls - 1) {
                // remove it
                _dolls.pop_back();
            } else {
                // swap the last for this one
                Ragdoll* lastDoll = _dolls[numDolls - 1];
                _dolls.pop_back();
                _dolls[i] = lastDoll;
            }
            break;
        }
    }
}

void PhysicsSimulation::stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec) {
    /* TODO: Andrew to make this work
    int iterations = 0;
    float delta = 0.0f;
    quint64 now = usecTimestampNow();
    quint64 startTime = now;
    quint64 expiry = now + maxUsec;

    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        // TODO: Andrew need to implement:
        // (1) joints pull points (SpecialCapsuleShape would help solve this)
        // (2) points slam shapes (SpecialCapsuleShape would help solve this)
        // (3) shapes collide with pairwise collision bypass
        // (4) collisions move points (SpecialCapsuleShape would help solve this)
        // (5) enforce constraints
        // (6) add and enforce angular contraints for joints
        //_dolls.at(i)->stepForward(deltaTime);
    }

    
    // collide
    _collisionList.clear();
    // TODO: keep track of QSet<PhysicsEntity*> collidedEntities;
    for (int i = 0; i < numDolls; ++i) {
        const QVector<Shape*>* shapesA = _dolls.at(i)->getShapes();
        if (!shapesA) {
            continue;
        }
        int numShapesA = shapesA->size();
        // collide with self
        for (int j = 0; j < numShapesA; ++j) {
            const Shape* shapeA = shapesA->at(j);
            if (!shapeA) {
                continue;
            }
            // TODO: check for pairwise collision bypass here
            ShapeCollider::collideShapeWithShapes(shapeA, *shapesA, j+1, _collisionList);
        }

        // collide with others
        for (int j = i+1; j < numDolls; ++j) {
            const QVector<Shape*>* shapesB = _dolls.at(j)->getShapes();
            if (!shapesB) {
                continue;
            }
            ShapeCollider::collideShapesWithShapes(*shapesA, *shapesB, _collisionList);
        }
    }

    // enforce constraints
    float error = 0.0f;
    do {
        error = 0.0f;
        for (int i = 0; i < numDolls; ++i) {
            error = glm::max(error, _dolls[i]->enforceConstraints());
        }
        ++iterations;
        now = usecTimestampNow();
    } while (iterations < maxIterations && delta > minError && now < expiry);
    _enforcementIterations = iterations;
    _enforcementError = delta;
    _enforcementTime = now - startTime;
    */
}

int PhysicsSimulation::computeCollisions() {
    return 0.0f;
}

void PhysicsSimulation::processCollisions() {
}

