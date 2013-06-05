//
//  Skeleton.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Skeleton_h
#define hifi_Skeleton_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

const glm::vec3 JOINT_DIRECTION = glm::vec3(1.0f, 0.0f, 0.0f);

class Skeleton {
public:
    Skeleton();

    void initialize();
    void update(float deltaTime, const glm::quat&, glm::vec3 position);
    void render();
    
    float getArmLength();
    float getHeight();
    float getPelvisStandingHeight();
    float getPelvisFloatingHeight();
    //glm::vec3 getJointVectorFromParent(AvatarJointID jointID) {return joint[jointID].position - joint[joint[jointID].parent].position; }
    
    struct AvatarJoint
    {
        AvatarJointID parent;                    // which joint is this joint connected to?
        glm::vec3     position;                  // the position at the "end" of the joint - in global space
        glm::vec3     defaultPosePosition;       // the parent relative position when the avatar is in the default pose
        glm::vec3     bindPosePosition;          // the parent relative position when the avatar is in the "T-pose"
        glm::vec3     absoluteBindPosePosition;  // the absolute position when the avatar is in the "T-pose"
        glm::quat     absoluteBindPoseRotation;  // the absolute rotation when the avatar is in the "T-pose"
        float         bindRadius;                // the radius of the bone capsule that envelops the vertices to bind
        glm::quat     rotation;                  // the parent-relative rotation (orientation) of the joint as a quaternion
        float         length;                    // the length of vector connecting the joint and its parent
    };

    AvatarJoint    joint[ NUM_AVATAR_JOINTS ];        
 };

#endif
