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

CollisionInfo* CollisionList::getCollision(int index) {
    return (index > -1 && index < _size) ? &(_collisions[index]) : NULL;
}

CollisionInfo* CollisionList::getLastCollision() {
    return (_size > 0) ? &(_collisions[_size - 1]) : NULL;
}

void CollisionList::clear() {
    for (int i = 0; i < _size; ++i) {
        // we only clear the important stuff
        CollisionInfo& collision = _collisions[i];
        collision._type = BASE_COLLISION;
        collision._data = NULL; // CollisionInfo does not own whatever this points to.
        collision._flags = 0;
        // we rely on the consumer to properly overwrite these fields when the collision is "created"
        //collision._damping;
        //collision._elasticity;
        //collision._contactPoint;
        //collision._penetration;
        //collision._addedVelocity;
    }
    _size = 0;
}

const CollisionInfo* CollisionList::operator[](int index) const {
    return (index > -1 && index < _size) ? &(_collisions[index]) : NULL;
}
