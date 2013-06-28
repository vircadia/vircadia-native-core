//
//  Hand.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QImage>

#include <AgentList.h>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Util.h"
#include "renderer/ProgramObject.h"

using namespace std;

Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),
    _owningAvatar(owningAvatar),
    _renderAlpha(1.0),
    _lookingInMirror(false),
    _ballColor(0.0, 0.4, 0.0),
    _position(0.0, 0.4, 0.0),
    _orientation(0.0, 0.0, 0.0, 1.0)
{
}

void Hand::init() {
    _numLeapBalls = 0;    
    for (int b = 0; b < MAX_AVATAR_LEAP_BALLS; b++) {
        _leapBall[b].position       = glm::vec3(0.0, 0.0, 0.0);
        _leapBall[b].velocity       = glm::vec3(0.0, 0.0, 0.0);
        _leapBall[b].radius         = 0.01;
        _leapBall[b].touchForce     = 0.0;
        _leapBall[b].isCollidable   = true;
    }
}

void Hand::reset() {
}

void Hand::simulate(float deltaTime, bool isMine) {
}

void Hand::calculateGeometry() {
    glm::vec3 offset(0.1, -0.1, -0.15);  // place the hand in front of the face where we can see it
    
    Head& head = _owningAvatar->getHead();
    _position = head.getPosition() + head.getOrientation() * offset;
    _orientation = head.getOrientation();

    _numLeapBalls = _fingerPositions.size();
    
    float unitScale = 0.001; // convert mm to meters
    for (int b = 0; b < _numLeapBalls; b++) {
        glm::vec3 pos = unitScale * _fingerPositions[b] + offset;
        _leapBall[b].rotation = _orientation;
        _leapBall[b].position = _position + _orientation * pos;
    }
}


void Hand::render(bool lookingInMirror) {

    _renderAlpha = 1.0;
    _lookingInMirror = lookingInMirror;
    
    calculateGeometry();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    renderHandSpheres();
}

void Hand::renderHandSpheres() {
    glPushMatrix();
    // Draw the leap balls
    for (int b = 0; b < _numLeapBalls; b++) {
        float alpha = 1.0f;
        
        if (alpha > 0.0f) {
            glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, alpha); // Just to test
            
            glPushMatrix();
            glTranslatef(_leapBall[b].position.x, _leapBall[b].position.y, _leapBall[b].position.z);
            glutSolidSphere(_leapBall[b].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
    }
    glPopMatrix();
}

void Hand::setLeapFingers(const std::vector<glm::vec3>& fingerPositions) {
    _fingerPositions = fingerPositions;
}


