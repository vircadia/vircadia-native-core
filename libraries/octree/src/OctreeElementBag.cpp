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

OctreeElementBag::OctreeElementBag() : 
    _bagElements()
{
    OctreeElement::addDeleteHook(this);
    _hooked = true;
}

OctreeElementBag::~OctreeElementBag() {
    unhookNotifications();
    deleteAll();
}

void OctreeElementBag::unhookNotifications() {
    if (_hooked) {
        OctreeElement::removeDeleteHook(this);
        _hooked = false;
    }
}

void OctreeElementBag::elementDeleted(OctreeElementPointer element) {
    remove(element); // note: remove can safely handle nodes that aren't in it, so we don't need to check contains()
}


void OctreeElementBag::deleteAll() {
    _bagElements.clear();
}


void OctreeElementBag::insert(OctreeElementPointer element) {
    _bagElements.insert(element);
}

OctreeElementPointer OctreeElementBag::extract() {
    OctreeElementPointer result = NULL;

    if (_bagElements.size() > 0) {
        QSet<OctreeElementPointer>::iterator front = _bagElements.begin();
        result = *front;
        _bagElements.erase(front);
    }
    return result;
}

bool OctreeElementBag::contains(OctreeElementPointer element) {
    return _bagElements.contains(element);
}

void OctreeElementBag::remove(OctreeElementPointer element) {
    _bagElements.remove(element);
}
