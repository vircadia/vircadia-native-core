//
//  Avatar.h
//  interface
//
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__avatar__
#define __interface__avatar__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QSettings>

#include <AvatarData.h>

#include "AvatarTouch.h"
#include "AvatarVoxelSystem.h"
#include "Balls.h"
#include "Hand.h"
#include "Head.h"
#include "InterfaceConfig.h"
#include "Skeleton.h"
#include "SerialInterface.h"
#include "Transmitter.h"
#include "world.h"

const float BODY_BALL_RADIUS_PELVIS           = 0.07;
const float BODY_BALL_RADIUS_TORSO            = 0.065;
const float BODY_BALL_RADIUS_CHEST            = 0.08;
const float BODY_BALL_RADIUS_NECK_BASE        = 0.03;
const float BODY_BALL_RADIUS_HEAD_BASE        = 0.07;
const float BODY_BALL_RADIUS_LEFT_COLLAR      = 0.04;
const float BODY_BALL_RADIUS_LEFT_SHOULDER    = 0.03;
const float BODY_BALL_RADIUS_LEFT_ELBOW       = 0.02;
const float BODY_BALL_RADIUS_LEFT_WRIST       = 0.02;
const float BODY_BALL_RADIUS_LEFT_FINGERTIPS  = 0.01;
const float BODY_BALL_RADIUS_RIGHT_COLLAR     = 0.04;
const float BODY_BALL_RADIUS_RIGHT_SHOULDER   = 0.03;
const float BODY_BALL_RADIUS_RIGHT_ELBOW      = 0.02;
const float BODY_BALL_RADIUS_RIGHT_WRIST      = 0.02;
const float BODY_BALL_RADIUS_RIGHT_FINGERTIPS = 0.01;
const float BODY_BALL_RADIUS_LEFT_HIP         = 0.04;
const float BODY_BALL_RADIUS_LEFT_MID_THIGH   = 0.03;
const float BODY_BALL_RADIUS_LEFT_KNEE        = 0.025;
const float BODY_BALL_RADIUS_LEFT_HEEL        = 0.025;
const float BODY_BALL_RADIUS_LEFT_TOES        = 0.025;
const float BODY_BALL_RADIUS_RIGHT_HIP        = 0.04;
const float BODY_BALL_RADIUS_RIGHT_KNEE       = 0.025;
const float BODY_BALL_RADIUS_RIGHT_HEEL       = 0.025;
const float BODY_BALL_RADIUS_RIGHT_TOES       = 0.025;

enum AvatarBodyBallID
{
	BODY_BALL_NULL = -1,
	BODY_BALL_PELVIS,	
	BODY_BALL_TORSO,	
	BODY_BALL_CHEST,	
	BODY_BALL_NECK_BASE,	
	BODY_BALL_HEAD_BASE,	
	BODY_BALL_HEAD_TOP,	
	BODY_BALL_LEFT_COLLAR,
	BODY_BALL_LEFT_SHOULDER,
	BODY_BALL_LEFT_ELBOW,
	BODY_BALL_LEFT_WRIST,
	BODY_BALL_LEFT_FINGERTIPS,
	BODY_BALL_RIGHT_COLLAR,
	BODY_BALL_RIGHT_SHOULDER,
	BODY_BALL_RIGHT_ELBOW,
	BODY_BALL_RIGHT_WRIST,
	BODY_BALL_RIGHT_FINGERTIPS,
	BODY_BALL_LEFT_HIP,
	BODY_BALL_LEFT_KNEE,
	BODY_BALL_LEFT_HEEL,		
	BODY_BALL_LEFT_TOES,		
	BODY_BALL_RIGHT_HIP,	
	BODY_BALL_RIGHT_KNEE,	
	BODY_BALL_RIGHT_HEEL,	
	BODY_BALL_RIGHT_TOES,	
    
//TEST!     
//BODY_BALL_LEFT_MID_THIGH,	
	NUM_AVATAR_BODY_BALLS
};

enum DriveKeys
{
    FWD = 0,
    BACK,
    LEFT, 
    RIGHT, 
    UP,
    DOWN,
    ROT_LEFT, 
    ROT_RIGHT, 
    MAX_DRIVE_KEYS
};

enum AvatarMode
{
    AVATAR_MODE_STANDING = 0,
    AVATAR_MODE_WALKING,
    AVATAR_MODE_INTERACTING,
    NUM_AVATAR_MODES
};

class Avatar : public AvatarData {
public:
    Avatar(Node* owningNode = NULL);
    ~Avatar();
    
    void init();
    void reset();
    void simulate(float deltaTime, Transmitter* transmitter);
    void updateThrust(float deltaTime, Transmitter * transmitter);
    void updateFromGyrosAndOrWebcam(bool gyroLook,
                                    const glm::vec3& amplifyAngle,
                                    float yawFromTouch,
                                    float pitchFromTouch);
    void addBodyYaw(float bodyYaw) {_bodyYaw += bodyYaw;};
    void addBodyYawDelta(float bodyYawDelta) {_bodyYawDelta += bodyYawDelta;}
    void render(bool lookingInMirror, bool renderAvatarBalls);

    //setters
    void setMousePressed           (bool      mousePressed           ) { _mousePressed    = mousePressed;} 
    void setNoise                  (float     mag                    ) { _head.noise      = mag;}
    void setMovedHandOffset        (glm::vec3 movedHandOffset        ) { _movedHandOffset = movedHandOffset;}
    void setThrust                 (glm::vec3 newThrust              ) { _thrust          = newThrust; };
    void setDisplayingLookatVectors(bool      displayingLookatVectors) { _head.setRenderLookatVectors(displayingLookatVectors);}
    void setVelocity               (const glm::vec3 velocity         ) { _velocity = velocity; };
    void setLeanScale              (float     scale                  ) { _leanScale = scale;}
    void setGravity                (glm::vec3 gravity);
    void setMouseRay               (const glm::vec3 &origin, const glm::vec3 &direction);
    void setOrientation            (const glm::quat& orientation);
    void setScale                  (const float scale);

    //getters
    bool             isInitialized             ()                const { return _initialized;}
    bool             isMyAvatar                ()                const { return _owningNode == NULL; }
    const Skeleton&  getSkeleton               ()                const { return _skeleton;}
    float            getHeadYawRate            ()                const { return _head.yawRate;}
    float            getBodyYaw                ()                const { return _bodyYaw;}    
    bool             getIsNearInteractingOther ()                const { return _avatarTouch.getAbleToReachOtherAvatar();}
    const glm::vec3& getHeadJointPosition      ()                const { return _skeleton.joint[ AVATAR_JOINT_HEAD_BASE ].position;}
    const glm::vec3& getBallPosition           (AvatarJointID j) const { return _bodyBall[j].position;}
    glm::vec3        getBodyRightDirection     ()                const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3        getBodyUpDirection        ()                const { return getOrientation() * IDENTITY_UP; }
    glm::vec3        getBodyFrontDirection     ()                const { return getOrientation() * IDENTITY_FRONT; }
    float            getScale                  ()                const { return _scale;}
    const glm::vec3& getVelocity               ()                const { return _velocity;}
    float            getSpeed                  ()                const { return _speed;}
    float            getHeight                 ()                const { return _height;}
    AvatarMode       getMode                   ()                const { return _mode;}
    float            getLeanScale              ()                const { return _leanScale;}
    float            getElapsedTimeStopped     ()                const { return _elapsedTimeStopped;}
    float            getElapsedTimeMoving      ()                const { return _elapsedTimeMoving;}
    float            getElapsedTimeSinceCollision()              const { return _elapsedTimeSinceCollision;}
    const glm::vec3& getLastCollisionPosition  ()                const { return _lastCollisionPosition;}
    float            getAbsoluteHeadYaw        () const;
    float            getAbsoluteHeadPitch      () const;
    Head&            getHead                   () {return _head; }
    Hand&            getHand                   () {return _hand; }
    glm::quat        getOrientation            () const;
    glm::quat        getWorldAlignedOrientation() const;
    
    const glm::vec3& getMouseRayOrigin()    const { return _mouseRayOrigin;    }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }

    
    glm::vec3        getGravity        ()         const { return _gravity; }
    
    glm::vec3 getUprightHeadPosition() const;
    glm::vec3 getUprightEyeLevelPosition() const;
    
    AvatarVoxelSystem* getVoxels() { return &_voxels; }
    
    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };
    void jump() { _shouldJump = true; };

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    
    // get/set avatar data
    void saveData(QSettings* set);
    void loadData(QSettings* set);

    //  Get the position/rotation of a single body ball
    void getBodyBallTransform(AvatarJointID jointID, glm::vec3& position, glm::quat& rotation) const;

    static void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);

private:
    // privatize copy constructor and assignment operator to avoid copying
    Avatar(const Avatar&);
    Avatar& operator= (const Avatar&);
    
    struct AvatarBall
    {
        AvatarJointID    parentJoint;    // the skeletal joint that serves as a reference for determining the position
        glm::vec3        parentOffset;   // a 3D vector in the frame of reference of the parent skeletal joint
        AvatarBodyBallID parentBall;     // the ball to which this ball is constrained for spring forces 
        glm::vec3        position;       // the actual dynamic position of the ball at any given time
        glm::quat        rotation;       // the rotation of the ball           
        glm::vec3        velocity;       // the velocity of the ball
        float            springLength;   // the ideal length of the spring between this ball and its parentBall 
        float            jointTightness; // how tightly the ball position attempts to stay at its ideal position (determined by parentOffset)
        float            radius;         // the radius of the ball
        bool             isCollidable;   // whether or not the ball responds to collisions 
        float            touchForce;     // a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };

    bool        _initialized;
    Head        _head;
    Hand        _hand;
    Skeleton    _skeleton;
    bool        _ballSpringsInitialized;
    float       _TEST_bigSphereRadius;
    glm::vec3   _TEST_bigSpherePosition;
    bool        _mousePressed;
    float       _bodyPitchDelta;
    float       _bodyYawDelta;
    float       _bodyRollDelta;
    glm::vec3   _movedHandOffset;
    AvatarBall	_bodyBall[ NUM_AVATAR_BODY_BALLS ];
    AvatarMode  _mode;
    glm::vec3   _handHoldingPosition;
    glm::vec3   _velocity;
    glm::vec3   _thrust;
    bool        _shouldJump;
    float       _speed;
    float       _maxArmLength;
    float       _leanScale;
    int         _driveKeys[MAX_DRIVE_KEYS];
    float       _pelvisStandingHeight;
    float       _pelvisFloatingHeight;
    float       _pelvisToHeadLength;
    float       _scale;
    float       _height;
    Balls*      _balls;
    AvatarTouch _avatarTouch;
    float       _distanceToNearestAvatar;       //  How close is the nearest avatar?
    glm::vec3   _gravity;
    glm::vec3   _worldUpDirection;
    glm::vec3   _mouseRayOrigin;
    glm::vec3   _mouseRayDirection;
    Avatar*     _interactingOther;
    bool        _isMouseTurningRight;
    float       _elapsedTimeMoving;             //  Timers to drive camera transitions when moving
    float       _elapsedTimeStopped;
    float       _elapsedTimeSinceCollision;
    glm::vec3   _lastCollisionPosition;
    bool        _speedBrakes;
    bool        _isThrustOn;
    
    AvatarVoxelSystem _voxels;
    
    // private methods...
    glm::vec3 calculateAverageEyePosition() { return _head.calculateAverageEyePosition(); } // get the position smack-dab between the eyes (for lookat)
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    float getBallRenderAlpha(int ball, bool lookingInMirror) const;
    void renderBody(bool lookingInMirror, bool renderAvatarBalls);
    void initializeBodyBalls();
    void resetBodyBalls();
    void updateBodyBalls( float deltaTime );
    void calculateBoneLengths();
    void readSensors();
    void updateHandMovementAndTouching(float deltaTime, bool enableHandMovement);
    void updateAvatarCollisions(float deltaTime);
    void updateArmIKAndConstraints( float deltaTime );
    void updateCollisionWithSphere( glm::vec3 position, float radius, float deltaTime );
    void updateCollisionWithEnvironment(float deltaTime);
    void updateCollisionWithVoxels(float deltaTime);
    void applyHardCollision(const glm::vec3& penetration, float elasticity, float damping);
    void updateCollisionSound(const glm::vec3& penetration, float deltaTime, float frequency);
    void applyCollisionWithOtherAvatar( Avatar * other, float deltaTime );
    void checkForMouseRayTouching();
};

#endif
