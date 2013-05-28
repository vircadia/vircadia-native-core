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
#include <Orientation.h>
#include "world.h"
#include "AvatarTouch.h"
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
    
    void reset();
    void simulate(float deltaTime, Transmitter* transmitter);
    void updateHeadFromGyros(float frametime, SerialInterface * serialInterface, glm::vec3 * gravity);
    void updateFromMouse(int mouseX, int mouseY, int screenWidth, int screenHeight);
    void addBodyYaw(float y) {_bodyYaw += y;};
    void render(bool lookingInMirror, glm::vec3 cameraPosition);

    //setters
    void setMousePressed           (bool      mousePressed           ) { _mousePressed    = mousePressed;} 
    void setNoise                  (float     mag                    ) { _head.noise      = mag;}
    void setMovedHandOffset        (glm::vec3 movedHandOffset        ) { _movedHandOffset = movedHandOffset;}
    void setThrust                 (glm::vec3 newThrust              ) { _thrust          = newThrust; };
    void setDisplayingLookatVectors(bool      displayingLookatVectors) { _head.setRenderLookatVectors(displayingLookatVectors);}
    void setGravity                (glm::vec3 gravity);
    void setMouseRay               (const glm::vec3 &origin, const glm::vec3 &direction);

    //getters
    float            getHeadYawRate           ()                const { return _head.yawRate;}
    float            getBodyYaw               ()                const { return _bodyYaw;}
    bool             getIsNearInteractingOther()                const { return _avatarTouch.getAbleToReachOtherAvatar();}
    const glm::vec3& getHeadPosition          ()                const { return _joint[ AVATAR_JOINT_HEAD_BASE ].position;}
    const glm::vec3& getSpringyHeadPosition   ()                const { return _joint[ AVATAR_JOINT_HEAD_BASE ].springyPosition;}
    const glm::vec3& getJointPosition         (AvatarJointID j) const { return _joint[j].springyPosition;} 
    const glm::vec3& getBodyUpDirection       ()                const { return _orientation.getUp();}
    const glm::vec3& getVelocity              ()                const { return _velocity;}
    float            getSpeed                 ()                const { return _speed;}
    float            getHeight                ()                const { return _height;}
    AvatarMode       getMode                  ()                const { return _mode;}
    float            getAbsoluteHeadYaw       () const;
    float            getAbsoluteHeadPitch     () const;
    Head&            getHead                  () {return _head; }
    
    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    
    //read/write avatar data
    void writeAvatarDataToFile();
    void readAvatarDataFromFile();

private:
    // privatize copy constructor and assignment operator to avoid copying
    Avatar(const Avatar&);
    Avatar& operator= (const Avatar&);

    struct AvatarJoint
    {
        AvatarJointID parent;               // which joint is this joint connected to?
        glm::vec3	  position;				// the position at the "end" of the joint - in global space
        glm::vec3	  defaultPosePosition;	// the parent relative position when the avatar is in the "T-pose"
        glm::vec3	  springyPosition;		// used for special effects (a 'flexible' variant of position)
        glm::vec3	  springyVelocity;		// used for special effects ( the velocity of the springy position)
        float		  springBodyTightness;	// how tightly the springy position tries to stay on the position
        glm::quat     rotation;             // this will eventually replace yaw, pitch and roll (and maybe orientation)
        float		  yaw;					// the yaw Euler angle of the joint rotation off the parent
        float		  pitch;				// the pitch Euler angle of the joint rotation off the parent
        float		  roll;					// the roll Euler angle of the joint rotation off the parent
        Orientation	  orientation;			// three orthogonal normals determined by yaw, pitch, roll
        float		  length;				// the length of vector connecting the joint and its parent
        float		  radius;               // used for detecting collisions for certain physical effects
        bool		  isCollidable;         // when false, the joint position will not register a collision
        float         touchForce;           // if being touched, what's the degree of influence? (0 to 1)
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
    AvatarJoint	_joint[ NUM_AVATAR_JOINTS ];
    AvatarMode  _mode;
    glm::vec3   _cameraPosition;
    glm::vec3   _handHoldingPosition;
    glm::vec3   _velocity;
    glm::vec3	_thrust;
    float       _speed;
    float		_maxArmLength;
    Orientation	_orientation;
    glm::quat   _righting;
    int         _driveKeys[MAX_DRIVE_KEYS];
    float       _pelvisStandingHeight;
    float       _pelvisFloatingHeight;
    float       _height;
    Balls*      _balls;
    AvatarTouch _avatarTouch;
    float       _distanceToNearestAvatar; //  How close is the nearest avatar?
    glm::vec3   _gravity;
    glm::vec3   _mouseRayOrigin;
    glm::vec3   _mouseRayDirection;
    Avatar*     _interactingOther;
    float       _cumulativeMouseYaw;
    bool        _isMouseTurningRight;
    
    // private methods...
    glm::vec3 caclulateAverageEyePosition() { return _head.caclulateAverageEyePosition(); } // get the position smack-dab between the eyes (for lookat)
    void renderBody(bool lookingInMirror);
    void initializeSkeleton();
    void updateSkeleton();
    void initializeBodySprings();
    void updateBodySprings( float deltaTime );
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
