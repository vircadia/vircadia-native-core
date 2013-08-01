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
    
    _raveGloveClock(0.0f),
    _raveGloveMode(RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR),
    _raveGloveInitialized(false),
    _isRaveGloveActive(false),
    _owningAvatar(owningAvatar),
    _renderAlpha(1.0),
    _lookingInMirror(false),
    _ballColor(0.0, 0.0, 0.4)    
 {
    // initialize all finger particle emitters with an invalid id as default
    for (int f = 0; f< NUM_FINGERS; f ++ ) {
        _raveGloveEmitter[f] = NULL_EMITTER;
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
        updateRaveGloveParticles(deltaTime);
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
    
        case Qt::Key_0: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR); break;
        case Qt::Key_1: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_TRAILS         ); break;
        case Qt::Key_2: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_FIRE           ); break;
        case Qt::Key_3: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_WATER          ); break;
        case Qt::Key_4: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_FLASHY         ); break;
        case Qt::Key_5: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_BOZO_SPARKLER  ); break;
        case Qt::Key_6: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_LONG_SPARKLER  ); break;
        case Qt::Key_7: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_SNAKE          ); break;
        case Qt::Key_8: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_PULSE          ); break;
        case Qt::Key_9: setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_THROB          ); break;
     };        
}

void Hand::render(bool lookingInMirror) {
    
    _renderAlpha = 1.0;
    _lookingInMirror = lookingInMirror;
    
    calculateGeometry();

    if (_isRaveGloveActive) {
        renderRaveGloveStage();

        if (_raveGloveInitialized) {
            updateRaveGloveEmitters(); // do this after calculateGeometry
            _raveGloveParticleSystem.render();
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
void Hand::updateRaveGloveEmitters() {

    bool debug = false;

    if (_raveGloveInitialized) {
        
        if(debug) printf( "\n" );
        if(debug) printf( "------------------------------------\n" );
        if(debug) printf( "updating rave glove emitters:\n" );
        if(debug) printf( "------------------------------------\n" );
        
        int emitterIndex = 0;

        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            
            if(debug) printf( "\n" );
            if(debug) printf( "palm %d ", (int)i );
            
            if (palm.isActive()) {
            
                if(debug) printf( "is active\n" );

                for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                    FingerData& finger = palm.getFingers()[f];

                    if(debug) printf( "emitterIndex %d:    ", emitterIndex );

                    if (finger.isActive()) {
                    
                        if ((emitterIndex >=0)
                        &&  (emitterIndex < NUM_FINGERS)) {
                        
                            assert(emitterIndex >=0 );
                            assert(emitterIndex < NUM_FINGERS );

                            if(debug) printf( "_raveGloveEmitter[%d] = %d\n", emitterIndex, _raveGloveEmitter[emitterIndex] );
                            
                            glm::vec3 fingerDirection = finger.getTipPosition() - finger.getRootPosition();
                            float fingerLength = glm::length(fingerDirection);
                            
                            if (fingerLength > 0.0f) {
                                fingerDirection /= fingerLength;
                            } else {
                                fingerDirection = IDENTITY_UP;
                            }
                            
                            assert(_raveGloveEmitter[emitterIndex] >=0 );
                            assert(_raveGloveEmitter[emitterIndex] < NUM_FINGERS );
                            
                            _raveGloveParticleSystem.setEmitterPosition (_raveGloveEmitter[emitterIndex], finger.getTipPosition());
                            _raveGloveParticleSystem.setEmitterDirection(_raveGloveEmitter[emitterIndex], fingerDirection);
                        }
                    } else {
                        if(debug) printf( "BOGUS finger\n" );
                    }
                            
                    emitterIndex ++;
                }
            } else {
                if(debug) printf( "is NOT active\n" );
            }
        }
    }
}


// call this from within the simulate method
void Hand::updateRaveGloveParticles(float deltaTime) {

    if (!_raveGloveInitialized) {
    
        //printf( "Initializing rave glove emitters:\n" );
        //printf( "The indices of the emitters are:\n" );
    
        // start up the rave glove finger particles...
        for ( int f = 0; f< NUM_FINGERS; f ++ ) {
            _raveGloveEmitter[f] = _raveGloveParticleSystem.addEmitter();
            assert( _raveGloveEmitter[f] >= 0 );
            assert( _raveGloveEmitter[f] != NULL_EMITTER );

            //printf( "%d\n", _raveGloveEmitter[f] );
        }
                                                    
        setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_FIRE);
        _raveGloveParticleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));
        _raveGloveInitialized = true;         
    } else {        
        
        _raveGloveClock += deltaTime;
        
        // this rave glove effect oscillates though various colors and radii that are meant to show off some effects
        if (_raveGloveMode == RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR) {
            ParticleSystem::ParticleAttributes attributes;
            float red   = 0.5f + 0.5f * sinf(_raveGloveClock * 1.4f);
            float green = 0.5f + 0.5f * cosf(_raveGloveClock * 1.7f);
            float blue  = 0.5f + 0.5f * sinf(_raveGloveClock * 2.0f);
            float alpha = 1.0f;
            
            attributes.color = glm::vec4(red, green, blue, alpha);            
            attributes.radius = 0.01f + 0.005f * sinf(_raveGloveClock * 2.2f);
            attributes.modulationAmplitude = 0.0f;
            
            for ( int f = 0; f< NUM_FINGERS; f ++ ) {
                _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
                _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
                _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
                _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            }
        } 

        _raveGloveParticleSystem.simulate(deltaTime); 
    }
}

void Hand::setRaveGloveMode(int mode) {

    _raveGloveMode = mode;

    _raveGloveParticleSystem.killAllParticles();

    for ( int f = 0; f< NUM_FINGERS; f ++ ) {

        ParticleSystem::ParticleAttributes attributes;

        //-----------------------------------------
        // throbbing color cycle
        //-----------------------------------------
        if (mode == RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR) {
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 30.0f );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 20   );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);
            
            attributes.radius      = 0.02f;
            attributes.gravity     = 0.0f;
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.0f;
            attributes.bounce      = 0.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);

        //-----------------------------------------
        // trails
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_TRAILS) {            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_RIBBON );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], false );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 1.0f );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 50.0f );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 5    );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius      = 0.001f;
            attributes.color       = glm::vec4( 1.0f, 0.5f, 0.2f, 1.0f);
            attributes.gravity     = 0.005f;
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.0f;
            attributes.bounce      = 0.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.002f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.color = glm::vec4( 1.0f, 0.2f, 0.2f, 0.5f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.color = glm::vec4( 1.0f, 0.2f, 0.2f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        }
        
        //-----------------------------------------
        // Fire!
        //-----------------------------------------
        if (mode == RAVE_GLOVE_EFFECTS_MODE_FIRE) {

            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], false  );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 1.0f   );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.002f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 120.0    );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 6      );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius      = 0.005f;
            attributes.color       = glm::vec4( 1.0f, 1.0f, 0.5f, 0.5f);
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.003f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.jitter = 0.0f;
            attributes.gravity = -0.005f;
            attributes.color  = glm::vec4( 1.0f, 0.2f, 0.0f, 0.4f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.gravity = 0.0f;
            attributes.color  = glm::vec4( 0.4f, 0.4f, 0.4f, 0.2f);
             _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.02f;
            attributes.color  = glm::vec4( 0.4f, 0.6f, 0.9f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            
        //-----------------------------------------
        // water
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_WATER) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true   );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.6f   );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.001f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 100.0  );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 5      );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius      = 0.001f;
            attributes.color       = glm::vec4( 0.8f, 0.9f, 1.0f, 0.5f);
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.004f;
            attributes.bounce      = 1.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.gravity = 0.01f;
            attributes.jitter  = 0.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.color  = glm::vec4( 0.8f, 0.9f, 1.0f, 0.2f);
            attributes.radius = 0.002f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.color = glm::vec4( 0.8f, 0.9f, 1.0f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
            
        //-----------------------------------------
        // flashy
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_FLASHY) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true   );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.1    );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.002f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 100.0    );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 12     );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius      = 0.0f;
            attributes.color       = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.05f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 0.0f, 1.0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // Bozo sparkler
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_BOZO_SPARKLER) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_RIBBON  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], false   );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.2    );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.002f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 100.0    );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 12     );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius      = 0.0f;
            attributes.color       = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.01f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.01f;
            attributes.color = glm::vec4( 1.0f, 0.0f, .0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.0f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // long sparkler
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_LONG_SPARKLER) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_RIBBON  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], false   );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 1.0     );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.002f  );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 100.0     );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 7       );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.color       = glm::vec4( 0.3f, 0.3f, 0.3f, 0.4f);
            attributes.radius      = 0.0f;
            attributes.airFriction = 0.0f;
            attributes.jitter      = 0.0001f;
            attributes.bounce      = 1.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.005f;
            attributes.color = glm::vec4( 0.0f, 0.5f, 0.5f, 0.8f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.007f;
            attributes.color = glm::vec4( 0.5f, 0.0f, 0.5f, 0.5f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.02f;
            attributes.color = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // bubble snake
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_SNAKE) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true   );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 1.0    );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.002f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 100.0    );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 7      );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius             = 0.001f;
            attributes.color              = glm::vec4( 0.5f, 1.0f, 0.5f, 1.0f);
            attributes.airFriction        = 0.01f;
            attributes.jitter             = 0.0f;
            attributes.emitterAttraction  = 0.0f;
            attributes.tornadoForce       = 1.1f;
            attributes.neighborAttraction = 1.1f;
            attributes.neighborRepulsion  = 1.1f;
            attributes.bounce             = 0.0f;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);

            attributes.radius = 0.002f;
            attributes.color  = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);

            attributes.radius = 0.003f;
            attributes.color  = glm::vec4( 0.3f, 0.3f, 0.3f, 0.5f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);

            attributes.radius = 0.004f;
            attributes.color  = glm::vec4( 0.3f, 0.3f, 0.3f, 0.0f);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // pulse
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_PULSE) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.0  );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 30.0 );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 20   );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius               = 0.01f;
            attributes.color                = glm::vec4( 0.1f, 0.2f, 0.4f, 0.5f);
            attributes.modulationAmplitude  = 0.9;
            attributes.modulationRate       = 7.0;
            attributes.modulationStyle      = COLOR_MODULATION_STYLE_LIGHNTESS_PULSE;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        
        //-----------------------------------------
        // throb
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_LONG_SPARKLER) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.0  );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 30.0 );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 20   );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius               = 0.01f;
            attributes.color                = glm::vec4( 0.5f, 0.4f, 0.3f, 0.5f);
            attributes.modulationAmplitude  = 0.3;
            attributes.modulationRate       = 1.0;
            attributes.modulationStyle      = COLOR_MODULATION_STYLE_LIGHTNESS_WAVE;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        }
    }
}




