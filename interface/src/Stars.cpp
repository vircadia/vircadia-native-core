//
// Stars.cpp
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include "Stars.h"

#define __interface__Starfield_impl__
#include "starfield/Controller.h"
#undef __interface__Starfield_impl__

Stars::Stars() : 
    _ptrController(0l) { 
    _ptrController = new starfield::Controller; 
}

Stars::~Stars() { 
    delete _ptrController; 
}

bool Stars::readInput(const char* url, const char* cacheFile, unsigned limit) {
    return _ptrController->readInput(url, cacheFile, limit); 
}

bool Stars::setResolution(unsigned k) { 
    return _ptrController->setResolution(k); 
}

float Stars::changeLOD(float fraction, float overalloc, float realloc) { 
    return float(_ptrController->changeLOD(fraction, overalloc, realloc));
} 

void Stars::render(float fovDiagonal, float aspect, glm::mat4 const& view) {

    _ptrController->render(fovDiagonal, aspect, glm::affineInverse(view)); 
}


