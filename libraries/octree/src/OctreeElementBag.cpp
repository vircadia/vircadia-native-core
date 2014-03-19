//
//  OctreeElementBag.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/25/2013
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "OctreeElementBag.h"
#include <OctalCode.h>

OctreeElementBag::OctreeElementBag() : 
    _bagElements()
{
    OctreeElement::addDeleteHook(this);
};

OctreeElementBag::~OctreeElementBag() {
    OctreeElement::removeDeleteHook(this);
    deleteAll();
}

void OctreeElementBag::elementDeleted(OctreeElement* element) {
    remove(element); // note: remove can safely handle nodes that aren't in it, so we don't need to check contains()
}


void OctreeElementBag::deleteAll() {
    _bagElements.clear();
}


void OctreeElementBag::insert(OctreeElement* element) {
    _bagElements.insert(element);
}

OctreeElement* OctreeElementBag::extract() {
    OctreeElement* result = NULL;

    if (_bagElements.size() > 0) {
        QSet<OctreeElement*>::iterator front = _bagElements.begin();
        result = *front;
        _bagElements.erase(front);
    }
    return result;
}

bool OctreeElementBag::contains(OctreeElement* element) {
    return _bagElements.contains(element);
}

void OctreeElementBag::remove(OctreeElement* element) {
    _bagElements.remove(element);
}
