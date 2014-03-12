//
// Stars.cpp
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include "Stars.h" 

#include "starfield/Controller.h"

Stars::Stars() : 
    _controller(0l), _starsLoaded(false) {
    _controller = new starfield::Controller;
}

Stars::~Stars() { 
    delete _controller; 
}

bool Stars::generate(unsigned numStars, unsigned seed) {
    _starsLoaded = _controller->computeStars(numStars, seed);
    return _starsLoaded;
}

bool Stars::setResolution(unsigned k) { 
    return _controller->setResolution(k); 
}

void Stars::render(float fovY, float aspect, float nearZ, float alpha) {
    // determine length of screen diagonal from quadrant height and aspect ratio
    float quadrantHeight = nearZ * tan(RADIANS_PER_DEGREE * fovY * 0.5f);
    float halfDiagonal = sqrt(quadrantHeight * quadrantHeight * (1.0f + aspect * aspect));

    // determine fov angle in respect to the diagonal
    float fovDiagonal = atan(halfDiagonal / nearZ) * 2.0f;

    // pull the modelview matrix off the GL stack
    glm::mat4 view; glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view)); 

    _controller->render(fovDiagonal, aspect, glm::affineInverse(view), alpha);
}


