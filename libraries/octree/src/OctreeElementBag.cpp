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

/// does the bag contain elements?
/// if all of the contained elements are expired, they will not report as empty, and
/// a single last item will be returned by extract as a null pointer
bool OctreeElementBag::isEmpty() {
    return _bagElements.empty();
}

void OctreeElementBag::insert(OctreeElementPointer element) {
    _bagElements[element.get()] = element;
}

OctreeElementPointer OctreeElementBag::extract() {
    OctreeElementPointer result;

    // Find the first element still alive
    Bag::iterator it = _bagElements.begin();
    while (it != _bagElements.end() && !result) {
        result = it->second.lock();
        it = _bagElements.erase(it);
    }
    return result;
}
