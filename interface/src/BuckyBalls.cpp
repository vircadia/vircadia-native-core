//
//  BuckyBalls.cpp
//  interface/src
//
//  Created by Philip on 1/2/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BuckyBalls.h"

#include "Util.h"
#include "world.h"
#include "devices/SixenseManager.h"

const int NUM_ELEMENTS = 3;
const float RANGE_BBALLS = 0.5f;
const float SIZE_BBALLS = 0.02f;
const float CORNER_BBALLS = 2.f;
const float GRAVITY_BBALLS = -0.25f;
const float BBALLS_ATTRACTION_DISTANCE = SIZE_BBALLS / 2.f;
const float COLLISION_RADIUS = 0.01f;
const float INITIAL_VELOCITY = 0.3f;

glm::vec3 colors[NUM_ELEMENTS];

// Make some bucky balls for the avatar
BuckyBalls::BuckyBalls() {
    _bballIsGrabbed[0] = 0;
    _bballIsGrabbed[1] = 0;
    colors[0] = glm::vec3(0.13f, 0.55f, 0.13f);
    colors[1] = glm::vec3(0.64f, 0.16f, 0.16f);
    colors[2] = glm::vec3(0.31f, 0.58f, 0.80f);

    qDebug("Creating buckyballs...");
    for (int i = 0; i < NUM_BBALLS; i++) {
        _bballPosition[i] = CORNER_BBALLS + randVector() * RANGE_BBALLS;
        int element = (rand() % NUM_ELEMENTS);
        if (element == 0) {
            _bballRadius[i] = SIZE_BBALLS;
            _bballColor[i] = colors[0];
        } else if (element == 1) {
            _bballRadius[i] = SIZE_BBALLS / 2.f;
            _bballColor[i] = colors[1];
        } else {
            _bballRadius[i] = SIZE_BBALLS * 2.f;
            _bballColor[i] = colors[2];
        }
        _bballColliding[i] = 0.f;
        _bballElement[i] = element;
        if (_bballElement[i] != 1) {
            _bballVelocity[i] = randVector() * INITIAL_VELOCITY;
        } else {
            _bballVelocity[i] = glm::vec3(0);
        }
    }
}

void BuckyBalls::grab(PalmData& palm, float deltaTime) {
    float penetration;
    glm::vec3 fingerTipPosition = palm.getTipPosition();

    if (palm.getControllerButtons() & BUTTON_FWD) {
        if (!_bballIsGrabbed[palm.getSixenseID()]) {
            // Look for a ball to grab
            for (int i = 0; i < NUM_BBALLS; i++) {
                glm::vec3 diff = _bballPosition[i] - fingerTipPosition;
                penetration = glm::length(diff) - (_bballRadius[i] + COLLISION_RADIUS);
                if (penetration < 0.f) {
                    _bballIsGrabbed[palm.getSixenseID()] = i;
                }
            }
        }
        if (_bballIsGrabbed[palm.getSixenseID()]) {
            //  If ball being grabbed, move with finger
            glm::vec3 diff = _bballPosition[_bballIsGrabbed[palm.getSixenseID()]] - fingerTipPosition;
            penetration = glm::length(diff) - (_bballRadius[_bballIsGrabbed[palm.getSixenseID()]] + COLLISION_RADIUS);
            _bballPosition[_bballIsGrabbed[palm.getSixenseID()]] -= glm::normalize(diff) * penetration;
            glm::vec3 fingerTipVelocity = palm.getTipVelocity();
            if (_bballElement[_bballIsGrabbed[palm.getSixenseID()]] != 1) {
                _bballVelocity[_bballIsGrabbed[palm.getSixenseID()]] = fingerTipVelocity;
            }
            _bballPosition[_bballIsGrabbed[palm.getSixenseID()]] = fingerTipPosition;
            _bballColliding[_bballIsGrabbed[palm.getSixenseID()]] = 1.f;
        }
    } else {
        _bballIsGrabbed[palm.getSixenseID()] = 0;
    }
}

const float COLLISION_BLEND_RATE = 0.5f;
const float ATTRACTION_BLEND_RATE = 0.9f;
const float ATTRACTION_VELOCITY_BLEND_RATE = 0.10f;

void BuckyBalls::simulate(float deltaTime, const HandData* handData) {
    //  First, update the grab behavior from the hand controllers
    for (size_t i = 0; i < handData->getNumPalms(); ++i) {
        PalmData palm = handData->getPalms()[i];
        grab(palm, deltaTime);
    }

    //  Look for collisions
    for (int i = 0; i < NUM_BBALLS; i++) {
        if (_bballElement[i] != 1) {
            // For 'interacting' elements, look for other balls to interact with
            for (int j = 0; j < NUM_BBALLS; j++) {
                if (i != j) {
                    glm::vec3 diff = _bballPosition[i] - _bballPosition[j];
                    
                    float diffLength = glm::length(diff);
                    float penetration = diffLength - (_bballRadius[i] + _bballRadius[j]);
                    
                    if (diffLength != 0) {
                        if (penetration < 0.f) {
                            //  Colliding - move away and transfer velocity
                            _bballPosition[i] -= glm::normalize(diff) * penetration * COLLISION_BLEND_RATE;
                            if (glm::dot(_bballVelocity[i], diff) < 0.f) {
                                _bballVelocity[i] = _bballVelocity[i] * (1.f - COLLISION_BLEND_RATE) +
                                glm::reflect(_bballVelocity[i], glm::normalize(diff)) * COLLISION_BLEND_RATE;
                            }
                        } else if ((penetration > EPSILON) && (penetration < BBALLS_ATTRACTION_DISTANCE)) {
                            //  If they get close to each other, bring them together with magnetic force
                            _bballPosition[i] -= glm::normalize(diff) * penetration * ATTRACTION_BLEND_RATE;
                            
                            //  Also make their velocities more similar
                            _bballVelocity[i] = _bballVelocity[i] * (1.f - ATTRACTION_VELOCITY_BLEND_RATE) + _bballVelocity[j] * ATTRACTION_VELOCITY_BLEND_RATE;
                        }
                    }
                }
            }
        }
    }
    //  Update position and bounce on walls
    const float BBALL_CONTINUOUS_DAMPING = 0.00f;
    const float BBALL_WALL_COLLISION_DAMPING = 0.2f;
    const float COLLISION_DECAY_RATE = 0.8f;
    
    for (int i = 0; i < NUM_BBALLS; i++) {
        _bballPosition[i] += _bballVelocity[i] * deltaTime;
        if (_bballElement[i] != 1) {
            _bballVelocity[i].y += GRAVITY_BBALLS * deltaTime;
        }
        _bballVelocity[i] -= _bballVelocity[i] * BBALL_CONTINUOUS_DAMPING * deltaTime;
        for (int j = 0; j < 3; j++) {
            if ((_bballPosition[i][j] + _bballRadius[i]) > (CORNER_BBALLS + RANGE_BBALLS)) {
                _bballPosition[i][j] = (CORNER_BBALLS + RANGE_BBALLS) - _bballRadius[i];
                _bballVelocity[i][j] *= -(1.f - BBALL_WALL_COLLISION_DAMPING);
            }
            if ((_bballPosition[i][j] - _bballRadius[i]) < (CORNER_BBALLS -RANGE_BBALLS)) {
                _bballPosition[i][j] = (CORNER_BBALLS -RANGE_BBALLS) + _bballRadius[i];
                _bballVelocity[i][j] *= -(1.f - BBALL_WALL_COLLISION_DAMPING);
            }
        }
        _bballColliding[i] *= COLLISION_DECAY_RATE;
        if (_bballColliding[i] < 0.1f) {
            _bballColliding[i] = 0.f;
        }
    }
}

void BuckyBalls::render() {
    glEnable(GL_LIGHTING);
    for (int i = 0; i < NUM_BBALLS; i++) {
        if (_bballColliding[i] > 0.f) {
            const float GRAB_BRIGHTEN = 1.15f;
            glColor3f(_bballColor[i].x * GRAB_BRIGHTEN, _bballColor[i].y * GRAB_BRIGHTEN, _bballColor[i].z * GRAB_BRIGHTEN);
        } else {
            glColor3f(_bballColor[i].x, _bballColor[i].y, _bballColor[i].z);
        }
        glPushMatrix();
        glTranslatef(_bballPosition[i].x, _bballPosition[i].y, _bballPosition[i].z);
        glutSolidSphere(_bballRadius[i], 15, 15);
        glPopMatrix();
    }
}


