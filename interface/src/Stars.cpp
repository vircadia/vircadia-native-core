//
// Stars.cpp
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include "FieldOfView.h"
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

bool Stars::readInput(const char* url, unsigned limit) {
    return _ptrController->readInput(url, limit); 
}

bool Stars::setResolution(unsigned k) { 
    return _ptrController->setResolution(k); 
}

float Stars::changeLOD(float fraction, float overalloc, float realloc) { 
    return float(_ptrController->changeLOD(fraction, overalloc, realloc));
} 

void Stars::render(FieldOfView const& fov) {
    _ptrController->render(fov.getPerspective(), fov.getAspectRatio(), fov.getOrientation()); 
}


