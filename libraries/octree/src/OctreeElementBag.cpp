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
    _bagElements(NULL),
    _elementsInUse(0),
    _sizeOfElementsArray(0) {
    OctreeElement::addDeleteHook(this);
};

OctreeElementBag::~OctreeElementBag() {
    OctreeElement::removeDeleteHook(this);
    deleteAll();
}

void OctreeElementBag::deleteAll() {
    if (_bagElements) {
        delete[] _bagElements;
    }
    _bagElements = NULL;
    _elementsInUse = 0;
    _sizeOfElementsArray = 0;
}


const int GROW_BAG_BY = 100;

// put a node into the bag
void OctreeElementBag::insert(OctreeElement* element) {

    // Search for where we should live in the bag (sorted)
    // Note: change this to binary search... instead of linear!
    int insertAt = _elementsInUse;
    for (int i = 0; i < _elementsInUse; i++) {
        // just compare the pointers... that's good enough
        if (_bagElements[i] == element) {
            return; // exit early!!
        }
        
        if (_bagElements[i] > element) {
            insertAt = i;
            break;
        }
    }
    // at this point, inserAt will be the location we want to insert at.
    
    // If we don't have room in our bag, then grow the bag
    if (_sizeOfElementsArray < _elementsInUse + 1) {
        OctreeElement** oldBag = _bagElements;
        _bagElements = new OctreeElement*[_sizeOfElementsArray + GROW_BAG_BY];
        _sizeOfElementsArray += GROW_BAG_BY;
        
        // If we had an old bag...
        if (oldBag) {
            // copy old elements into the new bag, but leave a space where we need to
            // insert the new node
            memcpy(_bagElements, oldBag, insertAt * sizeof(OctreeElement*));
            memcpy(&_bagElements[insertAt + 1], &oldBag[insertAt], (_elementsInUse - insertAt) * sizeof(OctreeElement*));
            delete[] oldBag;
        }
    } else {
        // move existing elements further back in the bag array, leave a space where we need to
        // insert the new node
        memmove(&_bagElements[insertAt + 1], &_bagElements[insertAt], (_elementsInUse - insertAt) * sizeof(OctreeElement*));
    }
    _bagElements[insertAt] = element;
    _elementsInUse++;
}
 
// pull a node out of the bag (could come in any order)
OctreeElement* OctreeElementBag::extract() {
    // pull the last node out, and shrink our list...
    if (_elementsInUse) {
        
        // get the last element
        OctreeElement* element = _bagElements[_elementsInUse - 1];
        
        // reduce the count
        _elementsInUse--;

        return element;
    }
    return NULL;
}

bool OctreeElementBag::contains(OctreeElement* element) {
    for (int i = 0; i < _elementsInUse; i++) {
        // just compare the pointers... that's good enough
        if (_bagElements[i] == element) {
            return true; // exit early!!
        }
        // if we're past where it should be, then it's not here!
        if (_bagElements[i] > element) {
            return false;
        }
    }
    // if we made it through the entire bag, it's not here!
    return false;
}

void OctreeElementBag::remove(OctreeElement* element) {
    int foundAt = -1;
    for (int i = 0; i < _elementsInUse; i++) {
        // just compare the pointers... that's good enough
        if (_bagElements[i] == element) {
            foundAt = i;
            break;
        }
        // if we're past where it should be, then it's not here!
        if (_bagElements[i] > element) {
            break;
        }
    }
    // if we found it, then we need to remove it....
    if (foundAt != -1) {
        memmove(&_bagElements[foundAt], &_bagElements[foundAt + 1], (_elementsInUse - foundAt) * sizeof(OctreeElement*));
        _elementsInUse--;
    }
}


void OctreeElementBag::elementDeleted(OctreeElement* element) {
    remove(element); // note: remove can safely handle nodes that aren't in it, so we don't need to check contains()
}


