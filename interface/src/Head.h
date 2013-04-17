//
//  Head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__head__
#define __interface__head__

#include <iostream>

#include <AvatarData.h>
#include <Orientation.h>

#include "Field.h"
#include "world.h"

#include "InterfaceConfig.h"
#include "SerialInterface.h"

//#include <glm/glm.hpp>

#include <glm/gtc/quaternion.hpp> and <glm/gtx/quaternion.hpp>


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
	AVATAR_MODE_COMMUNICATING,
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
    glm::quat   rotation;               // this will eventually replace yaw, pitch and roll (and maybe orienttion)
	float		yaw;					// the yaw Euler angle of the bone rotation off the parent
	float		pitch;					// the pitch Euler angle of the bone rotation off the parent
	float		roll;					// the roll Euler angle of the bone rotation off the parent
	Orientation	orientation;			// three orthogonal normals determined by yaw, pitch, roll
	float		length;					// the length of the bone
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
        Head();
        ~Head();
        Head(const Head &otherHead);
        Head* clone() const;
    
        void reset();
        void UpdatePos(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity);
        void setNoise (float mag) { noise = mag; }
        void setPitch(float p) {Pitch = p; }
        void setYaw(float y) {Yaw = y; }
        void setRoll(float r) {Roll = r; };
        void setScale(float s) {scale = s; };
        void setRenderYaw(float y) {renderYaw = y;}
        void setRenderPitch(float p) {renderPitch = p;}
        float getRenderYaw() {return renderYaw;}
        float getRenderPitch() {return renderPitch;}
        void setLeanForward(float dist);
        void setLeanSideways(float dist);
        void addPitch(float p) {Pitch -= p; }
        void addYaw(float y){Yaw -= y; }
        void addRoll(float r){Roll += r; }
        void addLean(float x, float z);
        float getPitch() {return Pitch;}
        float getRoll() {return Roll;}
        float getYaw() {return Yaw;}
        float getLastMeasuredYaw() {return YawRate;}
		
		float getBodyYaw();
		glm::vec3 getHeadLookatDirection();
		glm::vec3 getHeadLookatDirectionUp();
		glm::vec3 getHeadLookatDirectionRight();
		glm::vec3 getHeadPosition();
		glm::vec3 getBonePosition( AvatarBones b );		
		
		AvatarMode getMode();
		
		void setTriggeringAction( bool trigger ); 
        
        void render(int faceToFace, int isMine);
		
		void renderBody();
		void renderHead( int faceToFace, int isMine );
		//void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size );

        void simulate(float);
				
		void setHandMovement( glm::vec3 movement );
		void updateHandMovement();
        
        float getLoudness() {return loudness;};
        float getAverageLoudness() {return averageLoudness;};
        void setAverageLoudness(float al) {averageLoudness = al;};
        void setLoudness(float l) {loudness = l;};
        
        void SetNewHeadTarget(float, float);
    
        //  Set what driving keys are being pressed to control thrust levels
        void setDriveKeys(int key, bool val) { driveKeys[key] = val; };
        bool getDriveKeys(int key) { return driveKeys[key]; };
    
        //  Set/Get update the thrust that will move the avatar around
        void setThrust(glm::vec3 newThrust) { avatar.thrust = newThrust; };
        void addThrust(glm::vec3 newThrust) { avatar.thrust += newThrust; };
        glm::vec3 getThrust() { return avatar.thrust; };
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //

        void processTransmitterData(unsigned char * packetData, int numBytes);
        float getTransmitterHz() { return transmitterHz; };
    
    private:
        float noise;
        float Pitch;
        float Yaw;
        float Roll;
        float PitchRate;
        float YawRate;
        float RollRate;
        float EyeballPitch[2];
        float EyeballYaw[2];
        float EyebrowPitch[2];
        float EyebrowRoll[2];
        float EyeballScaleX, EyeballScaleY, EyeballScaleZ;
        float interPupilDistance;
        float interBrowDistance;
        float NominalPupilSize;
        float PupilSize;
        float MouthPitch;
        float MouthYaw;
        float MouthWidth;
        float MouthHeight;
        float leanForward;
        float leanSideways;
        float PitchTarget; 
        float YawTarget; 
        float NoiseEnvelope;
        float PupilConverge;
        float scale;
        
        //  Sound loudness information
        float loudness, lastLoudness;
        float averageLoudness;
        float audioAttack;
        float browAudioLift;
		
		bool triggeringAction;
    
		float bodyYawDelta;
		
		float		closeEnoughToInteract;
		int			closestOtherAvatar;
        
        //temporary - placeholder for real other avs
		glm::vec3	DEBUG_otherAvatarListPosition	[ NUM_OTHER_AVATARS ];
		float		DEBUG_otherAvatarListTimer		[ NUM_OTHER_AVATARS ];
		
		bool usingSprings;
		
		bool handBeingMoved;
		bool previousHandBeingMoved;
		glm::vec3 movedHandOffset;
    
        int driveKeys[MAX_DRIVE_KEYS];
		
		float springVelocityDecay;
		float springForce;
        
        int eyeContact;
        eyeContactTargets eyeContactTarget;
    
        GLUquadric *sphere;
		Avatar avatar;
        
        glm::quat rotation; // the rotation of the avatar body as a whole

		AvatarBone	bone[ NUM_AVATAR_BONES ];
		
		AvatarMode mode;

        float renderYaw, renderPitch;       //   Pitch from view frustum when this is own head.
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //
        timeval transmitterTimer;
        float transmitterHz;
        int transmitterPackets;

        
        //-------------------------------------------
        // private methods...
        //-------------------------------------------
		void initializeAvatar();
		void initializeSkeleton();
		void updateSkeleton();
		void initializeBodySprings();
		void updateBodySprings( float deltaTime );
		void calculateBoneLengths();
    
        void readSensors();
};

#endif
