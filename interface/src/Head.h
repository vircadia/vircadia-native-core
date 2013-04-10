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
#include "AgentData.h"
#include "Field.h"
#include "world.h"
#include "Hand.h"
#include "Vector3D.h"		// added by Ventrella as a utility
#include "Orientation.h"	// added by Ventrella as a utility
#include "InterfaceConfig.h"
#include "SerialInterface.h"

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

/*
enum AvatarJoints
{
	AVATAR_JOINT_NULL = -1,
	AVATAR_JOINT_PELVIS,
	AVATAR_JOINT_TORSO,
	AVATAR_JOINT_CHEST,
	AVATAR_JOINT_NECK_BASE,
	AVATAR_JOINT_HEAD_BASE,
	AVATAR_JOINT_HEAD_TOP,
	
	AVATAR_JOINT_LEFT_CLAVICLE,
	AVATAR_JOINT_LEFT_SHOULDER,
	AVATAR_JOINT_LEFT_ELBOW,
	AVATAR_JOINT_LEFT_WRIST,
	AVATAR_JOINT_LEFT_FINGERTIPS,
	
	AVATAR_JOINT_RIGHT_CLAVICLE,
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
*/




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
	AvatarBones	parent;
	glm::vec3	worldPosition;
	glm::vec3	defaultPosePosition;
	glm::dvec3	velocity;
	float		yaw;
	float		pitch;
	float		roll;
	Orientation	worldOrientation;
	float		length;
};

struct Avatar
{
	glm::vec3	position;
	glm::dvec3	velocity;
	glm::vec3	thrust;
	float		yaw;
	float		pitch;
	float		roll;
	float		yawDelta;
	float		maxArmLength;
	Orientation	orientation;
	AvatarBone	bone[ NUM_AVATAR_BONES ];
};

class Head : public AgentData {
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
		
		float getAvatarYaw();
        
        void render(int faceToFace, int isMine);
		
		void setAvatarPosition( float, float, float );
		void renderBody();
		void renderHead( int faceToFace, int isMine );

        void simulate(float);
				
		void setHandMovement( glm::vec3 movement );
		void updateHandMovement();
        
        //  Send and receive network data
        int getBroadcastData(char * data);
        void parseData(void *data, int size);
        
        float getLoudness() {return loudness;};
        float getAverageLoudness() {return averageLoudness;};
        void setAverageLoudness(float al) {averageLoudness = al;};
        void setLoudness(float l) {loudness = l;};
        
        void SetNewHeadTarget(float, float);
        glm::vec3 getPos() { return position; };
        void setPos(glm::vec3 newpos) { position = newpos; };
    
        //  Set what driving keys are being pressed to control thrust levels
        void setDriveKeys(int key, bool val) { driveKeys[key] = val; };
        bool getDriveKeys(int key) { return driveKeys[key]; };
    
        //  Set/Get update the thrust that will move the avatar around
        void setThrust(glm::vec3 newThrust) { thrust = newThrust; };
        void addThrust(glm::vec3 newThrust) { thrust += newThrust; };
        glm::vec3 getThrust() { return thrust; };
    
        Hand * hand;
    
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
    
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 thrust;
		
		glm::vec3 handOffset;
    
        int driveKeys[MAX_DRIVE_KEYS];
        
        int eyeContact;
        eyeContactTargets eyeContactTarget;
    
        GLUquadric *sphere;
		Avatar avatar;
		
		void initializeAvatar();
		void updateAvatarSkeleton();
		void calculateBoneLengths();
    
        void readSensors();
        float renderYaw, renderPitch;       //   Pitch from view frustum when this is own head.
    
};

#endif
