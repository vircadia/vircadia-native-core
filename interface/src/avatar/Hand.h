//
//  Hand.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Hand_h
#define hifi_Hand_h

#include <QApplication>
#include <QAction>
#include <QSettings>
#include <QTouchEvent>
#include <QList>

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

enum RaveGloveEffectsMode
{
	RAVE_GLOVE_EFFECTS_MODE_NULL = -1,
	RAVE_GLOVE_EFFECTS_MODE_0,
	RAVE_GLOVE_EFFECTS_MODE_1,
	RAVE_GLOVE_EFFECTS_MODE_2,
	RAVE_GLOVE_EFFECTS_MODE_3,
	RAVE_GLOVE_EFFECTS_MODE_4,
	RAVE_GLOVE_EFFECTS_MODE_5,
	RAVE_GLOVE_EFFECTS_MODE_6,
	RAVE_GLOVE_EFFECTS_MODE_7,
	RAVE_GLOVE_EFFECTS_MODE_8,
	RAVE_GLOVE_EFFECTS_MODE_9,
	NUM_RAVE_GLOVE_EFFECTS_MODES
};

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
    void updateFingerParticles(float deltaTime);
    void updateFingerParticleEmitters();
    void setRaveGloveActive(bool active) { _isRaveGloveActive = active; }
    void setRaveGloveEffectsMode(QKeyEvent* event);

    // getters
    const glm::vec3& getLeapBallPosition       (int ball)       const { return _leapBalls[ball].position;}
    bool isRaveGloveActive                     ()               const { return _isRaveGloveActive; }

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
    
    ParticleSystem _particleSystem;
    float          _testRaveGloveClock;
    int            _testRaveGloveMode;
    bool           _particleSystemInitialized;
    int            _fingerParticleEmitter[NUM_FINGERS];
    Avatar*        _owningAvatar;
    float          _renderAlpha;
    bool           _lookingInMirror;
    bool           _isRaveGloveActive;
    glm::vec3      _ballColor;
    std::vector<HandBall> _leapBalls;
    
    // private methods
    void setLeapHands(const std::vector<glm::vec3>& handPositions,
                      const std::vector<glm::vec3>& handNormals);

    void renderRaveGloveStage();
    void setRaveGloveMode(int mode);
    void renderHandSpheres();
    void renderFingerTrails();
    void calculateGeometry();
};

#endif
