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
#include <AudioScriptingInterface.h>
#include <HandData.h>

#include "InterfaceConfig.h"
#include "world.h"
#include "VoxelSystem.h"


class Avatar;
class ProgramObject;

const float HAND_PADDLE_OFFSET = 0.1f;
const float HAND_PADDLE_THICKNESS = 0.01f;
const float HAND_PADDLE_RADIUS = 0.15f;

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

    // getters
    const glm::vec3& getLeapFingerTipBallPosition (int ball) const { return _leapFingerTipBalls [ball].position;}
    const glm::vec3& getLeapFingerRootBallPosition(int ball) const { return _leapFingerRootBalls[ball].position;}
    
    void collideAgainstAvatarOld(Avatar* avatar, bool isMyHand);
    void collideAgainstAvatar(Avatar* avatar, bool isMyHand);
    void collideAgainstOurself();

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
        
    int _controllerButtons;             ///  Button states read from hand-held controllers

    Avatar*        _owningAvatar;
    float          _renderAlpha;
    std::vector<HandBall> _leapFingerTipBalls;
    std::vector<HandBall> _leapFingerRootBalls;
    
    glm::vec3 _lastFingerAddVoxel, _lastFingerDeleteVoxel;
    VoxelDetail _collidingVoxel;
    
    glm::vec3 _collisionCenter;
    float _collisionAge;
    float _collisionDuration;
    
    void renderLeapHands(bool isMine);
    void renderLeapFingerTrails();
    
    void calculateGeometry();
    
    void handleVoxelCollision(PalmData* palm, const glm::vec3& fingerTipPosition, VoxelTreeElement* voxel, float deltaTime);

    void playSlaps(PalmData& palm, Avatar* avatar);
};

#endif
