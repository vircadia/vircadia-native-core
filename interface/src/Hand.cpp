//
//  Hand.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QImage>

#include <NodeList.h>

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
    _ballColor(0.0, 0.0, 0.4),
    _particleSystemInitialized(false)
{
    // initialize all finger particle emitters with an invalid id as default
    for (int f = 0; f< NUM_FINGERS_PER_HAND; f ++ ) {
        _fingerParticleEmitter[f] = -1;
    }
}

void Hand::init() {
    // Different colors for my hand and others' hands
    if (_owningAvatar && _owningAvatar->isMyAvatar()) {
        _ballColor = glm::vec3(0.0, 0.4, 0.0);
    }
    else
        _ballColor = glm::vec3(0.0, 0.0, 0.4);
}

void Hand::reset() {
}

void Hand::simulate(float deltaTime, bool isMine) {
    if (_isRaveGloveActive) {
        updateFingerParticles(deltaTime);
    }
}

void Hand::calculateGeometry() {
    glm::vec3 offset(0.2, -0.2, -0.3);  // place the hand in front of the face where we can see it
    
    Head& head = _owningAvatar->getHead();
    _basePosition = head.getPosition() + head.getOrientation() * offset;
    _baseOrientation = head.getOrientation();

    int numLeapBalls = _fingerTips.size();
    _leapBalls.resize(numLeapBalls);
    
    for (int i = 0; i < _fingerTips.size(); ++i) {
        _leapBalls[i].rotation = _baseOrientation;
        _leapBalls[i].position = leapPositionToWorldPosition(_fingerTips[i]);
        _leapBalls[i].radius         = 0.01;
        _leapBalls[i].touchForce     = 0.0;
        _leapBalls[i].isCollidable   = true;
    }
}


void Hand::render(bool lookingInMirror) {
    
    _renderAlpha = 1.0;
    _lookingInMirror = lookingInMirror;
    
    calculateGeometry();

    if (_isRaveGloveActive) {
        renderRaveGloveStage();

        if (_particleSystemInitialized) {
            _particleSystem.render();
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    renderHandSpheres();
}

void Hand::renderRaveGloveStage() {
    if (_owningAvatar && _owningAvatar->isMyAvatar()) {
        Head& head = _owningAvatar->getHead();
        glm::quat headOrientation = head.getOrientation();
        glm::vec3 headPosition = head.getPosition();
        float scale = 100.0f;
        glm::vec3 vc = headOrientation * glm::vec3( 0.0f,  0.0f, -30.0f) + headPosition;
        glm::vec3 v0 = headOrientation * (glm::vec3(-1.0f, -1.0f, 0.0f) * scale) + vc;
        glm::vec3 v1 = headOrientation * (glm::vec3( 1.0f, -1.0f, 0.0f) * scale) + vc;
        glm::vec3 v2 = headOrientation * (glm::vec3( 1.0f,  1.0f, 0.0f) * scale) + vc;
        glm::vec3 v3 = headOrientation * (glm::vec3(-1.0f,  1.0f, 0.0f) * scale) + vc;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        glVertex3fv((float*)&vc);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glVertex3fv((float*)&v0);
        glVertex3fv((float*)&v1);
        glVertex3fv((float*)&v2);
        glVertex3fv((float*)&v3);
        glVertex3fv((float*)&v0);
        glEnd();
        glEnable(GL_DEPTH_TEST);
    }
}

void Hand::renderHandSpheres() {
    glPushMatrix();
    // Draw the leap balls
    for (size_t i = 0; i < _leapBalls.size(); i++) {
        float alpha = 1.0f;
        
        if (alpha > 0.0f) {
            glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, alpha);
            
            glPushMatrix();
            glTranslatef(_leapBalls[i].position.x, _leapBalls[i].position.y, _leapBalls[i].position.z);
            glutSolidSphere(_leapBalls[i].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
    }
    
    // Draw the finger root cones
    if (_fingerTips.size() == _fingerRoots.size()) {
        for (size_t i = 0; i < _fingerTips.size(); ++i) {
            glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, 0.5);
            glm::vec3 tip = leapPositionToWorldPosition(_fingerTips[i]);
            glm::vec3 root = leapPositionToWorldPosition(_fingerRoots[i]);
            Avatar::renderJointConnectingCone(root, tip, 0.001, 0.003);
        }
    }

    // Draw the palms
    if (_handPositions.size() == _handNormals.size()) {
        for (size_t i = 0; i < _handPositions.size(); ++i) {
            glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, 0.25);
            glm::vec3 tip = leapPositionToWorldPosition(_handPositions[i]);
            glm::vec3 root = leapPositionToWorldPosition(_handPositions[i] + (_handNormals[i] * 2.0f));
            Avatar::renderJointConnectingCone(root, tip, 0.05, 0.03);
        }
    }

    glPopMatrix();
}

void Hand::setLeapFingers(const std::vector<glm::vec3>& fingerTips,
                          const std::vector<glm::vec3>& fingerRoots) {
    _fingerTips = fingerTips;
    _fingerRoots = fingerRoots;
}

void Hand::setLeapHands(const std::vector<glm::vec3>& handPositions,
                          const std::vector<glm::vec3>& handNormals) {
    _handPositions = handPositions;
    _handNormals = handNormals;
}


void Hand::updateFingerParticles(float deltaTime) {

    if (!_particleSystemInitialized) {
        for ( int f = 0; f< NUM_FINGERS_PER_HAND; f ++ ) {
            _fingerParticleEmitter[f] = _particleSystem.addEmitter();
            _particleSystem.setShowingEmitter(_fingerParticleEmitter[f], true);
        }
        _particleSystemInitialized = true;         
    } else {
        // update the particles
        
        static float t = 0.0f;
        t += deltaTime;
                
        for ( int f = 0; f< _fingerTips.size(); f ++ ) {
            
            if (_fingerParticleEmitter[f] != -1) {
                    
                glm::vec3 particleEmitterPosition = leapPositionToWorldPosition(_fingerTips[f]);
                       
                // this aspect is still being designed....
                       
                glm::vec3 tilt = glm::vec3
                (
                    30.0f * sinf( t * 0.55f ),
                    0.0f,
                    30.0f * cosf( t * 0.75f )
                );
                
                glm::quat particleEmitterRotation = glm::quat(glm::radians(tilt));

                _particleSystem.setEmitterPosition(_fingerParticleEmitter[0], particleEmitterPosition);
                _particleSystem.setEmitterRotation(_fingerParticleEmitter[0], particleEmitterRotation);
                
                float radius = 0.005f;
                glm::vec4 color(1.0f, 0.6f, 0.0f, 0.5f);
                glm::vec3 velocity(0.0f, 0.005f, 0.0f);
                float lifespan = 0.3f;
                _particleSystem.emitParticlesNow(_fingerParticleEmitter[0], 1, radius, color, velocity, lifespan); 
            }  
        }
        
        _particleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));  
        _particleSystem.simulate(deltaTime); 
    }
}



