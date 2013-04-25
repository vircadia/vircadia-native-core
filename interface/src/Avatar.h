//
//  Avatar.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__avatar__
#define __interface__avatar__

#include <AvatarData.h>
#include <Orientation.h>

#include "Field.h"
#include "world.h"

#include "InterfaceConfig.h"
#include "SerialInterface.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> //looks like we might not need this

const bool  AVATAR_GRAVITY  = true;
const float DECAY           = 0.1;
const float THRUST_MAG      = 10.0;
const float YAW_MAG         = 300.0;
const float TEST_YAW_DECAY  = 5.0;
const float LIN_VEL_DECAY   = 5.0;

const float COLLISION_BODY_RADIUS = 0.1;
const float COLLISION_HEIGHT = 1.5;

enum eyeContactTargets {LEFT_EYE, RIGHT_EYE, MOUTH};

#define FWD 0
#define BACK 1 
#define LEFT 2 
#define RIGHT 3 
#define UP 4 
#define DOWN 5
#define ROT_LEFT 6 
#define ROT_RIGHT 7 
#define MAX_DRIVE_KEYS 8

#define MAX_OTHER_AVATARS 10 // temporary - for testing purposes!

enum AvatarMode
{
	AVATAR_MODE_STANDING = 0,
	AVATAR_MODE_WALKING,
	AVATAR_MODE_INTERACTING,
	NUM_AVATAR_MODES
};

enum AvatarBoneID
{
	AVATAR_BONE_NULL = -1,
	AVATAR_BONE_PELVIS_SPINE,		// connects pelvis			joint with torso			joint (not supposed to be rotated)
	AVATAR_BONE_MID_SPINE,			// connects torso			joint with chest			joint
	AVATAR_BONE_CHEST_SPINE,		// connects chest			joint with neckBase			joint (not supposed to be rotated)
	AVATAR_BONE_NECK,				// connects neckBase		joint with headBase			joint
	AVATAR_BONE_HEAD,				// connects headBase		joint with headTop			joint
	AVATAR_BONE_LEFT_CHEST,			// connects chest			joint with left clavicle	joint (not supposed to be rotated)
	AVATAR_BONE_LEFT_SHOULDER,		// connects left clavicle	joint with left shoulder	joint
	AVATAR_BONE_LEFT_UPPER_ARM,		// connects left shoulder	joint with left elbow		joint
	AVATAR_BONE_LEFT_FOREARM,		// connects left elbow		joint with left wrist		joint
	AVATAR_BONE_LEFT_HAND,			// connects left wrist		joint with left fingertips	joint
	AVATAR_BONE_RIGHT_CHEST,		// connects chest			joint with right clavicle	joint (not supposed to be rotated)
	AVATAR_BONE_RIGHT_SHOULDER,		// connects right clavicle	joint with right shoulder	joint
	AVATAR_BONE_RIGHT_UPPER_ARM,	// connects right shoulder	joint with right elbow		joint
	AVATAR_BONE_RIGHT_FOREARM,		// connects right elbow		joint with right wrist		joint
	AVATAR_BONE_RIGHT_HAND,			// connects right wrist		joint with right fingertips	joint
	AVATAR_BONE_LEFT_PELVIS,		// connects pelvis			joint with left hip			joint (not supposed to be rotated)
	AVATAR_BONE_LEFT_THIGH,			// connects left hip		joint with left knee		joint
	AVATAR_BONE_LEFT_SHIN,			// connects left knee		joint with left heel		joint
	AVATAR_BONE_LEFT_FOOT,			// connects left heel		joint with left toes		joint
	AVATAR_BONE_RIGHT_PELVIS,		// connects pelvis			joint with right hip		joint (not supposed to be rotated)
	AVATAR_BONE_RIGHT_THIGH,		// connects right hip		joint with right knee		joint
	AVATAR_BONE_RIGHT_SHIN,			// connects right knee		joint with right heel		joint
	AVATAR_BONE_RIGHT_FOOT,			// connects right heel		joint with right toes		joint
    
	NUM_AVATAR_BONES
};

struct AvatarCollisionElipsoid
{
    bool      colliding;
    glm::vec3 position;
    float     girth;
    float     height;
    glm::vec3 upVector;
};

struct AvatarHandHolding
{
    glm::vec3 position;
    glm::vec3 velocity;
    float     force;
};

struct AvatarBone
{
	AvatarBoneID parent;				// which bone is this bone connected to?
	glm::vec3	 position;				// the position at the "end" of the bone
	glm::vec3	 defaultPosePosition;	// the parent relative position when the avatar is in the "T-pose"
	glm::vec3	 springyPosition;		// used for special effects (a 'flexible' variant of position)
	glm::dvec3	 springyVelocity;		// used for special effects ( the velocity of the springy position)
	float		 springBodyTightness;	// how tightly the springy position tries to stay on the position
    glm::quat    rotation;              // this will eventually replace yaw, pitch and roll (and maybe orientation)
	float		 yaw;					// the yaw Euler angle of the bone rotation off the parent
	float		 pitch;					// the pitch Euler angle of the bone rotation off the parent
	float		 roll;					// the roll Euler angle of the bone rotation off the parent
	Orientation	 orientation;			// three orthogonal normals determined by yaw, pitch, roll
	float		 length;				// the length of the bone
	float		 radius;                // used for detecting collisions for certain physical effects
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
};


class Avatar : public AvatarData {
    public:
        Avatar(bool isMine);
        ~Avatar();
        Avatar(const Avatar &otherAvatar);
        Avatar* clone() const;
    
        void  reset();
        void  UpdateGyros(float frametime, SerialInterface * serialInterface, glm::vec3 * gravity);
        void  setNoise (float mag) { _head.noise = mag; }
        void  setScale(float s) {_head.scale = s; };
        void  setRenderYaw(float y) {_renderYaw = y;}
        void  setRenderPitch(float p) {_renderPitch = p;}
        float getRenderYaw() {return _renderYaw;}
        float getRenderPitch() {return _renderPitch;}
        void  setLeanForward(float dist);
        void  setLeanSideways(float dist);
        void  addLean(float x, float z);
        float getLastMeasuredHeadYaw() const {return _head.yawRate;}
        float getBodyYaw() {return _bodyYaw;};
        void  addBodyYaw(float y) {_bodyYaw += y;};
    
        const glm::vec3& getHeadLookatDirection() const { return _orientation.getFront(); };
        const glm::vec3& getHeadLookatDirectionUp() const { return _orientation.getUp(); };
        const glm::vec3& getHeadLookatDirectionRight() const { return _orientation.getRight(); };
		const glm::vec3& getHeadPosition() const ;
        const glm::vec3& getBonePosition(AvatarBoneID b) const { return _bone[b].position; };
        const glm::vec3& getBodyUpDirection() const { return _orientation.getUp(); };
        float getGirth();
        float getHeight();
        
		AvatarMode getMode();
		
		void setMousePressed( bool pressed ); 
        void render(bool lookingInMirror);
		void renderBody();
		void renderHead(bool lookingInMirror);
        void simulate(float);
		void startHandMovement();
		void stopHandMovement();
		void setHandMovementValues( glm::vec3 movement );
		void updateHandMovement( float deltaTime );
		void updateArmIKAndConstraints( float deltaTime );
        
        float getAverageLoudness() {return _head.averageLoudness;};
        void setAverageLoudness(float al) {_head.averageLoudness = al;};
         
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
    
    private:
        AvatarHead        _head;    
        bool              _isMine;
        glm::vec3         _TEST_bigSpherePosition;
        float             _TEST_bigSphereRadius;
		bool              _mousePressed;
		float             _bodyYawDelta;
		bool              _usingBodySprings;
		glm::vec3         _movedHandOffset;
		float             _springVelocityDecay;
		float             _springForce;
        glm::quat         _rotation; // the rotation of the avatar body as a whole expressed as a quaternion
		AvatarBone	      _bone[ NUM_AVATAR_BONES ];
		AvatarMode        _mode;
        AvatarHandHolding _handHolding;
        glm::dvec3        _velocity;
        glm::vec3	      _thrust;
        float		      _maxArmLength;
        Orientation	      _orientation;
        int               _driveKeys[MAX_DRIVE_KEYS];
        GLUquadric*       _sphere;
        float             _renderYaw;
        float             _renderPitch; //   Pitch from view frustum when this is own head
        timeval           _transmitterTimer;
        float             _transmitterHz;
        int               _transmitterPackets;
        Avatar*           _interactingOther;
        bool              _interactingOtherIsNearby;
        
        // private methods...
		void initializeSkeleton();
		void updateSkeleton();
		void initializeBodySprings();
		void updateBodySprings( float deltaTime );
		void calculateBoneLengths();
        void readSensors();
        void renderBoneAsBlock( AvatarBoneID b );
        void updateAvatarCollisionDetectionAndResponse
        ( 
            glm::vec3 collisionPosition, 
            float     collisionGirth, 
            float     collisionHeight, 
            glm::vec3 collisionUpVector, 
            float     deltaTime 
        );
};

#endif
