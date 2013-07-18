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

    _leapBalls.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    const float standardBallRadius = 0.01f;
                    _leapBalls.resize(_leapBalls.size() + 1);
                    HandBall& ball = _leapBalls.back();
                    ball.rotation = _baseOrientation;
                    ball.position = finger.getTipPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                }
            }
        }
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
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, 0.5);
                    glm::vec3 tip = finger.getTipPosition();
                    glm::vec3 root = finger.getRootPosition();
                    Avatar::renderJointConnectingCone(root, tip, 0.001, 0.003);
                }
            }
        }
    }

    // Draw the palms
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            const float palmThickness = 0.002f;
            glColor4f(_ballColor.r, _ballColor.g, _ballColor.b, 0.25);
            glm::vec3 tip = palm.getPosition();
            glm::vec3 root = palm.getPosition() + palm.getNormal() * palmThickness;
            Avatar::renderJointConnectingCone(root, tip, 0.05, 0.03);
        }
    }

    glPopMatrix();
}

void Hand::setLeapFingers(const std::vector<glm::vec3>& fingerTips,
                          const std::vector<glm::vec3>& fingerRoots) {
    // TODO: add id-checking here to increase finger stability
    
    size_t fingerIndex = 0;
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            if (fingerIndex < fingerTips.size()) {
                finger.setActive(true);
                finger.setRawTipPosition(fingerTips[fingerIndex]);
                finger.setRawRootPosition(fingerRoots[fingerIndex]);
                fingerIndex++;
            }
            else {
                finger.setActive(false);
            }
        }
    }
}

void Hand::setLeapHands(const std::vector<glm::vec3>& handPositions,
                          const std::vector<glm::vec3>& handNormals) {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (i < handPositions.size()) {
            palm.setActive(true);
            palm.setRawPosition(handPositions[i]);
            palm.setRawNormal(handNormals[i]);
        }
        else {
            palm.setActive(false);
        }
    }
}


void Hand::updateFingerParticles(float deltaTime) {

    if (!_particleSystemInitialized) {
                    
        for ( int f = 0; f< NUM_FINGERS_PER_HAND; f ++ ) {

            _fingerParticleEmitter[f] = _particleSystem.addEmitter();
            
            ParticleSystem::ParticleAttributes attributes;

           // set attributes for each life stage of the particle:
            attributes.radius               = 0.0f;
            attributes.color                = glm::vec4( 1.0f, 1.0f, 0.5f, 0.5f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.002f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 1.0f;
            attributes.usingCollisionSphere = false;
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], 0, attributes);

            attributes.radius = 0.01f;
            attributes.jitter = 0.0f;
            attributes.gravity = -0.005f;
            attributes.color  = glm::vec4( 1.0f, 0.2f, 0.0f, 0.4f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], 1, attributes);

            attributes.radius = 0.01f;
            attributes.gravity = 0.0f;
            attributes.color  = glm::vec4( 0.0f, 0.0f, 0.0f, 0.2f);
             _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], 2, attributes);

            attributes.radius = 0.02f;
            attributes.color  = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f);
             _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], 3, attributes);
        }

        _particleSystemInitialized = true;         
    } else {
        // update the particles
        
        static float t = 0.0f;
        t += deltaTime;

        int fingerIndex = 0;
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (palm.isActive()) {
                for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                    FingerData& finger = palm.getFingers()[f];
                    if (finger.isActive()) {
                        if (_fingerParticleEmitter[fingerIndex] != -1) {
                            
                            glm::vec3 particleEmitterPosition = finger.getTipPosition();
                            
                            glm::vec3 fingerDirection = particleEmitterPosition - leapPositionToWorldPosition(finger.getRootPosition());
                            float fingerLength = glm::length(fingerDirection);
                            
                            if (fingerLength > 0.0f) {
                                fingerDirection /= fingerLength;
                            } else {
                                fingerDirection = IDENTITY_UP;
                            }
                            
                            glm::quat particleEmitterRotation = rotationBetween(IDENTITY_UP, fingerDirection);
                            
                            _particleSystem.setEmitterPosition(_fingerParticleEmitter[f], particleEmitterPosition);
                            _particleSystem.setEmitterRotation(_fingerParticleEmitter[f], particleEmitterRotation);
                            
                            float radius = 0.005f;
                            const glm::vec4 color(1.0f, 0.6f, 0.0f, 0.5f);
                            const glm::vec3 velocity = fingerDirection * 0.002f;
                            const float lifespan = 1.0f;
                            _particleSystem.emitParticlesNow(_fingerParticleEmitter[f], 1, radius, color, velocity, lifespan); 
                        }
                    }
                }
            }
        }
        
        _particleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));  
        _particleSystem.simulate(deltaTime); 
    }
}



