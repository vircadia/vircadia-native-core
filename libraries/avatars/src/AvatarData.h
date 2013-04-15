//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <iostream>

#include <glm/glm.hpp>
#include <AgentData.h>

#include "Orientation.h"

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
	float		springBodyTightness;	// how tightly (0 to 1) the springy position tries to stay on the position
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
	AvatarBone	bone[ NUM_AVATAR_BONES ];
};

class AvatarData : public AgentData {
public:
    AvatarData();
    ~AvatarData();
    
    void parseData(void *data, int size);
    AvatarData* clone() const;
    
    float getPitch();
    void setPitch(float pitch);
    float getYaw();
    void setYaw(float yaw);
    float getRoll();
    void setRoll(float roll);
    float getHeadPositionX();
    float getHeadPositionY();
    float getHeadPositionZ();
    void setHeadPosition(float x, float y, float z);
    float getLoudness();
    void setLoudness(float loudness);
    float getAverageLoudness();
    void setAverageLoudness(float averageLoudness);
    float getHandPositionX();
    float getHandPositionY();
    float getHandPositionZ();
    void setHandPosition(float x, float y, float z);

private:
    float _pitch;
    float _yaw;
    float _roll;
    float _headPositionX;
    float _headPositionY;
    float _headPositionZ;
    float _loudness;
    float _averageLoudness;
    float _handPositionX;
    float _handPositionY;
    float _handPositionZ;
};

#endif /* defined(__hifi__AvatarData__) */
