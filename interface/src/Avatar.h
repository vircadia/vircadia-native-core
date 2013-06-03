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
#include <AvatarData.h>
#include "world.h"
#include "AvatarTouch.h"
#include "AvatarVoxelSystem.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include "Balls.h"
#include "Head.h"
#include "Skeleton.h"
#include "Transmitter.h"

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
    Avatar(Agent* owningAgent = NULL);
    ~Avatar();
    
    void init();
    void reset();
    void simulate(float deltaTime, Transmitter* transmitter);
    void updateHeadFromGyros(float frametime, SerialInterface * serialInterface);
    void updateFromMouse(int mouseX, int mouseY, int screenWidth, int screenHeight);
    void addBodyYaw(float y) {_bodyYaw += y;};
    void render(bool lookingInMirror);

    //setters
    void setMousePressed           (bool      mousePressed           ) { _mousePressed    = mousePressed;} 
    void setNoise                  (float     mag                    ) { _head.noise      = mag;}
    void setMovedHandOffset        (glm::vec3 movedHandOffset        ) { _movedHandOffset = movedHandOffset;}
    void setThrust                 (glm::vec3 newThrust              ) { _thrust          = newThrust; };
    void setDisplayingLookatVectors(bool      displayingLookatVectors) { _head.setRenderLookatVectors(displayingLookatVectors);}
    void setGravity                (glm::vec3 gravity);
    void setMouseRay               (const glm::vec3 &origin, const glm::vec3 &direction);
    void setOrientation            (const glm::quat& orientation);

    //getters
    const Skeleton&  getSkeleton              ()                const { return _skeleton;}
    float            getHeadYawRate           ()                const { return _head.yawRate;}
    float            getBodyYaw               ()                const { return _bodyYaw;}    
    bool             getIsNearInteractingOther()                const { return _avatarTouch.getAbleToReachOtherAvatar();}
    const glm::vec3& getHeadPosition          ()                const { return _skeleton.joint[ AVATAR_JOINT_HEAD_BASE ].position;}
    const glm::vec3& getSpringyHeadPosition   ()                const { return _bodyBall[ AVATAR_JOINT_HEAD_BASE ].position;}
    const glm::vec3& getJointPosition         (AvatarJointID j) const { return _bodyBall[j].position;} 

    glm::vec3        getBodyRightDirection      ()                const { return getOrientation() * AVATAR_RIGHT; }
    glm::vec3        getBodyUpDirection         ()                const { return getOrientation() * AVATAR_UP; }
    glm::vec3        getBodyFrontDirection      ()                const { return getOrientation() * AVATAR_FRONT; }


    const glm::vec3& getVelocity               ()                const { return _velocity;}
    float            getSpeed                  ()                const { return _speed;}
    float            getHeight                 ()                const { return _height;}
    AvatarMode       getMode                   ()                const { return _mode;}
    float            getAbsoluteHeadYaw        () const;
    float            getAbsoluteHeadPitch      () const;
    Head&            getHead                   () {return _head; }
    glm::quat        getOrientation            () const;
    glm::quat        getWorldAlignedOrientation() const;
    
    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    
    //  Get the position/rotation of a single body ball
    void getBodyBallTransform(AvatarJointID jointID, glm::vec3& position, glm::quat& rotation) const;
    
    //read/write avatar data
    void writeAvatarDataToFile();
    void readAvatarDataFromFile();

private:
    // privatize copy constructor and assignment operator to avoid copying
    Avatar(const Avatar&);
    Avatar& operator= (const Avatar&);

    struct AvatarBall
    {
        glm::vec3 position;
        glm::quat rotation;      
        glm::vec3 velocity;      
        float     jointTightness;  
        float     radius;               
        bool      isCollidable;         
        float     touchForce;           
    };

    Head        _head;
    Skeleton    _skeleton;
    float       _TEST_bigSphereRadius;
    glm::vec3   _TEST_bigSpherePosition;
    bool        _mousePressed;
    float       _bodyPitchDelta;
    float       _bodyYawDelta;
    float       _bodyRollDelta;
    glm::vec3   _movedHandOffset;
    glm::quat   _rotation; // the rotation of the avatar body as a whole expressed as a quaternion
    AvatarBall	_bodyBall[ NUM_AVATAR_JOINTS ];
    AvatarMode  _mode;
    glm::vec3   _cameraPosition;
    glm::vec3   _handHoldingPosition;
    glm::vec3   _velocity;
    glm::vec3   _thrust;
    float       _speed;
    float       _maxArmLength;
    glm::quat   _righting;
    int         _driveKeys[MAX_DRIVE_KEYS];
    float       _pelvisStandingHeight;
    float       _pelvisFloatingHeight;
    float       _height;
    Balls*      _balls;
    AvatarTouch _avatarTouch;
    float       _distanceToNearestAvatar; //  How close is the nearest avatar?
    glm::vec3   _gravity;
    glm::vec3   _worldUpDirection;
    glm::vec3   _mouseRayOrigin;
    glm::vec3   _mouseRayDirection;
    Avatar*     _interactingOther;
    float       _cumulativeMouseYaw;
    bool        _isMouseTurningRight;
    
    AvatarVoxelSystem _voxels;
    
    // private methods...
    glm::vec3 caclulateAverageEyePosition() { return _head.caclulateAverageEyePosition(); } // get the position smack-dab between the eyes (for lookat)
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void renderBody(bool lookingInMirror);
    void initializeBodyBalls();
    void resetBodyBalls();
    void updateBodyBalls( float deltaTime );
    void calculateBoneLengths();
    void readSensors();
    void updateHandMovementAndTouching(float deltaTime);
    void updateAvatarCollisions(float deltaTime);
    void updateArmIKAndConstraints( float deltaTime );
    void updateCollisionWithSphere( glm::vec3 position, float radius, float deltaTime );
    void updateCollisionWithEnvironment();
    void updateCollisionWithVoxels();
    void applyCollisionWithScene(const glm::vec3& penetration);
    void applyCollisionWithOtherAvatar( Avatar * other, float deltaTime );
    void setHeadFromGyros(glm::vec3 * eulerAngles, glm::vec3 * angularVelocity, float deltaTime, float smoothingTime);
    void checkForMouseRayTouching();
    void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);
};

#endif
