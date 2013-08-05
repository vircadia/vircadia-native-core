//
//  BendyLine.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "BendyLine.h"
#include "Util.h"
#include "world.h"

const float DEFAULT_BENDY_LINE_SPRING_FORCE  =  10.0f;
const float DEFAULT_BENDY_LINE_TORQUE_FORCE  =  0.1f;
const float DEFAULT_BENDY_LINE_DRAG          =  10.0f;
const float DEFAULT_BENDY_LINE_LENGTH        =  0.09f;
const float DEFAULT_BENDY_LINE_THICKNESS     =  0.03f;

BendyLine::BendyLine(){

    _springForce   = DEFAULT_BENDY_LINE_SPRING_FORCE;
    _torqueForce   = DEFAULT_BENDY_LINE_TORQUE_FORCE;
    _drag          = DEFAULT_BENDY_LINE_DRAG;
    _length        = DEFAULT_BENDY_LINE_LENGTH;
    _thickness     = DEFAULT_BENDY_LINE_THICKNESS;
    
    _gravityForce  = glm::vec3(0.0f, 0.0f, 0.0f);
    _basePosition  = glm::vec3(0.0f, 0.0f, 0.0f);	
    _baseDirection = glm::vec3(0.0f, 0.0f, 0.0f);  
    _midPosition   = glm::vec3(0.0f, 0.0f, 0.0f);          
    _endPosition   = glm::vec3(0.0f, 0.0f, 0.0f);          
    _midVelocity   = glm::vec3(0.0f, 0.0f, 0.0f);          
    _endVelocity   = glm::vec3(0.0f, 0.0f, 0.0f);  
}

void BendyLine::reset() {

    _midPosition = _basePosition + _baseDirection * _length * ONE_HALF;          
    _endPosition = _midPosition  + _baseDirection * _length * ONE_HALF;          
    _midVelocity = glm::vec3(0.0f, 0.0f, 0.0f);          
    _endVelocity = glm::vec3(0.0f, 0.0f, 0.0f);  
}

void BendyLine::update(float deltaTime) {

    glm::vec3 midAxis = _midPosition - _basePosition;
    glm::vec3 endAxis = _endPosition - _midPosition;

    float midLength = glm::length(midAxis);        
    float endLength = glm::length(endAxis);

    glm::vec3 midDirection;
    glm::vec3 endDirection;
    
    if (midLength > 0.0f) {
        midDirection = midAxis / midLength;
    } else {
        midDirection = _baseDirection;
    }
    
    if (endLength > 0.0f) {
        endDirection = endAxis / endLength;
    } else {
        endDirection = _baseDirection;
    }
    
    // add spring force
    float midForce = midLength - _length * ONE_HALF;
    float endForce = endLength - _length * ONE_HALF;
    _midVelocity -= midDirection * midForce * _springForce * deltaTime;
    _endVelocity -= endDirection * endForce * _springForce * deltaTime;

    // add gravity force    
    _midVelocity += _gravityForce;
    _endVelocity += _gravityForce;

    // add torque force
    _midVelocity += _baseDirection * _torqueForce * deltaTime;
    _endVelocity += midDirection   * _torqueForce * deltaTime;
    
    // add drag force
    float momentum = 1.0f - (_drag * deltaTime);
    if (momentum < 0.0f) {
        _midVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        _endVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    } else {
        _midVelocity *= momentum;
        _endVelocity *= momentum;
    }
        
    // update position by velocity
    _midPosition += _midVelocity;
    _endPosition += _endVelocity;

    // clamp lengths
    glm::vec3 newMidVector = _midPosition - _basePosition;
    glm::vec3 newEndVector = _endPosition - _midPosition;

    float newMidLength = glm::length(newMidVector);
    float newEndLength = glm::length(newEndVector);
    
    glm::vec3 newMidDirection;
    glm::vec3 newEndDirection;

    if (newMidLength > 0.0f) {
        newMidDirection = newMidVector/newMidLength;
    } else {
        newMidDirection = _baseDirection;
    }

    if (newEndLength > 0.0f) {
        newEndDirection = newEndVector/newEndLength;
    } else {
        newEndDirection = _baseDirection;
    }
    
    _endPosition = _midPosition  + newEndDirection * _length * ONE_HALF;
    _midPosition = _basePosition + newMidDirection * _length * ONE_HALF;
}


