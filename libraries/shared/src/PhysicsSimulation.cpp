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

#include "PerfStat.h"
#include "PhysicsEntity.h"
#include "Ragdoll.h"
#include "SharedUtil.h"
#include "ShapeCollider.h"

int MAX_DOLLS_PER_SIMULATION = 16;
int MAX_ENTITIES_PER_SIMULATION = 64;
int MAX_COLLISIONS_PER_SIMULATION = 256;


PhysicsSimulation::PhysicsSimulation() : _translation(0.0f), _frameCount(0), _entity(NULL), _ragdoll(NULL), 
        _collisions(MAX_COLLISIONS_PER_SIMULATION) {
}

PhysicsSimulation::~PhysicsSimulation() {
    // entities have a backpointer to this simulator that must be cleaned up
    int numEntities = _otherEntities.size();
    for (int i = 0; i < numEntities; ++i) {
        _otherEntities[i]->_simulation = NULL;
    }
    _otherEntities.clear();
    if (_entity) {
        _entity->_simulation = NULL;
    }

    // but Ragdolls do not
    _ragdoll = NULL;
    _otherRagdolls.clear();
}

void PhysicsSimulation::setRagdoll(Ragdoll* ragdoll) { 
    if (_ragdoll != ragdoll) {
        if (_ragdoll) {
            _ragdoll->_simulation = NULL;
        }
        _ragdoll = ragdoll;
        if (_ragdoll) {
            assert(!(_ragdoll->_simulation));
            _ragdoll->_simulation = this;
        }
    }
}

void PhysicsSimulation::setEntity(PhysicsEntity* entity) {
    if (_entity != entity) {
        if (_entity) {
            assert(_entity->_simulation == this);
            _entity->_simulation = NULL;
        }
        _entity = entity;
        if (_entity) {
            assert(!(_entity->_simulation));
            _entity->_simulation = this;
        }
    }
}

bool PhysicsSimulation::addEntity(PhysicsEntity* entity) {
    if (!entity) {
        return false;
    }
    if (entity->_simulation == this) {
        int numEntities = _otherEntities.size();
        for (int i = 0; i < numEntities; ++i) {
            if (entity == _otherEntities.at(i)) {
                // already in list
                return true;
            }
        }
        // belongs to some other simulation
        return false;
    }
    int numEntities = _otherEntities.size();
    if (numEntities > MAX_ENTITIES_PER_SIMULATION) {
        // list is full
        return false;
    }
    // add to list
    assert(!(entity->_simulation));
    entity->_simulation = this;
    _otherEntities.push_back(entity);
    return true;
}

void PhysicsSimulation::removeEntity(PhysicsEntity* entity) {
    if (!entity || !entity->_simulation || !(entity->_simulation == this)) {
        return;
    }
    removeShapes(entity);
    int numEntities = _otherEntities.size();
    for (int i = 0; i < numEntities; ++i) {
        if (entity == _otherEntities.at(i)) {
            if (i == numEntities - 1) {
                // remove it
                _otherEntities.pop_back();
            } else {
                // swap the last for this one
                PhysicsEntity* lastEntity = _otherEntities[numEntities - 1];
                _otherEntities.pop_back();
                _otherEntities[i] = lastEntity;
            }
            entity->_simulation = NULL;
            break;
        }
    }
}

void PhysicsSimulation::removeShapes(const PhysicsEntity* entity) {
    // remove data structures with pointers to entity's shapes
    QMap<quint64, ContactPoint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        if (entity == itr.value().getShapeA()->getEntity() || entity == itr.value().getShapeB()->getEntity()) {
            itr = _contacts.erase(itr);
        } else {
            ++itr;
        }
    }
}

const float OTHER_RAGDOLL_MASS_SCALE = 10.0f;

bool PhysicsSimulation::addRagdoll(Ragdoll* doll) {
    if (!doll) {
        return false;
    }
    int numDolls = _otherRagdolls.size();
    if (numDolls > MAX_DOLLS_PER_SIMULATION) {
        // list is full
        return false;
    }
    if (doll->_simulation == this) {
        for (int i = 0; i < numDolls; ++i) {
            if (doll == _otherRagdolls[i]) {
                // already in list
                return true;
            }
        }
    }
    // add to list
    assert(!(doll->_simulation));
    doll->_simulation = this;
    _otherRagdolls.push_back(doll);

    // set the massScale of otherRagdolls artificially high
    doll->setMassScale(OTHER_RAGDOLL_MASS_SCALE);
    return true;
}

void PhysicsSimulation::removeRagdoll(Ragdoll* doll) {
    int numDolls = _otherRagdolls.size();
    if (doll->_simulation != this) {
        return;
    }
    for (int i = 0; i < numDolls; ++i) {
        if (doll == _otherRagdolls[i]) {
            if (i == numDolls - 1) {
                // remove it
                _otherRagdolls.pop_back();
            } else {
                // swap the last for this one
                Ragdoll* lastDoll = _otherRagdolls[numDolls - 1];
                _otherRagdolls.pop_back();
                _otherRagdolls[i] = lastDoll;
            }
            doll->_simulation = NULL;
            doll->setMassScale(1.0f);
            break;
        }
    }
}

void PhysicsSimulation::stepForward(float deltaTime, float minError, int maxIterations, quint64 maxUsec) {
    ++_frameCount;
    if (!_ragdoll) {
        return;
    }
    quint64 now = usecTimestampNow();
    quint64 startTime = now;
    quint64 expiry = startTime + maxUsec;

    moveRagdolls(deltaTime);
    enforceContacts();
    int numDolls = _otherRagdolls.size();
    {
        PerformanceTimer perfTimer("enforce");
        _ragdoll->enforceConstraints();
        for (int i = 0; i < numDolls; ++i) {
            _otherRagdolls[i]->enforceConstraints();
        }
    }

    int iterations = 0;
    float error = 0.0f;
    do {
        computeCollisions();
        updateContacts();
        resolveCollisions();

        { // enforce constraints
            PerformanceTimer perfTimer("enforce");
            error = _ragdoll->enforceConstraints();
            for (int i = 0; i < numDolls; ++i) {
                error = glm::max(error, _otherRagdolls[i]->enforceConstraints());
            }
        }
        applyContactFriction();
        ++iterations;

        now = usecTimestampNow();
    } while (_collisions.size() != 0 && (iterations < maxIterations) && (error > minError) && (now < expiry));

    pruneContacts();
}

void PhysicsSimulation::moveRagdolls(float deltaTime) {
    PerformanceTimer perfTimer("integrate");
    _ragdoll->stepForward(deltaTime);
    int numDolls = _otherRagdolls.size();
    for (int i = 0; i < numDolls; ++i) {
        _otherRagdolls[i]->stepForward(deltaTime);
    }
}

void PhysicsSimulation::computeCollisions() {
    PerformanceTimer perfTimer("collide");
    _collisions.clear();

    const QVector<Shape*> shapes = _entity->getShapes();
    int numShapes = shapes.size();
    // collide main ragdoll with self
    for (int i = 0; i < numShapes; ++i) {
        const Shape* shape = shapes.at(i);
        if (!shape) {
            continue;
        }
        for (int j = i+1; j < numShapes; ++j) {
            const Shape* otherShape = shapes.at(j);
            if (otherShape && _entity->collisionsAreEnabled(i, j)) {
                ShapeCollider::collideShapes(shape, otherShape, _collisions);
            }
        }
    }

    // collide main ragdoll with others
    int numEntities = _otherEntities.size();
    for (int i = 0; i < numEntities; ++i) {
        const QVector<Shape*> otherShapes = _otherEntities.at(i)->getShapes();
        ShapeCollider::collideShapesWithShapes(shapes, otherShapes, _collisions);
    }
}

void PhysicsSimulation::resolveCollisions() {
    PerformanceTimer perfTimer("resolve");
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
    PerformanceTimer perfTimer("contacts");
    QMap<quint64, ContactPoint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        itr.value().enforce();
        ++itr;
    }
}

void PhysicsSimulation::applyContactFriction() {
    PerformanceTimer perfTimer("contacts");
    QMap<quint64, ContactPoint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        itr.value().applyFriction();
        ++itr;
    }
}

void PhysicsSimulation::updateContacts() {
    PerformanceTimer perfTimer("contacts");
    int numCollisions = _collisions.size();
    for (int i = 0; i < numCollisions; ++i) {
        CollisionInfo* collision = _collisions.getCollision(i);
        quint64 key = collision->getShapePairKey();
        if (key == 0) {
            continue;
        }
        QMap<quint64, ContactPoint>::iterator itr = _contacts.find(key);
        if (itr == _contacts.end()) {
            _contacts.insert(key, ContactPoint(*collision, _frameCount));
        } else {
            itr.value().updateContact(*collision, _frameCount);
        }
    }
}

const quint32 MAX_CONTACT_FRAME_LIFETIME = 2;

void PhysicsSimulation::pruneContacts() {
    QMap<quint64, ContactPoint>::iterator itr = _contacts.begin();
    while (itr != _contacts.end()) {
        if (_frameCount - itr.value().getLastFrame() > MAX_CONTACT_FRAME_LIFETIME) {
            itr = _contacts.erase(itr);
        } else {
            ++itr;
        }
    }
}
