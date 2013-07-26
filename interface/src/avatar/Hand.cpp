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

const bool SHOW_LEAP_HAND = false;

using namespace std;

Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),
    
    _testRaveGloveClock(0.0f),
    _testRaveGloveMode(0),
    _particleSystemInitialized(false),
    _owningAvatar(owningAvatar),
    _renderAlpha(1.0),
    _lookingInMirror(false),
    _isRaveGloveActive(false),
    _ballColor(0.0, 0.0, 0.4)

 {
    // initialize all finger particle emitters with an invalid id as default
    for (int f = 0; f< NUM_FINGERS; f ++ ) {
        _fingerParticleEmitter[f] = -1;
    }
}

void Hand::init() {
    // Different colors for my hand and others' hands
    if (_owningAvatar && _owningAvatar->isMyAvatar()) {
        _ballColor = glm::vec3(0.0, 0.4, 0.0);
    }
    else {
        _ballColor = glm::vec3(0.0, 0.0, 0.4);
    }
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

void Hand::setRaveGloveEffectsMode(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_0: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_0); break;
        case Qt::Key_1: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_1); break;
        case Qt::Key_2: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_2); break;
        case Qt::Key_3: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_3); break;
        case Qt::Key_4: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_4); break;
        case Qt::Key_5: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_5); break;
        case Qt::Key_6: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_6); break;
        case Qt::Key_7: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_7); break;
        case Qt::Key_8: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_8); break;
        case Qt::Key_9: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_9); break;
    };        
}

void Hand::render(bool lookingInMirror) {
    
    _renderAlpha = 1.0;
    _lookingInMirror = lookingInMirror;
    
    calculateGeometry();

    if (_isRaveGloveActive) {
        renderRaveGloveStage();

        if (_particleSystemInitialized) {
            updateFingerParticleEmitters(); // do this after calculateGeometry
            _particleSystem.render();
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    if ( SHOW_LEAP_HAND ) {
        renderFingerTrails();
        renderHandSpheres();
    }
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

void Hand::renderFingerTrails() {
    // Draw the finger root cones
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                int numPositions = finger.getTrailNumPositions();
                if (numPositions > 0) {
                    glBegin(GL_TRIANGLE_STRIP);
                    for (int t = 0; t < numPositions; ++t)
                    {
                        const glm::vec3& center = finger.getTrailPosition(t);
                        const float halfWidth = 0.001f;
                        const glm::vec3 edgeDirection(1.0f, 0.0f, 0.0f);
                        glm::vec3 edge0 = center + edgeDirection * halfWidth;
                        glm::vec3 edge1 = center - edgeDirection * halfWidth;
                        float alpha = 1.0f - ((float)t / (float)(numPositions - 1));
                        glColor4f(1.0f, 0.0f, 0.0f, alpha);
                        glVertex3fv((float*)&edge0);
                        glVertex3fv((float*)&edge1);
                    }
                    glEnd();
                }
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

// call this right after the geometry of the leap hands are set
void Hand::updateFingerParticleEmitters() {

    if (_particleSystemInitialized) {
    
        int fingerIndex = 0;
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (palm.isActive()) {
                for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                    FingerData& finger = palm.getFingers()[f];
                    if (finger.isActive()) {
                        if (_fingerParticleEmitter[0] != -1) {
                                                        
                            glm::vec3 fingerDirection = finger.getTipPosition() - finger.getRootPosition();
                            float fingerLength = glm::length(fingerDirection);
                            
                            if (fingerLength > 0.0f) {
                                fingerDirection /= fingerLength;
                            } else {
                                fingerDirection = IDENTITY_UP;
                            }
                            
                            _particleSystem.setEmitterPosition (_fingerParticleEmitter[fingerIndex], finger.getTipPosition());
                            _particleSystem.setEmitterDirection(_fingerParticleEmitter[fingerIndex], fingerDirection);
                            fingerIndex ++;
                         }
                    }
                }
            }
        }
    }
}


// call this from within the simulate method
void Hand::updateFingerParticles(float deltaTime) {

    if (!_particleSystemInitialized) {
    
        // start up the rave glove finger particles...
        for ( int f = 0; f< NUM_FINGERS; f ++ ) {
            _fingerParticleEmitter[f] = _particleSystem.addEmitter();
            assert( _fingerParticleEmitter[f] != -1 );
        }
                                            
        setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_2);
        _particleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));
        _particleSystemInitialized = true;         
    } else {        
        
        _testRaveGloveClock += deltaTime;
        
        if (_testRaveGloveMode == RAVE_GLOVE_EFFECTS_MODE_0) {
            ParticleSystem::ParticleAttributes attributes;
            float red   = 0.5f + 0.5f * sinf(_testRaveGloveClock * 1.4f);
            float green = 0.5f + 0.5f * cosf(_testRaveGloveClock * 1.7f);
            float blue  = 0.5f + 0.5f * sinf(_testRaveGloveClock * 2.0f);
            
            attributes.color = glm::vec4(red, green, blue, 1.0f);            
            attributes.radius = 0.01f + 0.005f * sinf(_testRaveGloveClock * 2.2f);
            for ( int f = 0; f< NUM_FINGERS; f ++ ) {
                _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
                _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
                _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
                _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            }
        } 

        _particleSystem.simulate(deltaTime); 
    }
}

void Hand::setRaveGloveMode(int mode) {

    _testRaveGloveMode = mode;

    _particleSystem.killAllParticles();

    for ( int f = 0; f< NUM_FINGERS; f ++ ) {

        ParticleSystem::ParticleAttributes attributes;

        //-----------------------------------------
        // throbbing color cycle
        //-----------------------------------------
        if (mode == RAVE_GLOVE_EFFECTS_MODE_0) {
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_SPHERE );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], true );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 0.0f );

            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.0f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 30.0f );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 20   );

            attributes.radius               = 0.02f;
            attributes.color                = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.0f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 0.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);

        //-----------------------------------------
        // trails
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_1) {            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_RIBBON );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], false );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 1.0f );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.0f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 50.0f );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 5    );

            attributes.radius               = 0.001f;
            attributes.color                = glm::vec4( 1.0f, 0.5f, 0.2f, 1.0f);
            attributes.gravity              = 0.005f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.0f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 0.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.002f;
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.color = glm::vec4( 1.0f, 0.2f, 0.2f, 0.5f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.color = glm::vec4( 1.0f, 0.2f, 0.2f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        }
        
        //-----------------------------------------
        // Fire!
        //-----------------------------------------
        if (mode == RAVE_GLOVE_EFFECTS_MODE_2) {

            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], false  );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 1.0f   );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.002f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 50.0    );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 6      );

            attributes.radius               = 0.005f;
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
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.jitter = 0.0f;
            attributes.gravity = -0.005f;
            attributes.color  = glm::vec4( 1.0f, 0.2f, 0.0f, 0.4f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.gravity = 0.0f;
            attributes.color  = glm::vec4( 0.4f, 0.4f, 0.4f, 0.2f);
             _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.02f;
            attributes.color  = glm::vec4( 0.4f, 0.6f, 0.9f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            
        //-----------------------------------------
        // water
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_3) {
            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_SPHERE );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], true   );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 0.6f   );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.001f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 100.0  );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 5      );

            attributes.radius               = 0.001f;
            attributes.color                = glm::vec4( 0.8f, 0.9f, 1.0f, 0.5f);
            attributes.gravity              = -0.01f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.002f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 1.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.gravity = 0.01f;
            attributes.jitter  = 0.0f;
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.color  = glm::vec4( 0.8f, 0.9f, 1.0f, 0.2f);
            attributes.radius = 0.002f;
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.color = glm::vec4( 0.8f, 0.9f, 1.0f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            
        //-----------------------------------------
        // flashy
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_4) {
            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], true   );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 0.1    );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.002f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 100.0    );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 12     );

            attributes.radius               = 0.0f;
            attributes.color                = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.05f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 1.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 0.0f, 1.0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.0f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // Bozo sparkler
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_5) {
            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_RIBBON  );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], false   );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 0.2    );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.002f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 100.0    );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 12     );

            attributes.radius               = 0.0f;
            attributes.color                = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.01f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 1.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 0.0f, .0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.0f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // long sparkler
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_6) {
            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_RIBBON  );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], false   );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 1.0     );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.002f  );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 100.0     );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 7       );

            attributes.radius               = 0.0f;
            attributes.color                = glm::vec4( 0.0f, 0.0f, 0.4f, 0.0f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.0f;
            attributes.jitter               = 0.0001f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 0.0f;
            attributes.neighborAttraction   = 0.0f;
            attributes.neighborRepulsion    = 0.0f;
            attributes.bounce               = 1.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.005f;
            attributes.color = glm::vec4( 0.0f, 0.5f, 0.5f, 0.8f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.007f;
            attributes.color = glm::vec4( 0.5f, 0.0f, 0.5f, 0.5f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.02f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // bubble snake
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_7) {
            
            _particleSystem.setParticleRenderStyle       (_fingerParticleEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _particleSystem.setShowingEmitterBaseParticle(_fingerParticleEmitter[f], true   );
            _particleSystem.setEmitterParticleLifespan   (_fingerParticleEmitter[f], 1.0    );
            _particleSystem.setEmitterThrust             (_fingerParticleEmitter[f], 0.002f );
            _particleSystem.setEmitterRate               (_fingerParticleEmitter[f], 100.0    );
            _particleSystem.setEmitterParticleResolution (_fingerParticleEmitter[f], 7      );

            attributes.radius               = 0.001f;
            attributes.color                = glm::vec4( 0.5f, 1.0f, 0.5f, 1.0f);
            attributes.gravity              = 0.0f;
            attributes.airFriction          = 0.01f;
            attributes.jitter               = 0.0f;
            attributes.emitterAttraction    = 0.0f;
            attributes.tornadoForce         = 1.1f;
            attributes.neighborAttraction   = 1.1f;
            attributes.neighborRepulsion    = 1.1f;
            attributes.bounce               = 0.0f;
            attributes.usingCollisionSphere = false;            
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius               = 0.002f;
            attributes.color                = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius               = 0.003f;
            attributes.color                = glm::vec4( 0.3f, 0.3f, 0.3f, 0.5f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius               = 0.004f;
            attributes.color                = glm::vec4( 0.3f, 0.3f, 0.3f, 0.0f);
            _particleSystem.setParticleAttributes(_fingerParticleEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        }
    }
}




