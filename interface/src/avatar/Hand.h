//
//  Hand.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Hand_h
#define hifi_Hand_h

#include <vector>

#include <QAction>

#include <glm/glm.hpp>

#include <SharedUtil.h>

#include <AvatarData.h>
#include <AudioInjector.h>
#include <HandData.h>
#include <ParticleEditHandle.h>

#include "InterfaceConfig.h"
#include "ParticleSystem.h"
#include "world.h"
#include "devices/SerialInterface.h"
#include "VoxelSystem.h"


class Avatar;
class ProgramObject;

const int NUM_BBALLS = 200;


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
        bool             isColliding;    // ball is currently colliding
        float            touchForce;     // a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };
    
    void init();
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(bool isMine);
    void setBallColor      (glm::vec3 ballColor         ) { _ballColor          = ballColor;          }

    // getters
    const glm::vec3& getLeapFingerTipBallPosition (int ball) const { return _leapFingerTipBalls [ball].position;}
    const glm::vec3& getLeapFingerRootBallPosition(int ball) const { return _leapFingerRootBalls[ball].position;}
    
    // Pitch from controller input to view
    const float getPitchUpdate() const { return _pitchUpdate; }
    void setPitchUpdate(float pitchUpdate) { _pitchUpdate = pitchUpdate; }
    
    // Get the drag distance to move
    glm::vec3 getAndResetGrabDelta();
    glm::vec3 getAndResetGrabDeltaVelocity();
    glm::quat getAndResetGrabRotation();

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
        
    int _controllerButtons;             ///  Button states read from hand-held controllers

    Avatar*        _owningAvatar;
    float          _renderAlpha;
    glm::vec3      _ballColor;
    std::vector<HandBall> _leapFingerTipBalls;
    std::vector<HandBall> _leapFingerRootBalls;
    
    glm::vec3 _lastFingerAddVoxel, _lastFingerDeleteVoxel;
    VoxelDetail _collidingVoxel;
    
    glm::vec3 _collisionCenter;
    float _collisionAge;
    float _collisionDuration;
    
    // private methods
    void setLeapHands(const std::vector<glm::vec3>& handPositions,
                      const std::vector<glm::vec3>& handNormals);

    void renderLeapHands(bool isMine);
    void renderLeapFingerTrails();
    
    void updateCollisions();
    void calculateGeometry();
    
    void handleVoxelCollision(PalmData* palm, const glm::vec3& fingerTipPosition, VoxelTreeElement* voxel, float deltaTime);
    
    void simulateToyBall(PalmData& palm, const glm::vec3& fingerTipPosition, float deltaTime);
    void grabBuckyBalls(PalmData& palm, const glm::vec3& fingerTipPosition, float deltaTime);

    void simulateBuckyBalls(float deltaTime);
    void renderBuckyBalls();
    
    #define MAX_HANDS 2
    bool _toyBallInHand[MAX_HANDS];
    int _whichBallColor[MAX_HANDS];
    ParticleEditHandle* _ballParticleEditHandles[MAX_HANDS];
    int _lastControllerButtons;
    
    float _pitchUpdate;
    
    glm::vec3 _grabDelta;
    glm::vec3 _grabDeltaVelocity;
    glm::quat _grabStartRotation;
    glm::quat _grabCurrentRotation;
    
    AudioInjector _throwInjector;
    AudioInjector _catchInjector;
    
    glm::vec3 _bballPosition[NUM_BBALLS];
    glm::vec3 _bballVelocity[NUM_BBALLS];
    glm::vec3 _bballColor[NUM_BBALLS];
    float _bballRadius[NUM_BBALLS];
    float _bballColliding[NUM_BBALLS];
    int _bballElement[NUM_BBALLS];
    int _bballIsGrabbed[2];
    
};

#endif
