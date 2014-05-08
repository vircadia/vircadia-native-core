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

#include "CollisionInfo.h"


CollisionList::CollisionList(int maxSize) :
    _maxSize(maxSize),
    _size(0) {
    _collisions.resize(_maxSize);
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
    // we rely on the external context to properly set or clear the data members of a collision
    // whenever it is used.
    /*
    for (int i = 0; i < _size; ++i) {
        // we only clear the important stuff
        CollisionInfo& collision = _collisions[i];
        collision._type = COLLISION_TYPE_UNKNOWN;
        //collision._data = NULL;
        //collision._intData = 0;
        //collision._floatDAta = 0.0f;
        //collision._vecData = glm::vec3(0.0f);
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
