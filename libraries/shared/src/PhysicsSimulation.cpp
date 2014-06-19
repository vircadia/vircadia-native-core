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

PhysicsSimulation::PhysicsSimulation() : _collisionList(MAX_COLLISIONS_PER_SIMULATION), 
        _numIterations(0), _numCollisions(0), _constraintError(0.0f), _stepTime(0) {
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

// TODO: Andrew need to implement:
// DONE (1) joints pull points (SpecialCapsuleShape would help solve this)
// DONE (2) points slam shapes (SpecialCapsuleShape would help solve this)
// DONE (3) detect collisions
// DONE (4) collisions move points (SpecialCapsuleShape would help solve this)
// DONE (5) enforce constraints
// (6) make sure MyAvatar creates shapes, adds to simulation with ragdoll support
// (7) support for pairwise collision bypass
// (8) process collisions
// (9) add and enforce angular contraints for joints
void PhysicsSimulation::stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec) {
    int iterations = 0;
    quint64 now = usecTimestampNow();
    quint64 startTime = now;
    quint64 expiry = now + maxUsec;

    // move dolls
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        _dolls.at(i)->stepRagdollForward(deltaTime);
    }
    
    // collide
    _collisionList.clear();
    // TODO: keep track of QSet<PhysicsEntity*> collidedEntities;
    int numEntities = _entities.size();
    for (int i = 0; i < numEntities; ++i) {
        const QVector<Shape*> shapes = _entities.at(i)->getShapes();
        int numShapes = shapes.size();
        // collide with self
        for (int j = 0; j < numShapes; ++j) {
            const Shape* shape = shapes.at(j);
            if (!shape) {
                continue;
            }
            // TODO: check for pairwise collision bypass here
            for (int k = j+1; k < numShapes; ++k) {
                const Shape* otherShape = shapes.at(k);
                ShapeCollider::collideShapes(shape, otherShape, _collisionList);
            }
        }

        // collide with others
        for (int j = i+1; j < numEntities; ++j) {
            const QVector<Shape*> otherShapes = _entities.at(j)->getShapes();
            ShapeCollider::collideShapesWithShapes(shapes, otherShapes, _collisionList);
        }
    }

    // TODO: process collisions
    _numCollisions = _collisionList.size();

    // enforce constraints
    float error = 0.0f;
    do {
        error = 0.0f;
        for (int i = 0; i < numDolls; ++i) {
            error = glm::max(error, _dolls[i]->enforceRagdollConstraints());
        }
        ++iterations;
        now = usecTimestampNow();
    } while (iterations < maxIterations && error > minError && now < expiry);

    _numIterations = iterations;
    _constraintError = error;
    _stepTime = now - startTime;
}

int PhysicsSimulation::computeCollisions() {
    return 0.0f;
}

void PhysicsSimulation::processCollisions() {
}

