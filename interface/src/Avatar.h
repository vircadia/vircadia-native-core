//
//  Avatar.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
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
#include "AvatarRenderer.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include "Balls.h"
#include "Head.h"
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

enum AvatarJointID
{
	AVATAR_JOINT_NULL = -1,
	AVATAR_JOINT_PELVIS,	
	AVATAR_JOINT_TORSO,	
	AVATAR_JOINT_CHEST,	
	AVATAR_JOINT_NECK_BASE,	
	AVATAR_JOINT_HEAD_BASE,	
	AVATAR_JOINT_HEAD_TOP,	
	AVATAR_JOINT_LEFT_COLLAR,
	AVATAR_JOINT_LEFT_SHOULDER,
	AVATAR_JOINT_LEFT_ELBOW,
	AVATAR_JOINT_LEFT_WRIST,
	AVATAR_JOINT_LEFT_FINGERTIPS,
	AVATAR_JOINT_RIGHT_COLLAR,
	AVATAR_JOINT_RIGHT_SHOULDER,
	AVATAR_JOINT_RIGHT_ELBOW,
	AVATAR_JOINT_RIGHT_WRIST,
	AVATAR_JOINT_RIGHT_FINGERTIPS,
	AVATAR_JOINT_LEFT_HIP,
	AVATAR_JOINT_LEFT_KNEE,
	AVATAR_JOINT_LEFT_HEEL,		
	AVATAR_JOINT_LEFT_TOES,		
	AVATAR_JOINT_RIGHT_HIP,	
	AVATAR_JOINT_RIGHT_KNEE,	
	AVATAR_JOINT_RIGHT_HEEL,	
	AVATAR_JOINT_RIGHT_TOES,	
    
	NUM_AVATAR_JOINTS
};


class Avatar : public AvatarData {
public:
    Avatar(bool isMine);
    ~Avatar();
    
    void  reset();
    void  updateHeadFromGyros(float frametime, SerialInterface * serialInterface, glm::vec3 * gravity);
    void  updateFromMouse(int mouseX, int mouseY, int screenWidth, int screenHeight);
    void  setNoise (float mag) {_head.noise = mag;}
    float getLastMeasuredHeadYaw() const {return _head.yawRate;}
    float getBodyYaw() {return _bodyYaw;};
    void  addBodyYaw(float y) {_bodyYaw += y;};
    void  setGravity(glm::vec3 gravity);

    void  setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction );
    bool  getIsNearInteractingOther();
        
    float getAbsoluteHeadYaw() const;
    float getAbsoluteHeadPitch() const;
    glm::vec3 getApproximateEyePosition(); 
    const glm::vec3& getHeadPosition() const ;          // get the position of the avatar's rigid body head
    const glm::vec3& getSpringyHeadPosition() const ;   // get the springy position of the avatar's head
    const glm::vec3& getJointPosition(AvatarJointID j) const { return _joint[j].springyPosition; }; 
    const glm::vec3& getBodyUpDirection() const { return _orientation.getUp(); };
    float getSpeed() const { return _speed; }
    const glm::vec3& getVelocity() const { return _velocity; };
    float getGirth();
    float getHeight() const { return _height; }
    
    AvatarMode getMode() const { return _mode; }
    Head getHead() const { return _head; }
    
    void setMousePressed(bool pressed); 
    void render(bool lookingInMirror, glm::vec3 cameraPosition);
    void renderBody(bool lookingInMirror);
    void simulate(float deltaTime, Transmitter* transmitter);
    void setMovedHandOffset(glm::vec3 movedHandOffset) { _movedHandOffset = movedHandOffset; }
    void updateArmIKAndConstraints( float deltaTime );
    void setDisplayingHead( bool displayingHead );
    
    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };

    //  Set/Get update the thrust that will move the avatar around
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; };
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    
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
    bool        _isMine;
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
    glm::vec3   _handHoldingPosition;
    glm::vec3   _velocity;
    glm::vec3	_thrust;
    float       _speed;
    float		_maxArmLength;
    Orientation	_orientation;
    int         _driveKeys[MAX_DRIVE_KEYS];
    float       _pelvisStandingHeight;
    float       _height;
    Balls*      _balls;
    AvatarTouch _avatarTouch;
    bool        _displayingHead; // should be false if in first-person view
    float       _distanceToNearestAvatar; //  How close is the nearest avatar?
    glm::vec3   _gravity;
    glm::vec3   _mouseRayOrigin;
    glm::vec3   _mouseRayDirection;
    glm::vec3   _cameraPosition;
    Avatar*     _interactingOther;
    float       _cumulativeMouseYaw;
    bool        _isMouseTurningRight;
    
    // private methods...
    void initializeSkeleton();
    void updateSkeleton();
    void initializeBodySprings();
    void updateBodySprings( float deltaTime );
    void calculateBoneLengths();
    void readSensors();
    void updateHandMovementAndTouching(float deltaTime);
    void updateAvatarCollisions(float deltaTime);
    void updateCollisionWithSphere( glm::vec3 position, float radius, float deltaTime );
    void updateCollisionWithVoxels(float deltaTime);
    void applyCollisionWithOtherAvatar( Avatar * other, float deltaTime );
    void setHeadFromGyros(glm::vec3 * eulerAngles, glm::vec3 * angularVelocity, float deltaTime, float smoothingTime);
    void checkForMouseRayTouching();
    void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);
};

#endif
