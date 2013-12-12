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
#include <HandData.h>
#include <ParticleEditHandle.h>

#include "Balls.h"
#include "InterfaceConfig.h"
#include "ParticleSystem.h"
#include "world.h"
#include "devices/SerialInterface.h"
#include "VoxelSystem.h"


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
    bool _isCollidingWithVoxel;
    VoxelDetail _collidingVoxel;
    
    glm::vec3 _collisionCenter;
    float _collisionAge;
    float _collisionDuration;
    
    // private methods
    void setLeapHands(const std::vector<glm::vec3>& handPositions,
                      const std::vector<glm::vec3>& handNormals);

    void renderLeapHands();
    void renderLeapFingerTrails();
    
    void updateCollisions();
    void calculateGeometry();
    
    void handleVoxelCollision(PalmData* palm, const glm::vec3& fingerTipPosition, VoxelTreeElement* voxel, float deltaTime);
    
    void simulateToyBall(PalmData& palm, const glm::vec3& fingerTipPosition, float deltaTime);
    
    glm::vec3 _toyBallPosition;
    glm::vec3 _toyBallVelocity;
    bool _toyBallInHand;
    bool _hasToyBall;
    ParticleEditHandle* _ballParticleEditHandle;
    
    float _pitchUpdate;

};

#endif
