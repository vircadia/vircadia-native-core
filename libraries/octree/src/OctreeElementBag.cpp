//
//  OctreeElementBag.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 4/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeElementBag.h"
#include <OctalCode.h>

void OctreeElementBag::deleteAll() {
    _bagElements = Bag();
}

bool OctreeElementBag::isEmpty() {
    // Pop all expired front elements
    while (!_bagElements.empty() && _bagElements.front().expired()) {
        _bagElements.pop();
    }
    
    return _bagElements.empty();
}

void OctreeElementBag::insert(OctreeElementPointer element) {
    _bagElements.push(element);
}

OctreeElementPointer OctreeElementBag::extract() {
    OctreeElementPointer result;

    // Find the first element still alive
    while (!result && !_bagElements.empty()) {
        result = _bagElements.front().lock(); // Grab head's shared_ptr
        _bagElements.pop();
    }
    return result;
}
