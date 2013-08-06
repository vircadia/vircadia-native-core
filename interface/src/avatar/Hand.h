//
//  Hand.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Hand_h
#define hifi_Hand_h

#include <QAction>
#include <glm/glm.hpp>
#include <AvatarData.h>
#include <HandData.h>
#include "Balls.h"
#include "world.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include "ParticleSystem.h"
#include <SharedUtil.h>
#include <vector>

class Avatar;
class ProgramObject;

class Hand : public HandData {
public:
    Hand(Avatar* owningAvatar);
    
    struct HandBall
    {
        glm::vec3        position;       // the actual dynamic position of the ball at any given time
        glm::quat        rotation;       // the rotation of the ball
        glm::vec3        velocity;       // the velocity of the ball
        float            radius;         // the radius of the ball
        bool             isCollidable;   // whether or not the ball responds to collisions
        float            touchForce;     // a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };

    void init();
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(bool lookingInMirror);

    void setBallColor      (glm::vec3 ballColor         ) { _ballColor          = ballColor;          }
    void updateRaveGloveParticles(float deltaTime);
    void updateRaveGloveEmitters();
    void setRaveGloveEffectsMode(QKeyEvent* event);

    // getters
    const glm::vec3& getLeapFingerTipBallPosition (int ball) const { return _leapFingerTipBalls [ball].position;}
    const glm::vec3& getLeapFingerRootBallPosition(int ball) const { return _leapFingerRootBalls[ball].position;}

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
    
    ParticleSystem _raveGloveParticleSystem;
    float          _raveGloveClock;
    bool           _raveGloveInitialized;
    int            _raveGloveEmitter[NUM_FINGERS];

    Avatar*        _owningAvatar;
    float          _renderAlpha;
    bool           _lookingInMirror;
    glm::vec3      _ballColor;
    std::vector<HandBall> _leapFingerTipBalls;
    std::vector<HandBall> _leapFingerRootBalls;
    
    // private methods
    void setLeapHands(const std::vector<glm::vec3>& handPositions,
                      const std::vector<glm::vec3>& handNormals);

    void activateNewRaveGloveMode();

    void renderRaveGloveStage();
    void renderLeapHandSpheres();
    void renderLeapHands();
    void renderLeapHand(PalmData& hand);
    void renderLeapFingerTrails();
    void calculateGeometry();
};

#endif
