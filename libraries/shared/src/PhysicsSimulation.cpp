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
#include <iostream>

#include "PhysicsSimulation.h"

#include "PerfStat.h"
#include "PhysicsEntity.h"
#include "Ragdoll.h"
#include "SharedUtil.h"
#include "ShapeCollider.h"

int MAX_DOLLS_PER_SIMULATION = 16;
int MAX_ENTITIES_PER_SIMULATION = 64;
int MAX_COLLISIONS_PER_SIMULATION = 256;


PhysicsSimulation::PhysicsSimulation() : _frame(0), _collisions(MAX_COLLISIONS_PER_SIMULATION) {
}

PhysicsSimulation::~PhysicsSimulation() {
    // entities have a backpointer to this simulator that must be cleaned up
    int numEntities = _entities.size();
    for (int i = 0; i < numEntities; ++i) {
        _entities[i]->_simulation = NULL;
    }
    _entities.clear();

    // but Ragdolls do not
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
    // remove corresponding contacts
    QMap<quint64, ContactConstraint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        if (entity == itr.value().getShapeA()->getEntity() || entity == itr.value().getShapeB()->getEntity()) {
            itr = _contacts.erase(itr);
        } else {
            ++itr;
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
    ++_frame;
    quint64 now = usecTimestampNow();
    quint64 startTime = now;
    quint64 expiry = startTime + maxUsec;

    moveRagdolls(deltaTime);
    computeCollisions();
    enforceContacts();
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        _dolls[i]->enforceRagdollConstraints();
    }

    int iterations = 0;
    float error = 0.0f;
    do {
        computeCollisions();
        updateContacts();
        resolveCollisions();

        { // enforce constraints
            PerformanceTimer perfTimer("4-enforce");
            error = 0.0f;
            for (int i = 0; i < numDolls; ++i) {
                error = glm::max(error, _dolls[i]->enforceRagdollConstraints());
            }
        }
        ++iterations;

        now = usecTimestampNow();
    } while (_collisions.size() != 0 && (iterations < maxIterations) && (error > minError) && (now < expiry));


#ifdef ANDREW_DEBUG
    quint64 stepTime = usecTimestampNow()- startTime;
    // temporary debug info for watching simulation performance
    if (0 == (_frame % 100)) {
        std::cout << "Ni = " << iterations << "  E = " << error  << "  t = " << stepTime << std::endl;
    }
#endif // ANDREW_DEBUG
    pruneContacts();
}

void PhysicsSimulation::moveRagdolls(float deltaTime) {
    PerformanceTimer perfTimer("1-integrate");
    int numDolls = _dolls.size();
    for (int i = 0; i < numDolls; ++i) {
        _dolls.at(i)->stepRagdollForward(deltaTime);
    }
}

void PhysicsSimulation::computeCollisions() {
    PerformanceTimer perfTimer("2-collide");
    _collisions.clear();
    // TODO: keep track of QSet<PhysicsEntity*> collidedEntities;
    int numEntities = _entities.size();
    for (int i = 0; i < numEntities; ++i) {
        PhysicsEntity* entity = _entities.at(i);
        const QVector<Shape*> shapes = entity->getShapes();
        int numShapes = shapes.size();
        // collide with self
        for (int j = 0; j < numShapes; ++j) {
            const Shape* shape = shapes.at(j);
            if (!shape) {
                continue;
            }
            for (int k = j+1; k < numShapes; ++k) {
                const Shape* otherShape = shapes.at(k);
                if (otherShape && entity->collisionsAreEnabled(j, k)) {
                    ShapeCollider::collideShapes(shape, otherShape, _collisions);
                }
            }
        }

        // collide with others
        for (int j = i+1; j < numEntities; ++j) {
            const QVector<Shape*> otherShapes = _entities.at(j)->getShapes();
            ShapeCollider::collideShapesWithShapes(shapes, otherShapes, _collisions);
        }
    }
}

void PhysicsSimulation::resolveCollisions() {
    PerformanceTimer perfTimer("3-resolve");
    // walk all collisions, accumulate movement on shapes, and build a list of affected shapes
    QSet<Shape*> shapes;
    int numCollisions = _collisions.size();
    for (int i = 0; i < numCollisions; ++i) {
        CollisionInfo* collision = _collisions.getCollision(i);
        collision->apply();
        // there is always a shapeA
        shapes.insert(collision->getShapeA());
        // but need to check for valid shapeB
        if (collision->_shapeB) {
            shapes.insert(collision->getShapeB());
        }
    }
    // walk all affected shapes and apply accumulated movement
    QSet<Shape*>::const_iterator shapeItr = shapes.constBegin();
    while (shapeItr != shapes.constEnd()) {
        (*shapeItr)->applyAccumulatedDelta();
        ++shapeItr;
    }
}

void PhysicsSimulation::enforceContacts() {
    QSet<Shape*> shapes;
    int numCollisions = _collisions.size();
    for (int i = 0; i < numCollisions; ++i) {
        CollisionInfo* collision = _collisions.getCollision(i);
        quint64 key = collision->getShapePairKey();
        if (key == 0) {
            continue;
        }
        QMap<quint64, ContactConstraint>::iterator itr = _contacts.find(key);
        if (itr != _contacts.end()) {
            if (itr.value().enforce() > 0.0f) {
                shapes.insert(collision->getShapeA());
                shapes.insert(collision->getShapeB());
            }
        }
    }
    // walk all affected shapes and apply accumulated movement
    QSet<Shape*>::const_iterator shapeItr = shapes.constBegin();
    while (shapeItr != shapes.constEnd()) {
        (*shapeItr)->applyAccumulatedDelta();
        ++shapeItr;
    }
}

void PhysicsSimulation::updateContacts() {
    PerformanceTimer perfTimer("3.5-updateContacts");
    int numCollisions = _collisions.size();
    for (int i = 0; i < numCollisions; ++i) {
        CollisionInfo* collision = _collisions.getCollision(i);
        quint64 key = collision->getShapePairKey();
        if (key == 0) {
            continue;
        }
        QMap<quint64, ContactConstraint>::iterator itr = _contacts.find(key);
        if (itr == _contacts.end()) {
            _contacts.insert(key, ContactConstraint(*collision, _frame));
        } else {
            itr.value().updateContact(*collision, _frame);
        }
    }
}

const quint32 MAX_CONTACT_FRAME_LIFETIME = 2;

void PhysicsSimulation::pruneContacts() {
    QMap<quint64, ContactConstraint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        if (_frame - itr.value().getLastFrame() > MAX_CONTACT_FRAME_LIFETIME) {
            itr = _contacts.erase(itr);
        } else {
            ++itr;
        }
    }
}
