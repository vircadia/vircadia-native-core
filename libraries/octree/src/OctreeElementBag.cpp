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
    _bagElements.clear();
}

void OctreeElementBag::insert(OctreeElementPointer element) {
    _bagElements.insert(element.get(), element);
}

OctreeElementPointer OctreeElementBag::extract() {
    OctreeElementPointer result;

    // Find the first element still alive
    while (!_bagElements.empty() && !result) {
        auto it = _bagElements.begin();
        result = it->lock();
        _bagElements.erase(it);
    }
    return result;
}

bool OctreeElementBag::contains(OctreeElementPointer element) {
    return _bagElements.contains(element.get());
}

void OctreeElementBag::remove(OctreeElementPointer element) {
    _bagElements.remove(element.get());
}
