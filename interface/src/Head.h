//
//  Head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__head__
#define __interface__head__

#include <AvatarData.h>
#include <Orientation.h>

#include "Field.h"
#include "world.h"

#include "InterfaceConfig.h"
#include "SerialInterface.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> //looks like we might not need this


// Note to self: 
// modes I might need to implement for avatar
//
// walking                   usingSprings - ramping down
// bumping into sphere       usingSprings is on
// moving hand               usingSprings is on
// stuck to another hand     usingSprings is on


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

#define NUM_OTHER_AVATARS 5 // temporary - for testing purposes!

enum AvatarMode
{
	AVATAR_MODE_STANDING = 0,
	AVATAR_MODE_WALKING,
	AVATAR_MODE_INTERACTING,
	NUM_AVATAR_MODES
};

enum AvatarBones
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

struct AvatarBone
{
	AvatarBones	parent;					// which bone is this bone connected to?
	glm::vec3	position;				// the position at the "end" of the bone
	glm::vec3	defaultPosePosition;	// the parent relative position when the avatar is in the "T-pose"
	glm::vec3	springyPosition;		// used for special effects (a 'flexible' variant of position)
	glm::dvec3	springyVelocity;		// used for special effects ( the velocity of the springy position)
	float		springBodyTightness;	// how tightly the springy position tries to stay on the position
    glm::quat   rotation;               // this will eventually replace yaw, pitch and roll (and maybe orientation)
	float		yaw;					// the yaw Euler angle of the bone rotation off the parent
	float		pitch;					// the pitch Euler angle of the bone rotation off the parent
	float		roll;					// the roll Euler angle of the bone rotation off the parent
	Orientation	orientation;			// three orthogonal normals determined by yaw, pitch, roll
	float		length;					// the length of the bone
	float		radius;					// used for detecting collisions for certain physical effects
};

struct Avatar
{
	glm::dvec3	velocity;
	glm::vec3	thrust;
	float		maxArmLength;
	Orientation	orientation;
};

class Head : public AvatarData {
    public:
        Head(bool isMine);
        ~Head();
        Head(const Head &otherHead);
        Head* clone() const;
    
        void reset();
        void UpdateGyros(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity);
        void setNoise (float mag) { _noise = mag; }
        void setPitch(float p) {_headPitch = p; }
        void setYaw(float y) {_headYaw = y; }
        void setRoll(float r) {_headRoll = r; };
        void setScale(float s) {_scale = s; };
        void setRenderYaw(float y) {_renderYaw = y;}
        void setRenderPitch(float p) {_renderPitch = p;}
        float getRenderYaw() {return _renderYaw;}
        float getRenderPitch() {return _renderPitch;}
        void setLeanForward(float dist);
        void setLeanSideways(float dist);
        void addPitch(float p) {_headPitch -= p; }
        void addYaw(float y){_headYaw -= y; }
        void addRoll(float r){_headRoll += r; }
        void addLean(float x, float z);
        float getPitch() {return _headPitch;}
        float getRoll() {return _headRoll;}
        float getYaw() {return _headYaw;}
        float getLastMeasuredYaw() {return _headYawRate;}
		
        float getBodyYaw() {return _bodyYaw;};
        void addBodyYaw(float y) {_bodyYaw += y;};
    
		glm::vec3 getHeadLookatDirection();
		glm::vec3 getHeadLookatDirectionUp();
		glm::vec3 getHeadLookatDirectionRight();
		glm::vec3 getHeadPosition();
		glm::vec3 getBonePosition( AvatarBones b );		
		
		AvatarMode getMode();
		
		void setTriggeringAction( bool trigger ); 
        
        void render(int faceToFace);
		
		void renderBody();
		void renderHead( int faceToFace);
		//void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size );

        void simulate(float);
				
		void startHandMovement();
		void stopHandMovement();
		void setHandMovementValues( glm::vec3 movement );
		void updateHandMovement();
        
        float getLoudness() {return _loudness;};
        float getAverageLoudness() {return _averageLoudness;};
        void setAverageLoudness(float al) {_averageLoudness = al;};
        void setLoudness(float l) {_loudness = l;};
        
        void SetNewHeadTarget(float, float);
    
        //  Set what driving keys are being pressed to control thrust levels
        void setDriveKeys(int key, bool val) { _driveKeys[key] = val; };
        bool getDriveKeys(int key) { return _driveKeys[key]; };
    
        //  Set/Get update the thrust that will move the avatar around
        void setThrust(glm::vec3 newThrust) { _avatar.thrust = newThrust; };
        void addThrust(glm::vec3 newThrust) { _avatar.thrust += newThrust; };
        glm::vec3 getThrust() { return _avatar.thrust; };
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //

        void processTransmitterData(unsigned char * packetData, int numBytes);
        float getTransmitterHz() { return _transmitterHz; };
    
    private:
        bool  _isMine;
        float _noise;
        float _headPitch;
        float _headYaw;
        float _headRoll;
        float _headPitchRate;
        float _headYawRate;
        float _headRollRate;
        float _eyeballPitch[2];
        float _eyeballYaw[2];
        float _eyebrowPitch[2];
        float _eyebrowRoll[2];
        float _eyeballScaleX, _eyeballScaleY, _eyeballScaleZ;
        float _interPupilDistance;
        float _interBrowDistance;
        float _nominalPupilSize;
        float _pupilSize;
        float _mouthPitch;
        float _mouthYaw;
        float _mouthWidth;
        float _mouthHeight;
        float _leanForward;
        float _leanSideways;
        float _pitchTarget; 
        float _yawTarget; 
        float _noiseEnvelope;
        float _pupilConverge;
        float _scale;
        
        //  Sound loudness information
        float _loudness, _lastLoudness;
        float _averageLoudness;
        float _audioAttack;
        float _browAudioLift;
        
        glm::vec3   _TEST_bigSpherePosition;
        float       _TEST_bigSphereRadius;
		glm::vec3	_DEBUG_otherAvatarListPosition[ NUM_OTHER_AVATARS ];
		bool        _triggeringAction;
		float       _bodyYawDelta;
		float       _closeEnoughToInteract;
		int         _closestOtherAvatar;
		bool        _usingBodySprings;
		glm::vec3   _movedHandOffset;
		float       _springVelocityDecay;
		float       _springForce;
        glm::quat   _rotation; // the rotation of the avatar body as a whole
		AvatarBone	_bone[ NUM_AVATAR_BONES ];
		AvatarMode  _mode;
		Avatar      _avatar;
        int         _driveKeys[MAX_DRIVE_KEYS];
        int         _eyeContact;
        eyeContactTargets _eyeContactTarget;
    
        GLUquadric *_sphere;

        float _renderYaw;
        float _renderPitch; //   Pitch from view frustum when this is own head.
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //
        timeval _transmitterTimer;
        float   _transmitterHz;
        int     _transmitterPackets;
        
        //-----------------------------
        // private methods...
        //-----------------------------
		void initializeSkeleton();
		void updateSkeleton();
		void initializeBodySprings();
		void updateBodySprings( float deltaTime );
		void calculateBoneLengths();
        void updateBigSphereCollisionTest( float deltaTime );
        void readSensors();
};

#endif
