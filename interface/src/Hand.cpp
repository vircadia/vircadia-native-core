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
    _ballColor(0.0f, 0.4f, 0.0f)
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
    _numLeapBalls = fingerPositions.size(); // just to test
    
    float unitScale = 0.001; // convert mm to meters
    glm::vec3 offset(0.2, -0.2, -0.3);  // place the hand in front of the face where we can see it
    
    Head& head = _owningAvatar->getHead();
    for (int b = 0; b < _numLeapBalls; b++) {
        glm::vec3 pos = unitScale * fingerPositions[b] + offset;
        _leapBall[b].rotation = head.getOrientation();
        _leapBall[b].position = head.getPosition() + head.getOrientation() * pos;
    }
}


