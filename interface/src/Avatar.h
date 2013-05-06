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
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include "Balls.h"

enum eyeContactTargets {LEFT_EYE, RIGHT_EYE, MOUTH};

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
    Avatar(const Avatar &otherAvatar);
    Avatar* clone() const;

    void  reset();
    void  UpdateGyros(float frametime, SerialInterface * serialInterface, glm::vec3 * gravity);
   
    void  setNoise (float mag) {_head.noise = mag;}
    void  setScale(float s) {_head.scale = s; };
    void  setRenderYaw(float y) {_renderYaw = y;}
    void  setRenderPitch(float p) {_renderPitch = p;}
    float getRenderYaw() {return _renderYaw;}
    float getRenderPitch() {return _renderPitch;}
    float getLastMeasuredHeadYaw() const {return _head.yawRate;}
    float getBodyYaw() {return _bodyYaw;};
    void  addBodyYaw(float y) {_bodyYaw += y;};
    void  setGravity(glm::vec3 gravity);
    
    bool  getIsNearInteractingOther();
        
    float getAbsoluteHeadYaw() const;
    void  setLeanForward(float dist);
    void  setLeanSideways(float dist);
    void  addLean(float x, float z);

    /*
    const glm::vec3& getHeadRightDirection() const { return _orientation.getRight(); };
    const glm::vec3& getHeadUpDirection   () const { return _orientation.getUp   (); };
    const glm::vec3& getHeadFrontDirection() const { return _orientation.getFront(); };
    */
    
    const glm::vec3& getHeadPosition() const ;
    const glm::vec3& getJointPosition(AvatarJointID j) const { return _joint[j].position; };
    const glm::vec3& getBodyUpDirection() const { return _orientation.getUp(); };
    float getSpeed() const { return _speed; };
    float getGirth();
    float getHeight();
    
    AvatarMode getMode();
    
    void setMousePressed( bool pressed ); 
    void render(bool lookingInMirror);
    void renderBody();
    void renderHead(bool lookingInMirror);
    void simulate(float);
    void setHandMovementValues( glm::vec3 movement );
    void updateArmIKAndConstraints( float deltaTime );
    void setDisplayingHead( bool displayingHead );
    
    float getAverageLoudness() {return _head.averageLoudness;};
    void  setAverageLoudness(float al) {_head.averageLoudness = al;};
     
    void SetNewHeadTarget(float, float);

    //  Set what driving keys are being pressed to control thrust levels
    void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key]; };

    //  Set/Get update the thrust that will move the avatar around
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; };
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };

    //  Related to getting transmitter UDP data used to animate the avatar hand
    void processTransmitterData(unsigned char * packetData, int numBytes);
    float getTransmitterHz() { return _transmitterHz; };
    
    void writeAvatarDataToFile();
    void readAvatarDataFromFile();

private:

    //  Do you want head to try to return to center (depends on interface detected)
    void setHeadReturnToCenter(bool r) { _returnHeadToCenter = r; };
    const bool getHeadReturnToCenter() const { return _returnHeadToCenter; };

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
    };

    struct AvatarHead
    {
        float pitchRate;
        float yawRate;
        float rollRate;
        float noise;
        float eyeballPitch[2];
        float eyeballYaw  [2];
        float eyebrowPitch[2];
        float eyebrowRoll [2];
        float eyeballScaleX;
        float eyeballScaleY;
        float eyeballScaleZ;
        float interPupilDistance;
        float interBrowDistance;
        float nominalPupilSize;
        float pupilSize;
        float mouthPitch;
        float mouthYaw;
        float mouthWidth;
        float mouthHeight;
        float leanForward;
        float leanSideways;
        float pitchTarget; 
        float yawTarget; 
        float noiseEnvelope;
        float pupilConverge;
        float scale;
        int   eyeContact;
        float browAudioLift;
        eyeContactTargets eyeContactTarget;
        
        //  Sound loudness information
        float lastLoudness;
        float averageLoudness;
        float audioAttack;
        
        //  Strength of return springs
        float returnSpringScale;
    };

    AvatarHead  _head;
    bool        _isMine;
    glm::vec3   _TEST_bigSpherePosition;
    float       _TEST_bigSphereRadius;
    bool        _mousePressed;
    float       _bodyPitchDelta;
    float       _bodyYawDelta;
    float       _bodyRollDelta;
    bool        _usingBodySprings;
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
    GLUquadric* _sphere;
    float       _renderYaw;
    float       _renderPitch; //   Pitch from view frustum when this is own head
    bool        _transmitterIsFirstData; 
    timeval     _transmitterTimeLastReceived;
    timeval     _transmitterTimer;
    float       _transmitterHz;
    int         _transmitterPackets;
    glm::vec3   _transmitterInitialReading;
    //Avatar*     _interactingOther;
    float       _pelvisStandingHeight;
    float       _height;
    Balls*      _balls;
    AvatarTouch _avatarTouch;
    bool        _displayingHead; // should be false if in first-person view
    bool        _returnHeadToCenter;
    float       _distanceToNearestAvatar;    //  How close is the nearest avatar?
    glm::vec3   _gravity;

    // private methods...
    void initializeSkeleton();
    void updateSkeleton();
    void initializeBodySprings();
    void updateBodySprings( float deltaTime );
    void calculateBoneLengths();
    void readSensors();
    void updateHead( float deltaTime );
    void updateHandMovementAndTouching(float deltaTime);
    void updateAvatarCollisions(float deltaTime);
    void updateCollisionWithSphere( glm::vec3 position, float radius, float deltaTime );
    void applyCollisionWithOtherAvatar( Avatar * other, float deltaTime );
    void setHeadFromGyros(glm::vec3 * eulerAngles, glm::vec3 * angularVelocity, float deltaTime, float smoothingTime);
    void setHeadSpringScale(float s) { _head.returnSpringScale = s; }
};

#endif
