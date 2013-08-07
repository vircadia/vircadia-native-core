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

const bool SHOW_LEAP_HAND = true;

using namespace std;

Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),
    
    _raveGloveClock(0.0f),
    _raveGloveInitialized(false),
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
    
    _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_FIRE;
    _raveGloveEffectsModeChanged = false;
}

void Hand::reset() {
}


void Hand::simulate(float deltaTime, bool isMine) {

    if (_isRaveGloveActive) {
        if (_raveGloveEffectsModeChanged && _raveGloveInitialized) {
            activateNewRaveGloveMode();
            _raveGloveEffectsModeChanged = false;
        }
        
        updateRaveGloveParticles(deltaTime);
    }
}

void Hand::calculateGeometry() {
    const glm::vec3 leapHandsOffsetFromFace(0.0, -0.2, -0.3);  // place the hand in front of the face where we can see it
    
    Head& head = _owningAvatar->getHead();
    _basePosition = head.getPosition() + head.getOrientation() * leapHandsOffsetFromFace;
    _baseOrientation = head.getOrientation();

    // generate finger tip balls....
    _leapFingerTipBalls.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    const float standardBallRadius = 0.005f;
                    _leapFingerTipBalls.resize(_leapFingerTipBalls.size() + 1);
                    HandBall& ball = _leapFingerTipBalls.back();
                    ball.rotation = _baseOrientation;
                    ball.position = finger.getTipPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                }
            }
        }
    }

    // generate finger root balls....
    _leapFingerRootBalls.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    const float standardBallRadius = 0.005f;
                    _leapFingerRootBalls.resize(_leapFingerRootBalls.size() + 1);
                    HandBall& ball = _leapFingerRootBalls.back();
                    ball.rotation = _baseOrientation;
                    ball.position = finger.getRootPosition();
                    ball.radius         = standardBallRadius;
                    ball.touchForce     = 0.0;
                    ball.isCollidable   = true;
                }
            }
        }
    }
}

void Hand::setRaveGloveEffectsMode(QKeyEvent* event) {

    _raveGloveEffectsModeChanged = true;
    
    switch (event->key()) {
    
        case Qt::Key_0: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR; break;
        case Qt::Key_1: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_TRAILS;          break;
        case Qt::Key_2: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_FIRE;            break;
        case Qt::Key_3: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_WATER;           break;
        case Qt::Key_4: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_FLASHY;          break;
        case Qt::Key_5: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_BOZO_SPARKLER;   break;
        case Qt::Key_6: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_LONG_SPARKLER;   break;
        case Qt::Key_7: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_SNAKE;           break;
        case Qt::Key_8: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_PULSE;           break;
        case Qt::Key_9: _raveGloveEffectsMode = RAVE_GLOVE_EFFECTS_MODE_THROB;           break;
     };        
}

void Hand::render(bool lookingInMirror) {
    
    _renderAlpha = 1.0;
    _lookingInMirror = lookingInMirror;
    
    calculateGeometry();

    if ( SHOW_LEAP_HAND ) {
        if (!isRaveGloveActive()) {
            renderLeapFingerTrails();
        }
        if (isRaveGloveActive()) {
            // Use mood lighting for the hand itself
            setRaveLights(RAVE_LIGHTS_AVATAR);
        }
        renderLeapHands();
    }

    if (_isRaveGloveActive) {
        if (_raveGloveInitialized) {
            updateRaveGloveEmitters(); // do this after calculateGeometry
            
            // Use normal lighting for the particles
            setRaveLights(RAVE_LIGHTS_PARTICLES);
            _raveGloveParticleSystem.render();
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
}

void Hand::setRaveLights(RaveLightsSetting setting) {
    if (setting == RAVE_LIGHTS_AVATAR) {
        // Set some mood lighting
        GLfloat ambient_color[] = { 0.0, 0.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
        GLfloat diffuse_color[] = { 0.4, 0.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
        GLfloat specular_color[] = { 0.0, 0.0, 0.0, 0.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
        glMateriali(GL_FRONT, GL_SHININESS, 0);
    }
    else if (setting == RAVE_LIGHTS_PARTICLES) {
        // particles use a brighter light setting
        GLfloat ambient_color[] = { 0.7, 0.7, 0.8 };
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
        GLfloat diffuse_color[] = { 0.8, 0.7, 0.7 };
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
        GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
        glMateriali(GL_FRONT, GL_SHININESS, 96);
    }
}

void Hand::renderRaveGloveStage() {

    // Draw a simple fullscreen triangle fan, darkest in the center.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBegin(GL_TRIANGLE_FAN);
    // Dark center vertex
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    // Lighter outer vertices
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glVertex3f(-1.0f,-1.0f, 0.0f);
    glVertex3f( 1.0f,-1.0f, 0.0f);
    glVertex3f( 1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f,-1.0f, 0.0f);
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void Hand::renderLeapHands() {

    const float alpha = 1.0f;
    //const glm::vec3 handColor = _ballColor;
    const glm::vec3 handColor(1.0, 0.84, 0.66); // use the skin color
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glPushMatrix();
    // Draw the leap balls
    for (size_t i = 0; i < _leapFingerTipBalls.size(); i++) {
        if (alpha > 0.0f) {
            glColor4f(handColor.r, handColor.g, handColor.b, alpha);
            
            glPushMatrix();
            glTranslatef(_leapFingerTipBalls[i].position.x, _leapFingerTipBalls[i].position.y, _leapFingerTipBalls[i].position.z);
            glutSolidSphere(_leapFingerTipBalls[i].radius, 20.0f, 20.0f);
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
                    glColor4f(handColor.r, handColor.g, handColor.b, 0.5);
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
            glColor4f(handColor.r, handColor.g, handColor.b, 0.25);
            glm::vec3 tip = palm.getPosition();
            glm::vec3 root = palm.getPosition() + palm.getNormal() * palmThickness;
            Avatar::renderJointConnectingCone(root, tip, 0.05, 0.03);
        }
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
}

void Hand::renderLeapFingerTrails() {
    // Draw the finger root cones
    glDisable(GL_LIGHTING);
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
                        const float halfWidth = 0.004f;
                        const glm::vec3 edgeDirection(1.0f, 0.0f, 0.0f);
                        glm::vec3 edge0 = center + edgeDirection * halfWidth;
                        glm::vec3 edge1 = center - edgeDirection * halfWidth;
                        float alpha = 1.0f - ((float)t / (float)(numPositions - 1));
                        alpha *= 0.25f;
                        glColor4f(1.0f, 1.0f, 1.0f, alpha);
                        glVertex3fv((float*)&edge0);
                        glVertex3fv((float*)&edge1);
                    }
                    glEnd();
                }
            }
        }
    }
    glEnable(GL_LIGHTING);
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


// call this soon after the geometry of the leap hands are set
void Hand::updateRaveGloveEmitters() {
    int emitterIndex = 0;

    for (size_t i = 0; i < NUM_FINGERS; i++) {
        _raveGloveParticleSystem.setEmitterActive(_raveGloveEmitter[i], false);
    }

    for (size_t palmIndex = 0; palmIndex < getNumPalms(); ++palmIndex) {
        PalmData& palm = getPalms()[palmIndex];
        if (palm.isActive()) {
            for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                FingerData& finger = palm.getFingers()[f];
                if (finger.isActive()) {
                    if (emitterIndex < NUM_FINGERS) {   // safety, stop at the array size
                        glm::vec3 fingerDirection = finger.getTipPosition() - finger.getRootPosition();
                        float fingerLength = glm::length(fingerDirection);
                        
                        if (fingerLength > 0.0f) {
                            fingerDirection /= fingerLength;
                        } else {
                            fingerDirection = IDENTITY_UP;
                        }
                        
                        _raveGloveParticleSystem.setEmitterActive   (_raveGloveEmitter[emitterIndex], true);
                        _raveGloveParticleSystem.setEmitterPosition (_raveGloveEmitter[emitterIndex], finger.getTipPosition());
                        _raveGloveParticleSystem.setEmitterDirection(_raveGloveEmitter[emitterIndex], fingerDirection);
                    }
                }
                emitterIndex++;
            }
        }
    }
}


// call this from within the simulate method
void Hand::updateRaveGloveParticles(float deltaTime) {

    if (!_raveGloveInitialized) {
    
        // start up the rave glove finger particles...
        for ( int f = 0; f< NUM_FINGERS; f ++ ) {
            _raveGloveEmitter[f] = _raveGloveParticleSystem.addEmitter();            
            assert( _raveGloveEmitter[f] >= 0 );
            assert( _raveGloveEmitter[f] != NULL_EMITTER );
        }
                                                    
        setRaveGloveMode(RAVE_GLOVE_EFFECTS_MODE_FIRE);
        activateNewRaveGloveMode();
        _raveGloveParticleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));
        _raveGloveInitialized = true;         
    } else {        
        _raveGloveParticleSystem.simulate(deltaTime); 
    }
}

// The rave glove mode has changed, so activate the effects.
void Hand::activateNewRaveGloveMode() {

    if (!_raveGloveInitialized) {
        return;
    }
    
    int mode = _raveGloveEffectsMode;
    _raveGloveParticleSystem.killAllParticles();

    for ( int f = 0; f< NUM_FINGERS; f ++ ) {

        ParticleSystem::ParticleAttributes attributes;

        //-----------------------------------------
        // throbbing color cycle
        //-----------------------------------------
        if (mode == RAVE_GLOVE_EFFECTS_MODE_THROBBING_COLOR) {
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.03f );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 30.0f );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 20   );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);
            
            attributes.modulationAmplitude = 1.0;
            attributes.modulationRate      = 0.33;
            attributes.modulationStyle     = COLOR_MODULATION_STYLE_RAINBOW_CYCLE;
            attributes.color               = glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f);
            attributes.radius              = 0.02f;
            attributes.gravity             = 0.0f;
            attributes.airFriction         = 0.0f;
            attributes.jitter              = 0.0f;
            attributes.bounce              = 0.0f;
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
        // long sparkler
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
        
        //-----------------------------------------
        // throb
        //-----------------------------------------
        } else if (mode == RAVE_GLOVE_EFFECTS_MODE_THROB) {
            
            _raveGloveParticleSystem.setParticleRenderStyle       (_raveGloveEmitter[f], PARTICLE_RENDER_STYLE_SPHERE  );
            _raveGloveParticleSystem.setShowingEmitterBaseParticle(_raveGloveEmitter[f], true );
            _raveGloveParticleSystem.setEmitterParticleLifespan   (_raveGloveEmitter[f], 0.03  );
            _raveGloveParticleSystem.setEmitterThrust             (_raveGloveEmitter[f], 0.0f );
            _raveGloveParticleSystem.setEmitterRate               (_raveGloveEmitter[f], 30.0 );
            _raveGloveParticleSystem.setEmitterParticleResolution (_raveGloveEmitter[f], 20   );

            _raveGloveParticleSystem.setParticleAttributesToDefault(&attributes);

            attributes.radius               = 0.01f;
            attributes.color                = glm::vec4( 0.1f, 0.2f, 0.4f, 0.5f);
            attributes.modulationAmplitude  = 0.5;
            attributes.modulationRate       = 3.0;
            attributes.modulationStyle      = COLOR_MODULATION_STYLE_LIGHTNESS_WAVE;
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_0, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_1, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_2, attributes);
            _raveGloveParticleSystem.setParticleAttributes(_raveGloveEmitter[f], PARTICLE_LIFESTAGE_3, attributes);
        }
    }
}




