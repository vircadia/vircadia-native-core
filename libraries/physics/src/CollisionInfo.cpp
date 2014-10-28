//
//  CollisionInfo.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 02/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <SharedUtil.h>

#include "CollisionInfo.h"
#include "Shape.h"

CollisionInfo::CollisionInfo() :
        _data(NULL),
        _intData(0),
        _shapeA(NULL),
        _shapeB(NULL),
        _damping(0.f),
        _elasticity(1.f),
        _contactPoint(0.f), 
        _penetration(0.f), 
        _addedVelocity(0.f) {
}

quint64 CollisionInfo::getShapePairKey() const {
    if (_shapeB == NULL || _shapeA == NULL) {
        // zero is an invalid key
        return 0;
    }
    quint32 idA = _shapeA->getID();
    quint32 idB = _shapeB->getID();
    return idA < idB ? ((quint64)idA << 32) + (quint64)idB : ((quint64)idB << 32) + (quint64)idA;
}

CollisionList::CollisionList(int maxSize) :
    _maxSize(maxSize),
    _size(0) {
    _collisions.resize(_maxSize);
}

void CollisionInfo::apply() {
    assert(_shapeA);
    // NOTE: Shape::computeEffectiveMass() has side effects: computes and caches partial Lagrangian coefficients
    Shape* shapeA = const_cast<Shape*>(_shapeA);
    float massA = shapeA->computeEffectiveMass(_penetration, _contactPoint);
    float massB = MAX_SHAPE_MASS;
    float totalMass = massA + massB;
    if (_shapeB) {
        Shape* shapeB = const_cast<Shape*>(_shapeB);
        massB = shapeB->computeEffectiveMass(-_penetration, _contactPoint - _penetration);
        totalMass = massA + massB;
        if (totalMass < EPSILON) {
            massA = massB = 1.0f;
            totalMass = 2.0f;
        }
        // remember that _penetration points from A into B
        shapeB->accumulateDelta(massA / totalMass, _penetration);
    }   
    // NOTE: Shape::accumulateDelta() uses the coefficients from previous call to Shape::computeEffectiveMass()
    // remember that _penetration points from A into B
    shapeA->accumulateDelta(massB / totalMass, -_penetration);
}

CollisionInfo* CollisionList::getNewCollision() {
    // return pointer to existing CollisionInfo, or NULL of list is full
    return (_size < _maxSize) ? &(_collisions[_size++]) : NULL;
}

void CollisionList::deleteLastCollision() {
    if (_size > 0) {
        --_size;
    }
}

CollisionInfo* CollisionList::getCollision(int index) {
    return (index > -1 && index < _size) ? &(_collisions[index]) : NULL;
}

CollisionInfo* CollisionList::getLastCollision() {
    return (_size > 0) ? &(_collisions[_size - 1]) : NULL;
}

void CollisionList::clear() {
    // we rely on the external context to properly set or clear the data members of CollisionInfos
    /*
    for (int i = 0; i < _size; ++i) {
        // we only clear the important stuff
        CollisionInfo& collision = _collisions[i];
        //collision._data = NULL;
        //collision._intData = 0;
        //collision._floatDAta = 0.0f;
        //collision._vecData = glm::vec3(0.0f);
        //collision._shapeA = NULL;
        //collision._shapeB = NULL;
        //collision._damping;
        //collision._elasticity;
        //collision._contactPoint;
        //collision._penetration;
        //collision._addedVelocity;
    }
    */
    _size = 0;
}

CollisionInfo* CollisionList::operator[](int index) {
    return (index > -1 && index < _size) ? &(_collisions[index]) : NULL;
}
