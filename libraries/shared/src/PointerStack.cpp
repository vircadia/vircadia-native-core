//
//  PointerStack.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 5/11/2013
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "PointerStack.h"
#include <stdio.h>

PointerStack::~PointerStack() {
    deleteAll();
}

void PointerStack::deleteAll() {
    if (_elements) {
        delete[] _elements;
    }
    _elements = NULL;
    _elementsInUse = 0;
    _sizeOfElementsArray = 0;
}

const int GROW_BY = 100;

void PointerStack::growAndPush(void* element) {
    //printf("PointerStack::growAndPush() _sizeOfElementsArray=%d",_sizeOfElementsArray);
    void** oldElements = _elements;
    _elements = new void* [_sizeOfElementsArray + GROW_BY];
    _sizeOfElementsArray += GROW_BY;
    
    // If we had an old stack...
    if (oldElements) {
        // copy old elements into the new stack
        memcpy(_elements, oldElements, _elementsInUse * sizeof(void*));
        delete[] oldElements;
    }
    _elements[_elementsInUse] = element;
    _elementsInUse++;
}

